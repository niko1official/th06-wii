#ifdef GEKKO

#include "GX.hpp"
#include "Supervisor.hpp"
#include "utils.hpp"

#include <cmath>
#include <cstring>
#include <gccore.h>
#include <malloc.h>

static void *s_Frame_Buffer[2] = {nullptr, nullptr};
static u32 s_FbIndex = 0;
static GXRModeObj *s_RMode = nullptr;

#define GP_FIFO_SIZE (256 * 1024)
static void *s_GpFifo = nullptr;
static bool s_ClearAfterPresent = false;

#define TH_VTXFMT GX_VTXFMT0

GfxInterface *GXBackend::Init()
{
    VIDEO_Init();

    s_RMode = VIDEO_GetPreferredMode(nullptr);

    s_Frame_Buffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(s_RMode));
    s_Frame_Buffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(s_RMode));

    VIDEO_Configure(s_RMode);
    VIDEO_SetNextFramebuffer(s_Frame_Buffer[0]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (s_RMode->viTVMode & VI_NON_INTERLACE)
    {
        VIDEO_WaitVSync();
    }

    s_GpFifo = memalign(32, GP_FIFO_SIZE);
    memset(s_GpFifo, 0, GP_FIFO_SIZE);
    GX_Init(s_GpFifo, GP_FIFO_SIZE);

    GX_SetCopyClear((GXColor){0, 0, 0, 0xff}, GX_MAX_Z24);

    GX_SetViewport(0, 0, s_RMode->fbWidth, s_RMode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)s_RMode->xfbHeight / (f32)s_RMode->efbHeight);
    GX_SetScissor(0, 0, s_RMode->fbWidth, s_RMode->efbHeight);
    GX_SetDispCopySrc(0, 0, s_RMode->fbWidth, s_RMode->efbHeight);
    GX_SetDispCopyDst(s_RMode->fbWidth, s_RMode->xfbHeight);
    GX_SetCopyFilter(s_RMode->aa, s_RMode->sample_pattern, GX_TRUE, s_RMode->vfilter);
    GX_SetFieldMode(s_RMode->field_rendering, ((s_RMode->viHeight == 2 * s_RMode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(s_Frame_Buffer[0], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    GX_SetVtxAttrFmt(TH_VTXFMT, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(TH_VTXFMT, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetVtxAttrFmt(TH_VTXFMT, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);

    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
    {

        Mtx identity;
        guMtxIdentity(identity);
        GX_LoadTexMtxImm(identity, GX_TEXMTX0, GX_MTX3x4);
    }
    GX_SetNumTevStages(1);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
    GX_SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_1);

    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);

    GX_SetAlphaCompare(GX_GEQUAL, 4, GX_AOP_AND, GX_ALWAYS, 0);

    GXBackend *backend = new GXBackend();

    backend->modelMatrix.Identity();
    backend->viewMatrix.Identity();
    backend->projMatrix.Identity();
    backend->textureMatrix.Identity();
    backend->projectionIsOrtho = true;
    backend->ApplyModelViewMatrix();
    backend->ApplyProjectionMatrix();
    backend->viewportWidth = s_RMode->fbWidth;
    backend->viewportHeight = s_RMode->efbHeight;
    backend->SetTextureFactor(0xffffffff);
    utils::DebugPrint2("Using renderer backend GX (Wii/GameCube)");
    return backend;
}

void GXBackend::Exit()
{
    for (u32 i = 1; i < MAX_TEXTURES; i++)
    {
        if (textures[i].gxData)
            free(textures[i].gxData);
        if (textures[i].rgba)
            free(textures[i].rgba);
        textures[i].gxData = nullptr;
        textures[i].rgba = nullptr;
        textures[i].allocated = false;
        textures[i].valid = false;
    }

    if (s_GpFifo)
    {
        free(s_GpFifo);
        s_GpFifo = nullptr;
    }
}

void GXBackend::SetFogRange(f32 nearPlane, f32 farPlane)
{
    fogStart = nearPlane;
    fogEnd = farPlane;
    ApplyFog();
}

void GXBackend::SetFogColor(ZunColor color)
{
    fogColor = {(u8)((color >> 16) & 0xff), (u8)((color >> 8) & 0xff), (u8)(color & 0xff),
                (u8)((color >> 24) & 0xff)};
    ApplyFog();
}

void GXBackend::ToggleVertexAttribute(u8 attr, bool enable)
{
    if (attr & VERTEX_ATTR_TEX_COORD)
    {
        attrs[VERTEX_ARRAY_TEX_COORD].enabled = enable;
    }
    if (attr & VERTEX_ATTR_DIFFUSE)
    {
        attrs[VERTEX_ARRAY_DIFFUSE].enabled = enable;
    }
    ConfigureTev();
}

void GXBackend::SetAttributePointer(VertexAttributeArrays attr, std::size_t stride, void *ptr)
{
    attrs[attr].ptr = ptr;
    attrs[attr].stride = stride;
    if (attr == VERTEX_ARRAY_POSITION)
    {
        attrs[attr].enabled = true;
    }
}

void GXBackend::ConfigureTev()
{
    const bool textured = attrs[VERTEX_ARRAY_TEX_COORD].enabled;
    const bool diffuse = attrs[VERTEX_ARRAY_DIFFUSE].enabled;

    GX_SetNumTexGens(textured ? 1 : 0);
    if (textured)
    {
        GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, diffuse ? GX_COLOR0A0 : GX_COLOR0A0);
    }
    else
    {
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, diffuse ? GX_COLOR0A0 : GX_COLOR0A0);
    }

    GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);

    GX_SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_1);

    if (textured)
    {
        if (rgbOp == COLOR_OP_REPLACE)
            GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
        else if (rgbOp == COLOR_OP_ADD)
            GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ONE,
                             diffuse ? GX_CC_RASC : GX_CC_KONST);
        else
            GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, diffuse ? GX_CC_RASC : GX_CC_KONST, GX_CC_ZERO);

        if (alphaOp == COLOR_OP_REPLACE)
            GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
        else if (alphaOp == COLOR_OP_ADD)
            GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST,
                             diffuse ? GX_CA_RASA : GX_CA_A0);
        else
            GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, diffuse ? GX_CA_RASA : GX_CA_A0, GX_CA_ZERO);
    }
    else
    {
        if (rgbOp == COLOR_OP_REPLACE)
            GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
        else
            GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);

        GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    }

    GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
}

