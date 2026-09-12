#pragma once

#ifdef GEKKO

#ifdef COLOR_BLACK
#undef COLOR_BLACK
#undef COLOR_WHITE
#undef COLOR_RED
#undef COLOR_YELLOW
#endif
#include <gccore.h>
#undef COLOR_BLACK
#undef COLOR_WHITE
#undef COLOR_RED
#undef COLOR_YELLOW
#endif

#include "GfxInterface.hpp"
#include <cstddef>

#ifndef COLOR_BLACK
#define COLOR_BLACK 0xff000000
#define COLOR_WHITE 0xffffffff
#define COLOR_RED 0xffff0000
#define COLOR_YELLOW 0xffffff00
#endif

#ifdef GEKKO

struct GXBackend : GfxInterface
{
    static GfxInterface *Init();
    void Exit();
    ~GXBackend() override
    {
        Exit();
    }

    virtual void SetFogRange(f32 nearPlane, f32 farPlane) override;
    virtual void SetFogColor(ZunColor color) override;
    virtual void ToggleVertexAttribute(u8 attr, bool enable) override;
    virtual void SetAttributePointer(VertexAttributeArrays attr, std::size_t stride, void *ptr) override;
    virtual void SetColorOp(TextureOpComponent component, ColorOp op) override;
    virtual void SetTextureFactor(ZunColor factor) override;
    virtual void SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix) override;

    virtual void SetTextureFilter() override;

    virtual void GetViewport(u32 *viewport) override;
    virtual void GetDepthRange(f32 *depthRange) override;
    virtual void SetViewport(i32 x, i32 y, i32 width, i32 height) override;
    virtual void SetDepthRange(f32 nearPlane, f32 farPlane) override;

    virtual void Enable(Capabilities cap) override;
    virtual bool HasError() override;
    virtual void SetBlendMode(BlendMode mode) override;
    virtual void SetDepthMask(bool enable) override;
    virtual void SetDepthFunc(DepthFunc func) override;

    virtual void SetClearDepth(f32 depth) override;
    virtual void SetClearColor(f32 r, f32 g, f32 b, f32 a) override;
    virtual void Clear(u32 clearBits) override;

    virtual GfxTextureHandle CreateTexture() override;
    virtual void BindTexture(GfxTextureHandle handle) override;
    virtual void DeleteTexture(GfxTextureHandle handle) override;
    virtual void SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type, const void *data) override;
    virtual void SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height, const void *data) override;

    virtual void ReadPixels(i32 x, i32 y, i32 width, i32 height, const void *pixels) override;

    virtual void Draw(PrimitiveType type, i32 start, i32 count) override;
    virtual void SwapBuffers() override;

  private:

    struct
    {
        void *ptr = nullptr;
        std::size_t stride = 0;
        bool enabled = false;
    } attrs[3];

    ZunColor textureFactor = 0xffffffff;
    ColorOp rgbOp = COLOR_OP_MODULATE;
    ColorOp alphaOp = COLOR_OP_MODULATE;
    BlendMode blendMode = BLEND_INV_SRC_ALPHA;
    bool blendEnabled = false;
    bool depthTestEnabled = false;
    bool depthMask = true;
    DepthFunc depthFunc = DEPTH_FUNC_LEQUAL;
    f32 fogStart = 1000.0f;
    f32 fogEnd = 5000.0f;
    f32 projectionNear = 100.0f;
    f32 projectionFar = 10000.0f;
    GXColor fogColor{0xa0, 0xa0, 0xa0, 0xff};

    ZunMatrix modelMatrix{};
    ZunMatrix viewMatrix{};
    ZunMatrix projMatrix{};
    ZunMatrix textureMatrix{};

    struct TextureSlot
    {
        GXTexObj obj{};
        void *gxData = nullptr;
        u8 *rgba = nullptr;
        u32 width = 0;
        u32 height = 0;
        bool allocated = false;
        bool valid = false;
    };

    static constexpr u32 MAX_TEXTURES = 512;
    TextureSlot textures[MAX_TEXTURES];
    u32 boundTexture = 0;
    u32 nextTextureId = 1;
    bool projectionIsOrtho = false;
    GXColor clearColor{0, 0, 0, 0xff};

    i32 viewportX = 0;
    i32 viewportY = 0;
    i32 viewportWidth = 0;
    i32 viewportHeight = 0;
    f32 viewportNear = 0.0f;
    f32 viewportFar = 1.0f;

    void ApplyModelViewMatrix();
    void ApplyProjectionMatrix();
    void ApplyViewport();
    void ApplyFog();
    void ApplyBlendAndDepthState();
    void ConfigureTev();
    void UploadTexture(TextureSlot &slot);
};

#endif
