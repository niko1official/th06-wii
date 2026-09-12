#pragma once

#include "AnmManager.hpp"
#include "ZunColor.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"

struct TextHelper
{
    static ZunResult CreateTextBuffer();
    static void ReleaseTextBuffer();
    static void RenderTextToTexture(i32 xPos, i32 yPos, i32 spriteWidth, i32 spriteHeight, i32 fontHeight,
                                    i32 fontWidth, ZunColor textColor, ZunColor shadowColor, const char *string,
                                    TextureData *outTexture);

    TextHelper();
    ~TextHelper();

    static bool InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight);

    i32 width;
    i32 height;
    u32 imageSizeInBytes;
    i32 imageWidthInBytes;

    u8 *buffer;
};