void GXBackend::SetColorOp(TextureOpComponent component, ColorOp op)
{
    if (component == COMPONENT_RGB)
        rgbOp = op;
    else
        alphaOp = op;
    ConfigureTev();
}

void GXBackend::SetTextureFactor(ZunColor factor)
{
    textureFactor = factor;
    GXColor c = {(u8)((factor >> 16) & 0xff), (u8)((factor >> 8) & 0xff), (u8)(factor & 0xff),
                 (u8)((factor >> 24) & 0xff)};
    GX_SetTevKColor(GX_KCOLOR0, c);
    GX_SetTevColor(GX_TEVREG0, c);
}

void GXBackend::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    switch (type)
    {
    case MATRIX_MODEL:
        modelMatrix = matrix;
        ApplyModelViewMatrix();
        break;
    case MATRIX_VIEW:
        viewMatrix = matrix;
        ApplyModelViewMatrix();
        break;
    case MATRIX_PROJECTION:
        projMatrix = matrix;

        projectionIsOrtho = std::fabs(matrix.m[3][3]) > 0.5f;

        ApplyModelViewMatrix();
        ApplyProjectionMatrix();
        break;
    case MATRIX_TEXTURE:
        textureMatrix = matrix;
        {
            Mtx tm;
            for (int row = 0; row < 3; row++)
                for (int col = 0; col < 4; col++)
                    tm[row][col] = matrix.m[col][row];
            GX_LoadTexMtxImm(tm, GX_TEXMTX0, GX_MTX3x4);
        }
        break;
    }
}

