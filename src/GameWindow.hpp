#pragma once

#ifndef GEKKO
#include <SDL2/SDL_video.h>
#endif

#include "ZunResult.hpp"
#include "graphics/GfxInterface.hpp"
#include "inttypes.hpp"

#define GAME_WINDOW_WIDTH (640)
#define GAME_WINDOW_HEIGHT (480)

#ifndef GAME_WINDOW_WIDTH_REAL
#define GAME_WINDOW_WIDTH_REAL (GAME_WINDOW_WIDTH)
#endif

#ifndef GAME_WINDOW_HEIGHT_REAL
#define GAME_WINDOW_HEIGHT_REAL (GAME_WINDOW_HEIGHT)
#endif

#define VIEWPORT_WIDTH GAME_WINDOW_WIDTH_REAL
#define VIEWPORT_OFF_X 0
#define VIEWPORT_HEIGHT GAME_WINDOW_HEIGHT_REAL
#define VIEWPORT_OFF_Y 0

#if (GAME_WINDOW_WIDTH_REAL * 3) > (GAME_WINDOW_HEIGHT_REAL * 4)
#undef VIEWPORT_WIDTH
#undef VIEWPORT_OFF_X
#define VIEWPORT_WIDTH ((u32)((GAME_WINDOW_HEIGHT_REAL / 3.0f) * 4.0f))
#define VIEWPORT_OFF_X ((GAME_WINDOW_WIDTH_REAL - VIEWPORT_WIDTH) / 2)
#elif (GAME_WINDOW_WIDTH_REAL * 3) < (GAME_WINDOW_HEIGHT_REAL * 4)
#undef VIEWPORT_HEIGHT
#undef VIEWPORT_OFF_Y
#define VIEWPORT_HEIGHT ((u32)((GAME_WINDOW_WIDTH_REAL / 4.0f) * 3.0f))
#define VIEWPORT_OFF_Y ((GAME_WINDOW_HEIGHT_REAL - VIEWPORT_HEIGHT) / 2)
#endif

#define WIDTH_RESOLUTION_SCALE (((f32)VIEWPORT_WIDTH) / GAME_WINDOW_WIDTH)
#define HEIGHT_RESOLUTION_SCALE (((f32)VIEWPORT_HEIGHT) / GAME_WINDOW_HEIGHT)

enum RenderResult
{
    RENDER_RESULT_KEEP_RUNNING,
    RENDER_RESULT_EXIT_SUCCESS,
    RENDER_RESULT_EXIT_ERROR,
};

struct GameWindow
{
    RenderResult Render();
    static void Present();

    static void CreateGameWindow();
    static ZunResult InitD3dRendering();
    static void InitD3dDevice();

    i32 isAppClosing;
    i32 lastActiveAppValue;
    i32 isAppActive;
    u8 curFrame;
    i32 screenSaveActive;
    i32 lowPowerActive;
    i32 powerOffActive;
    u32 renderBackendIndex;
};

extern GameWindow g_GameWindow;
extern i32 g_TickCountToEffectiveFramerate;
extern double g_LastFrameTime;
extern GfxInterface *g_GfxBackend;