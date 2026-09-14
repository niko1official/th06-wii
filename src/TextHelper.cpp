#include "TextHelper.hpp"
#include "GameErrorContext.hpp"
#include "GameWindow.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"

#include "thirdparty/sjis_converter.h"

#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cstring>
#include <vector>

static TTF_Font *g_Font;
static i32 g_FontSize;

TextHelper::TextHelper()
{

}

TextHelper::~TextHelper()
{
    TTF_Quit();
}

#define TEXT_BUFFER_HEIGHT 64

ZunResult TextHelper::CreateTextBuffer()
{
    TTF_Init();

    if ((g_Font = TTF_OpenFont(TH_PRIMARY_FONT_FILENAME, 10), g_Font == NULL) &&
        (std::printf("%s\n", TTF_GetError()), g_Font = TTF_OpenFont(TH_FALLBACK_FONT_FILENAME, 10), g_Font == NULL))
    {
        std::printf("%s\n", TTF_GetError());

        g_GameErrorContext.Fatal(TH_ERR_FONTS_NOT_FOUND);
        return ZUN_ERROR;
    }

    g_FontSize = 10;

    g_TextBufferSurface =
        SDL_CreateRGBSurfaceWithFormat(0, GAME_WINDOW_WIDTH, TEXT_BUFFER_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);

    SDL_SetSurfaceBlendMode(g_TextBufferSurface, SDL_BLENDMODE_NONE);

    return ZUN_SUCCESS;
}

bool TextHelper::InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight)
{
    u8 *bufferCursor;
    i32 gradientArea;
    i32 i = 0;

    gradientArea = spriteWidth * fontHeight;

    SDL_LockSurface(g_TextBufferSurface);

    for (bufferCursor = (u8 *)g_TextBufferSurface->pixels; i < gradientArea; i++, bufferCursor += 4)
    {
        if (bufferCursor[3])
        {
            bufferCursor[0] = bufferCursor[0] - bufferCursor[0] * i / gradientArea / 2;
            bufferCursor[1] = bufferCursor[1] - bufferCursor[1] * i / gradientArea / 2;
            bufferCursor[2] = bufferCursor[2] - bufferCursor[2] * i / gradientArea / 4;
        }
    }

    SDL_UnlockSurface(g_TextBufferSurface);

    return true;
}

bool isUTF8Encoded(const char *string)
{
#define UTF8_1BYTE_MASK 0x80
#define UTF8_2BYTE_MASK 0xE0
#define UTF8_3BYTE_MASK 0xF0
#define UTF8_4BYTE_MASK 0xF8

#define UTF8_2NDBYTE_MASK 0xC0

#define UTF8_1BYTE_PREFIX 0x00

#define UTF8_2BYTE_PREFIX 0xC0

#define UTF8_3BYTE_PREFIX 0xE0

#define UTF8_4BYTE_PREFIX 0xF0

#define UTF8_2NDBYTE_PREFIX 0x80

    bool isMultiByteParse = false;
    int codepointLen = 0;

    while (*string != '\0')
    {
        unsigned char c = *(unsigned char *)string;

        if (!isMultiByteParse)
        {
            if ((c & UTF8_1BYTE_MASK) != UTF8_1BYTE_PREFIX)
            {
                isMultiByteParse = true;

                if ((c & UTF8_2BYTE_MASK) == UTF8_2BYTE_PREFIX)
                    codepointLen = 1;
                else if ((c & UTF8_3BYTE_MASK) == UTF8_3BYTE_PREFIX)
                    codepointLen = 2;
                else if ((c & UTF8_4BYTE_MASK) == UTF8_4BYTE_PREFIX)
                    codepointLen = 3;
                else
                    return false;
            }
        }
        else
        {
            if ((c & UTF8_2NDBYTE_MASK) != UTF8_2NDBYTE_PREFIX)
                return false;

            if (--codepointLen == 0)
                isMultiByteParse = false;
        }

        string++;
    }

    return true;

#undef UTF8_1BYTE_MASK
#undef UTF8_2BYTE_MASK
#undef UTF8_3BYTE_MASK
#undef UTF8_4BYTE_MASK

#undef UTF8_2NDBYTE_MASK

#undef UTF8_1BYTE_PREFIX
#undef UTF8_2BYTE_PREFIX
#undef UTF8_3BYTE_PREFIX
#undef UTF8_4BYTE_PREFIX

#undef UTF8_2NDBYTE_PREFIX
}

void SurfaceOverwriteBlend(SDL_Surface *srcSurface, SDL_Surface *dstSurface, u32 x)
{

    SDL_LockSurface(srcSurface);
    SDL_LockSurface(dstSurface);

    u32 *srcData = (u32 *)srcSurface->pixels;
    u8 *dstData = (u8 *)dstSurface->pixels;

    for (int i = 0; i < srcSurface->h; i++)
    {
        for (int j = 0; j < srcSurface->w; j++)
        {
            if ((srcData[j] & 0xFF00'0000) != 0)
            {
                dstData[i * dstSurface->pitch + (x + j) * 4] = (srcData[j] >> 16) & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 1] = (srcData[j] >> 8) & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 2] = srcData[j] & 0xFF;
                dstData[i * dstSurface->pitch + (x + j) * 4 + 3] = (srcData[j] >> 24) & 0xFF;
            }
        }

        srcData += srcSurface->pitch / 4;
    }

    SDL_UnlockSurface(dstSurface);
    SDL_UnlockSurface(srcSurface);
}