void GXBackend::ApplyModelViewMatrix()
{
    ZunMatrix mv = viewMatrix * modelMatrix;
    Mtx m;

    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 4; col++)
        {
            m[row][col] = mv.m[col][row];
        }
    }

    if (!projectionIsOrtho)
    {
        for (int col = 0; col < 4; col++)
            m[2][col] = -m[2][col];
    }
    GX_LoadPosMtxImm(m, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
}

void GXBackend::ApplyProjectionMatrix()
{
    Mtx44 p;
    for (int row = 0; row < 4; row++)
    {
        for (int col = 0; col < 4; col++)
        {
            p[row][col] = projMatrix.m[col][row];
        }
    }

    if (!projectionIsOrtho)
    {

        for (int row = 0; row < 4; row++)
            p[row][2] = -p[row][2];

        const f32 zScale = projMatrix.m[2][2];
        const f32 zOffset = projMatrix.m[3][2];
        if (std::fabs(zScale + 1.0f) > 0.00001f && std::fabs(zScale - 1.0f) > 0.00001f)
        {
            projectionNear = -zOffset / (zScale + 1.0f);
            projectionFar = -zOffset / (zScale - 1.0f);
        }
    }

    for (int col = 0; col < 4; col++)
        p[2][col] = 0.5f * (p[2][col] - p[3][col]);

    GX_LoadProjectionMtx(p, projectionIsOrtho ? GX_ORTHOGRAPHIC : GX_PERSPECTIVE);
    ApplyFog();
}

void GXBackend::ApplyFog()
{

    GX_SetFog(GX_FOG_NONE, 0.0f, 1.0f, 0.1f, 1.0f, fogColor);
}

void GXBackend::SetTextureFilter()
{

}

void GXBackend::GetViewport(u32 *viewport)
{
    if (!viewport)
        return;
    viewport[0] = (u32)viewportX;
    viewport[1] = (u32)viewportY;
    viewport[2] = (u32)viewportWidth;
    viewport[3] = (u32)viewportHeight;
}

void GXBackend::GetDepthRange(f32 *depthRange)
{
    if (!depthRange)
        return;
    depthRange[0] = viewportNear;
    depthRange[1] = viewportFar;
}

void GXBackend::SetViewport(i32 x, i32 y, i32 width, i32 height)
{
    viewportX = x;
    viewportY = y;
    viewportWidth = width;
    viewportHeight = height;
    ApplyViewport();
}

void GXBackend::SetDepthRange(f32 nearPlane, f32 farPlane)
{
    viewportNear = nearPlane;
    viewportFar = farPlane;
    ApplyViewport();
}

void GXBackend::ApplyViewport()
{
    if (!s_RMode)
        return;

    const i32 gxY = (i32)s_RMode->efbHeight - (viewportY + viewportHeight);
    GX_SetViewport((f32)viewportX, (f32)gxY, (f32)viewportWidth, (f32)viewportHeight, viewportNear, viewportFar);
    GX_SetScissor((u32)viewportX, (u32)gxY, (u32)viewportWidth, (u32)viewportHeight);
}

void GXBackend::Enable(Capabilities cap)
{
    if (cap == CAPS_BLEND)
    {
        blendEnabled = true;
    }
    else if (cap == CAPS_DEPTH_TEST)
    {
        depthTestEnabled = true;
    }
    ApplyBlendAndDepthState();
}

bool GXBackend::HasError()
{

    return false;
}

void GXBackend::SetBlendMode(BlendMode mode)
{
    blendMode = mode;
    ApplyBlendAndDepthState();
}

void GXBackend::SetDepthMask(bool enable)
{
    depthMask = enable;
    ApplyBlendAndDepthState();
}

void GXBackend::SetDepthFunc(DepthFunc func)
{
    depthFunc = func;
    ApplyBlendAndDepthState();
}

void GXBackend::ApplyBlendAndDepthState()
{
    u8 gxDepthFunc = (depthFunc == DEPTH_FUNC_ALWAYS) ? GX_ALWAYS : GX_LEQUAL;
    GX_SetZMode(depthTestEnabled ? GX_TRUE : GX_FALSE, gxDepthFunc, depthMask ? GX_TRUE : GX_FALSE);

    if (blendEnabled)
    {
        if (blendMode == BLEND_ONE)
        {
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR);
        }
        else
        {
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
        }
    }
    else
    {
        GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    }
}

