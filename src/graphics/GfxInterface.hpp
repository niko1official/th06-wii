#pragma once

#include "ZunColor.hpp"
#include "ZunMath.hpp"
#include "inttypes.hpp"
#include <cstddef>

enum DepthFunc
{
    DEPTH_FUNC_LEQUAL,
    DEPTH_FUNC_ALWAYS
};

enum VertexAttributeFlags
{
    VERTEX_ATTR_TEX_COORD = (1 << 0),
    VERTEX_ATTR_DIFFUSE = (1 << 1),
};

enum VertexAttributeArrays
{
    VERTEX_ARRAY_POSITION,
    VERTEX_ARRAY_TEX_COORD,
    VERTEX_ARRAY_DIFFUSE
};

enum ColorOp
{
    COLOR_OP_MODULATE,
    COLOR_OP_ADD,
    COLOR_OP_REPLACE
};

enum TextureOpComponent
{
    COMPONENT_RGB,
    COMPONENT_ALPHA
};

enum TransformMatrix
{
    MATRIX_MODEL,
    MATRIX_VIEW,
    MATRIX_PROJECTION,
    MATRIX_TEXTURE
};
enum BlendMode
{
    BLEND_INV_SRC_ALPHA,
    BLEND_ONE,
};
enum Capabilities
{
    CAPS_BLEND,
    CAPS_DEPTH_TEST
};

enum PrimitiveType
{
    PRIM_TRIANGLE_STRIP,
    PRIM_TRIANGLES,
};

enum ClearBits
{
    CLEAR_COLOR_BUFFER = 1,
    CLEAR_DEPTH_BUFFER = 2
};

enum PixelFormat
{
    PIXEL_RGBA,
    PIXEL_RGB
};

enum PixelDataType
{
    PIXEL_UNSIGNED_BYTE,
    PIXEL_UNSIGNED_SHORT_5_5_5_1,
    PIXEL_UNSIGNED_SHORT_5_6_5,
    PIXEL_UNSIGNED_SHORT_4_4_4_4
};
struct GfxTextureHandle
{
    u32 id = 0;

    constexpr GfxTextureHandle() = default;
    constexpr GfxTextureHandle(u32 id) : id(id)
    {
    }

    constexpr operator u32() const
    {
        return id;
    }
    constexpr explicit operator bool() const
    {
        return id != 0;
    }
};

struct GfxInterface
{

    virtual ~GfxInterface() = default;

    virtual void SetFogRange(f32 nearPlane, f32 farPlane) = 0;
    virtual void SetFogColor(ZunColor color) = 0;
    virtual void ToggleVertexAttribute(u8 attr, bool enable) = 0;
    virtual void SetAttributePointer(VertexAttributeArrays attr, std::size_t stride, void *ptr) = 0;
    virtual void SetColorOp(TextureOpComponent component, ColorOp op) = 0;
    virtual void SetTextureFactor(ZunColor factor) = 0;
    virtual void SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix) = 0;

    virtual void SetTextureFilter() = 0;

    virtual void GetViewport(u32 *viewport) = 0;
    virtual void GetDepthRange(f32 *depthRange) = 0;
    virtual void SetViewport(i32 x, i32 y, i32 width, i32 height) = 0;
    virtual void SetDepthRange(f32 nearPlane, f32 farPlane) = 0;

    virtual void Enable(Capabilities cap) = 0;

    virtual bool HasError() = 0;

    virtual void SetBlendMode(BlendMode mode) = 0;
    virtual void SetDepthMask(bool enable) = 0;
    virtual void SetDepthFunc(DepthFunc func) = 0;

    virtual void SetClearDepth(f32 depth) = 0;
    virtual void SetClearColor(f32 r, f32 g, f32 b, f32 a) = 0;
    virtual void Clear(u32 clearBits) = 0;

    virtual GfxTextureHandle CreateTexture() = 0;
    virtual void BindTexture(GfxTextureHandle handle) = 0;
    virtual void DeleteTexture(GfxTextureHandle handle) = 0;
    virtual void SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type, const void *data) = 0;

    virtual void SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height, const void *data) = 0;

    virtual void ReadPixels(i32 x, i32 y, i32 width, i32 height, const void *pixels) = 0;

    virtual void Draw(PrimitiveType type, i32 start, i32 count) = 0;
    virtual void SwapBuffers() = 0;
};