void TextHelper::RenderTextToTexture(i32 xPos, i32 yPos, i32 spriteWidth, i32 spriteHeight, i32 fontHeight,
                                     i32 fontWidth, ZunColor textColor, ZunColor shadowColor, const char *string,
                                     TextureData *outTexture)
{
    char convertedText[1024];
    SDL_Rect finalCopyDst;
    SDL_Rect finalCopySrc;
    SDL_Rect shadowRect;

    if (!isUTF8Encoded(string))
    {
        char *utf8 = sjis2utf8(string);
        strcpy(convertedText, utf8);
        free(utf8);
    }
    else
    {
        strcpy(convertedText, string);
    }

    i32 requestedFontSize = fontHeight * 2;
    if (requestedFontSize != g_FontSize && TTF_SetFontSize(g_Font, requestedFontSize) == 0)
        g_FontSize = requestedFontSize;

    finalCopySrc.x = 0;
    finalCopySrc.y = 0;
    finalCopySrc.w = spriteWidth * 2 - 2;
    finalCopySrc.h = fontHeight * 2 - 2;

    SDL_FillRect(g_TextBufferSurface, &finalCopySrc, 0);

    if (shadowColor != COLOR_WHITE)
    {
        SDL_Surface *shadowText;

        SDL_Color sdlShadowColor;
        sdlShadowColor.a = 0xFF;
        sdlShadowColor.b = (shadowColor >> 16) & 0xFF;
        sdlShadowColor.g = (shadowColor >> 8) & 0xFF;
        sdlShadowColor.r = shadowColor & 0xFF;

        shadowText = TTF_RenderUTF8_Blended(g_Font, convertedText, sdlShadowColor);

        if (shadowText != NULL)
        {
            shadowRect.x = xPos * 2 + 3;
            shadowRect.y = 2;
            shadowRect.w = shadowText->w;
            shadowRect.h = shadowText->h;

            SDL_SetSurfaceBlendMode(shadowText, SDL_BLENDMODE_NONE);
            SDL_BlitSurface(shadowText, NULL, g_TextBufferSurface, &shadowRect);

            SDL_FreeSurface(shadowText);
        }
    }

    SDL_Color sdlTextColor;
    sdlTextColor.a = 0xFF;
    sdlTextColor.b = (textColor >> 16) & 0xFF;
    sdlTextColor.g = (textColor >> 8) & 0xFF;
    sdlTextColor.r = textColor & 0xFF;

    SDL_Surface *regularText = TTF_RenderUTF8_Blended(g_Font, convertedText, sdlTextColor);

    if (regularText != NULL)
    {
        SurfaceOverwriteBlend(regularText, g_TextBufferSurface, xPos * 2);

        SDL_FreeSurface(regularText);
    }

    bool textureRecreated = !outTexture->textureData || outTexture->format != TEX_FMT_A8R8G8B8;
    if (textureRecreated)
    {
        free(outTexture->textureData);
        outTexture->textureData = (u8 *)malloc(outTexture->width * outTexture->height * 4);
        memset(outTexture->textureData, 0, outTexture->width * outTexture->height * 4);
    }

    outTexture->format = TEX_FMT_A8R8G8B8;
    SDL_Surface *textureSurface = SDL_CreateRGBSurfaceWithFormatFrom(
        outTexture->textureData, outTexture->width, outTexture->height, SDL_BITSPERPIXEL(SDL_PIXELFORMAT_RGBA32),
        outTexture->width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA32), SDL_PIXELFORMAT_RGBA32);

    InvertAlpha(0, 0, spriteWidth * 2, fontHeight * 2 + 6);

    finalCopyDst.x = 0;
    finalCopyDst.y = yPos;
    finalCopyDst.w = spriteWidth;
    finalCopyDst.h = 16;

    if (SDL_SoftStretchLinear(g_TextBufferSurface, &finalCopySrc, textureSurface, &finalCopyDst) < 0)
    {
        SDL_Log("SDL_BlitScaled failed! Error: %s", SDL_GetError());
    }

    g_AnmManager->SetCurrentTexture(outTexture->handle);

    if (textureRecreated)
    {
        g_GfxBackend->SetTextureImage(outTexture->width, outTexture->height, PIXEL_RGBA, PIXEL_UNSIGNED_BYTE,
                                      outTexture->textureData);
    }
    else
    {
        i32 updateX = 0;
        i32 updateY = std::max(0, finalCopyDst.y);
        i32 updateWidth = std::min(finalCopyDst.w, (i32)outTexture->width);
        i32 updateHeight = std::min(finalCopyDst.h, (i32)outTexture->height - updateY);

        if (updateWidth > 0 && updateHeight > 0)
        {
            std::vector<u8> updatedPixels(updateWidth * updateHeight * 4);
            for (i32 row = 0; row < updateHeight; row++)
            {
                std::memcpy(updatedPixels.data() + row * updateWidth * 4,
                            outTexture->textureData + ((updateY + row) * outTexture->width + updateX) * 4,
                            updateWidth * 4);
            }
            g_GfxBackend->SetTextureSubImage(updateX, updateY, updateWidth, updateHeight, updatedPixels.data());
        }
    }

    SDL_FreeSurface(textureSurface);

    return;
}

void TextHelper::ReleaseTextBuffer()
{
    if (g_Font != NULL)
    {
        TTF_CloseFont(g_Font);
        g_Font = NULL;
        g_FontSize = 0;
    }

    if (g_TextBufferSurface != NULL)
    {
        SDL_FreeSurface(g_TextBufferSurface);
        g_TextBufferSurface = NULL;
    }

    return;
}