void GXBackend::SetClearDepth(f32 depth)
{
    (void)depth;
}

void GXBackend::SetClearColor(f32 r, f32 g, f32 b, f32 a)
{
    clearColor = (GXColor){(u8)(r * 255), (u8)(g * 255), (u8)(b * 255), (u8)(a * 255)};
    GX_SetCopyClear(clearColor, GX_MAX_Z24);
}

void GXBackend::Clear(u32 clearBits)
{

    if (clearBits & (CLEAR_COLOR_BUFFER | CLEAR_DEPTH_BUFFER))
        s_ClearAfterPresent = true;
}

GfxTextureHandle GXBackend::CreateTexture()
{
    for (u32 checked = 1; checked < MAX_TEXTURES; checked++)
    {
        const u32 id = nextTextureId;
        nextTextureId++;
        if (nextTextureId >= MAX_TEXTURES)
            nextTextureId = 1;

        if (!textures[id].allocated)
        {
            textures[id].allocated = true;
            return GfxTextureHandle(id);
        }
    }
    return GfxTextureHandle();
}

void GXBackend::BindTexture(GfxTextureHandle handle)
{
    boundTexture = handle.id;
    if (boundTexture == 0 || boundTexture >= MAX_TEXTURES || !textures[boundTexture].valid)
        return;

    GX_LoadTexObj(&textures[boundTexture].obj, GX_TEXMAP0);
}

void GXBackend::DeleteTexture(GfxTextureHandle handle)
{
    if (handle.id == 0 || handle.id >= MAX_TEXTURES)
        return;
    TextureSlot &slot = textures[handle.id];
    if (slot.gxData)
        free(slot.gxData);
    if (slot.rgba)
        free(slot.rgba);
    slot.gxData = nullptr;
    slot.rgba = nullptr;
    slot.width = slot.height = 0;
    slot.allocated = false;
    slot.valid = false;
}

static inline void ReadPixelRGBA(const u8 *src, PixelFormat fmt, PixelDataType type, u8 *r, u8 *g, u8 *b, u8 *a)
{
    *a = 255;
    if (type == PIXEL_UNSIGNED_BYTE)
    {
        *r = src[0];
        *g = src[1];
        *b = src[2];
        if (fmt == PIXEL_RGBA)
            *a = src[3];
        return;
    }

    u16 v;
    std::memcpy(&v, src, sizeof(v));
    if (type == PIXEL_UNSIGNED_SHORT_5_5_5_1)
    {
        *r = (u8)(((v >> 11) & 31) * 255 / 31);
        *g = (u8)(((v >> 6) & 31) * 255 / 31);
        *b = (u8)(((v >> 1) & 31) * 255 / 31);
        *a = (v & 1) ? 255 : 0;
    }
    else if (type == PIXEL_UNSIGNED_SHORT_5_6_5)
    {
        *r = (u8)(((v >> 11) & 31) * 255 / 31);
        *g = (u8)(((v >> 5) & 63) * 255 / 63);
        *b = (u8)((v & 31) * 255 / 31);
    }
    else
    {
        *r = (u8)(((v >> 12) & 15) * 17);
        *g = (u8)(((v >> 8) & 15) * 17);
        *b = (u8)(((v >> 4) & 15) * 17);
        *a = (u8)((v & 15) * 17);
    }
}

void GXBackend::UploadTexture(TextureSlot &slot)
{
    if (!slot.rgba || slot.width == 0 || slot.height == 0)
        return;

    const u32 paddedW = (slot.width + 3) & ~3u;
    const u32 paddedH = (slot.height + 3) & ~3u;
    const u32 texSize = GX_GetTexBufferSize(slot.width, slot.height, GX_TF_RGBA8, GX_FALSE, 0);

    if (!slot.gxData)
        slot.gxData = memalign(32, (texSize + 31) & ~31u);
    if (!slot.gxData)
        return;

    std::memset(slot.gxData, 0, (texSize + 31) & ~31u);
    u8 *dst = (u8 *)slot.gxData;
    u32 out = 0;

    for (u32 by = 0; by < paddedH; by += 4)
    {
        for (u32 bx = 0; bx < paddedW; bx += 4)
        {
            for (u32 ty = 0; ty < 4; ty++)
            {
                for (u32 tx = 0; tx < 4; tx++)
                {
                    const u32 x = bx + tx;
                    const u32 y = by + ty;
                    const u8 *p = (x < slot.width && y < slot.height) ? slot.rgba + (y * slot.width + x) * 4 : nullptr;
                    dst[out++] = p ? p[3] : 0;
                    dst[out++] = p ? p[0] : 0;
                }
            }
            for (u32 ty = 0; ty < 4; ty++)
            {
                for (u32 tx = 0; tx < 4; tx++)
                {
                    const u32 x = bx + tx;
                    const u32 y = by + ty;
                    const u8 *p = (x < slot.width && y < slot.height) ? slot.rgba + (y * slot.width + x) * 4 : nullptr;
                    dst[out++] = p ? p[1] : 0;
                    dst[out++] = p ? p[2] : 0;
                }
            }
        }
    }

    DCFlushRange(slot.gxData, (texSize + 31) & ~31u);
    GX_InitTexObj(&slot.obj, slot.gxData, slot.width, slot.height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjFilterMode(&slot.obj, GX_LINEAR, GX_LINEAR);
    slot.valid = true;
}

void GXBackend::SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type, const void *data)
{
    if (boundTexture == 0 || boundTexture >= MAX_TEXTURES)
        return;

    TextureSlot &slot = textures[boundTexture];
    if (slot.gxData)
        free(slot.gxData);
    if (slot.rgba)
        free(slot.rgba);
    slot.gxData = nullptr;
    slot.rgba = nullptr;
    slot.width = width;
    slot.height = height;
    slot.valid = false;

    if (width == 0 || height == 0)
        return;

    slot.rgba = (u8 *)memalign(32, width * height * 4);
    if (!slot.rgba)
        return;

    const u32 bpp = type == PIXEL_UNSIGNED_BYTE ? (fmt == PIXEL_RGBA ? 4 : 3) : 2;
    const u8 *src = (const u8 *)data;
    const bool isDummyWhiteTexture = !src && width == 1 && height == 1;
    for (u32 y = 0; y < height; y++)
    {
        for (u32 x = 0; x < width; x++)
        {
            u8 *d = slot.rgba + (y * width + x) * 4;
            if (src)
                ReadPixelRGBA(src + (y * width + x) * bpp, fmt, type, &d[0], &d[1], &d[2], &d[3]);
            else
                d[0] = d[1] = d[2] = d[3] = isDummyWhiteTexture ? 255 : 0;
        }
    }
    UploadTexture(slot);

    if (src)
    {
        free(slot.rgba);
        slot.rgba = nullptr;
    }
    GX_LoadTexObj(&slot.obj, GX_TEXMAP0);
    GX_InvalidateTexAll();
}

void GXBackend::SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height, const void *data)
{
    if (!data || boundTexture == 0 || boundTexture >= MAX_TEXTURES)
        return;
    TextureSlot &slot = textures[boundTexture];
    if (!slot.gxData || xoffset < 0 || yoffset < 0 || xoffset + width > (i32)slot.width ||
        yoffset + height > (i32)slot.height)
        return;

    const u8 *src = (const u8 *)data;
    u8 *dst = (u8 *)slot.gxData;
    const u32 tilesPerRow = (slot.width + 3) / 4;
    for (i32 y = 0; y < height; y++)
    {
        for (i32 x = 0; x < width; x++)
        {
            const u32 dstX = (u32)(xoffset + x);
            const u32 dstY = (u32)(yoffset + y);
            const u32 tileOffset = ((dstY / 4) * tilesPerRow + dstX / 4) * 64;
            const u32 pixelOffset = ((dstY & 3) * 4 + (dstX & 3)) * 2;
            const u8 *s = src + ((u32)y * (u32)width + (u32)x) * 4;
            dst[tileOffset + pixelOffset] = s[3];
            dst[tileOffset + pixelOffset + 1] = s[0];
            dst[tileOffset + 32 + pixelOffset] = s[1];
            dst[tileOffset + 32 + pixelOffset + 1] = s[2];

            if (slot.rgba)
                std::memcpy(slot.rgba + (dstY * slot.width + dstX) * 4, s, 4);
        }
    }

    const u32 texSize = GX_GetTexBufferSize(slot.width, slot.height, GX_TF_RGBA8, GX_FALSE, 0);
    DCFlushRange(slot.gxData, (texSize + 31) & ~31u);
    GX_InvalidateTexAll();
    GX_LoadTexObj(&slot.obj, GX_TEXMAP0);
}

void GXBackend::ReadPixels(i32 x, i32 y, i32 width, i32 height, const void *pixels)
{
    if (!pixels || !s_RMode || width <= 0 || height <= 0)
        return;

    GX_DrawDone();
    u8 *dst = (u8 *)pixels;
    for (i32 row = 0; row < height; row++)
    {
        for (i32 col = 0; col < width; col++)
        {
            const i32 gxX = x + col;
            const i32 gxY = (i32)s_RMode->efbHeight - 1 - (y + row);
            GXColor color = {0, 0, 0, 0};
            if (gxX >= 0 && gxX < (i32)s_RMode->fbWidth && gxY >= 0 && gxY < (i32)s_RMode->efbHeight)
                GX_PeekARGB((u16)gxX, (u16)gxY, &color);

            u8 *pixel = dst + ((row * width + col) * 4);
            pixel[0] = color.r;
            pixel[1] = color.g;
            pixel[2] = color.b;
            pixel[3] = color.a;
        }
    }
}

void GXBackend::Draw(PrimitiveType type, i32 start, i32 count)
{

    if (boundTexture != 0 && boundTexture < MAX_TEXTURES && textures[boundTexture].valid)
        GX_LoadTexObj(&textures[boundTexture].obj, GX_TEXMAP0);
    u8 gxPrim = (type == PRIM_TRIANGLE_STRIP) ? GX_TRIANGLESTRIP : GX_TRIANGLES;

    GX_Begin(gxPrim, TH_VTXFMT, count);
    for (i32 i = 0; i < count; i++)
    {
        i32 vtx = start + i;

        auto &pos = attrs[VERTEX_ARRAY_POSITION];
        const f32 *p = (const f32 *)((const u8 *)pos.ptr + vtx * pos.stride);
        GX_Position3f32(p[0], p[1], p[2]);

        if (attrs[VERTEX_ARRAY_DIFFUSE].enabled)
        {
            auto &d = attrs[VERTEX_ARRAY_DIFFUSE];

            const u8 *rgba = (const u8 *)d.ptr + vtx * d.stride;
            GX_Color4u8(rgba[0], rgba[1], rgba[2], rgba[3]);
        }
        else
        {
            GX_Color4u8(0xff, 0xff, 0xff, 0xff);
        }

        if (attrs[VERTEX_ARRAY_TEX_COORD].enabled)
        {
            auto &tc = attrs[VERTEX_ARRAY_TEX_COORD];
            const f32 *t = (const f32 *)((const u8 *)tc.ptr + vtx * tc.stride);
            GX_TexCoord2f32(t[0], t[1]);
        }
        else
        {
            GX_TexCoord2f32(0.0f, 0.0f);
        }
    }
    GX_End();
}

void GXBackend::SwapBuffers()
{
    const u32 nextFb = s_FbIndex ^ 1;

    GX_CopyDisp(s_Frame_Buffer[nextFb], s_ClearAfterPresent ? GX_TRUE : GX_FALSE);
    s_ClearAfterPresent = false;
    GX_Flush();

    VIDEO_SetNextFramebuffer(s_Frame_Buffer[nextFb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    s_FbIndex = nextFb;
}

#endif
