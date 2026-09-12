#include "GameWindow.hpp"
#include "AnmManager.hpp"
#include "GameErrorContext.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunMath.hpp"
#include "graphics/Software.hpp"
#include "i18n.hpp"
#include "utils.hpp"

#ifdef GEKKO
#include "graphics/GX.hpp"
#include <ogc/lwp_watchdog.h>
#else
#include "graphics/FixedFunctionGL.hpp"
#include "graphics/WebGL.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#endif

#include <cstring>

GameWindow g_GameWindow;
GfxInterface *g_GfxBackend;
i32 g_TickCountToEffectiveFramerate;
f64 g_LastFrameTime;

#define FRAME_TIME (1000. / 60.)

static inline u32 PlatformGetTicks()
{
#ifdef GEKKO
    return (u32)ticks_to_millisecs(gettime());
#else
    return SDL_GetTicks();
#endif
}

static const struct
{
    const char *name;
    GfxInterface *(*TryInit)();
} s_RenderBackends[] = {
#ifdef GEKKO
    {"GX (Wii/GameCube)", GXBackend::Init},
#else
    {"GL 2.1 / GL ES 2.0 / WebGL", WebGL::Create},
    {"Fixed function GL(ES)", FixedFunctionGL::Init},
#endif
    {"Software fallback (VERY SLOW)", Software::Init}};

RenderResult GameWindow::Render()
{
    i32 res;
    f64 slowdown;
    ZunViewport viewport;
    f64 delta;
    u32 curtime;

    if (this->lastActiveAppValue == 0)
    {
        return RENDER_RESULT_KEEP_RUNNING;
    }

    if (this->curFrame == 0)
    {
    RUN_CHAINS:
        if (g_Supervisor.cfg.frameskipConfig <= this->curFrame)
        {
            if (g_Supervisor.RedrawWholeFrame())
            {
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = GAME_WINDOW_WIDTH;
                viewport.height = GAME_WINDOW_HEIGHT;
                viewport.minZ = 0.0;
                viewport.maxZ = 1.0;
                viewport.Set();
                g_GfxBackend->SetClearColor(
                    ((g_Stage.skyFog.color >> 16) & 0xFF) / 255.0f, ((g_Stage.skyFog.color >> 8) & 0xFF) / 255.0f,
                    (g_Stage.skyFog.color & 0xFF) / 255.0f, (g_Stage.skyFog.color >> 24) / 255.0f);
                g_GfxBackend->Clear(CLEAR_COLOR_BUFFER | CLEAR_DEPTH_BUFFER);
                g_AnmManager->SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
                g_Supervisor.viewport.Set();
            }

            g_AnmManager->ClearVertexBuffer();
            g_AnmManager->flushesThisFrame = 0;
            g_Chain.RunDrawChain();
            g_AnmManager->SetCurrentTexture(0);
        }

        g_AnmManager->FlushVertexBuffer();
        g_Supervisor.viewport.x = 0;
        g_Supervisor.viewport.y = 0;
        g_Supervisor.viewport.width = GAME_WINDOW_WIDTH;
        g_Supervisor.viewport.height = GAME_WINDOW_HEIGHT;
        g_AnmManager->SetProjectionMode(PROJECTION_MODE_PERSPECTIVE);
        g_Supervisor.viewport.Set();
        res = g_Chain.RunCalcChain();
        g_SoundPlayer.PlaySounds();

        if (res == 0)
        {
            return RENDER_RESULT_EXIT_SUCCESS;
        }
        if (res == -1)
        {
            return RENDER_RESULT_EXIT_ERROR;
        }
        this->curFrame++;
    }

    if (g_Supervisor.cfg.windowed || g_Supervisor.ShouldRunAt60Fps())
    {
        if (this->curFrame != 0)
        {
            g_Supervisor.framerateMultiplier = 1.0;
            slowdown = PlatformGetTicks();
            if (slowdown < g_LastFrameTime)
            {
                g_LastFrameTime = slowdown;
            }
            delta = std::fabs(slowdown - g_LastFrameTime);
            if (delta >= FRAME_TIME)
            {
                do
                {
                    g_LastFrameTime += FRAME_TIME;
                    delta -= FRAME_TIME;
                } while (delta >= FRAME_TIME);

                if (g_Supervisor.cfg.frameskipConfig < this->curFrame)
                    goto I_HAVE_NO_CLUE_WHY_BUT_I_MUST_JUMP_HERE;
                goto RUN_CHAINS;
            }
        }
    }
    else
    {
        if (g_Supervisor.cfg.frameskipConfig >= this->curFrame)
        {
            Present();
            goto RUN_CHAINS;
        }

    I_HAVE_NO_CLUE_WHY_BUT_I_MUST_JUMP_HERE:
        Present();
        if (g_Supervisor.framerateMultiplier == 0.f)
        {
            if (2 <= g_TickCountToEffectiveFramerate)
            {
                curtime = PlatformGetTicks();
                if (curtime < g_Supervisor.lastFrameTime)
                {
                    g_Supervisor.lastFrameTime = curtime;
                }
                delta = curtime - g_Supervisor.lastFrameTime;
                delta = (delta * 60.) / 2. / 1000.;
                delta /= (g_Supervisor.cfg.frameskipConfig + 1);
                if (delta >= .865)
                {
                    delta = 1.0;
                }
                else if (delta >= .6)
                {
                    delta = 0.8;
                }
                else
                {
                    delta = 0.5;
                }
                g_Supervisor.effectiveFramerateMultiplier = delta;
                g_Supervisor.lastFrameTime = curtime;
                g_TickCountToEffectiveFramerate = 0;
            }
        }
        else
        {
            g_Supervisor.effectiveFramerateMultiplier = g_Supervisor.framerateMultiplier;
        }
        this->curFrame = 0;
        g_TickCountToEffectiveFramerate = g_TickCountToEffectiveFramerate + 1;
    }
    return RENDER_RESULT_KEEP_RUNNING;
}

void GameWindow::Present()
{

    g_AnmManager->TakeScreenshotIfRequested();
    if (g_Supervisor.unk198 != 0)
    {
        g_Supervisor.unk198--;
    }

    g_GfxBackend->SwapBuffers();

    return;
}

void GameWindow::CreateGameWindow()
{
#ifdef GEKKO

#else
    SDL_Init(SDL_INIT_GAMECONTROLLER);
#endif

    for (u32 i = 0; i < ARRAY_SIZE(s_RenderBackends); i++)
    {
        g_GfxBackend = s_RenderBackends[i].TryInit();
        if (g_GfxBackend)
        {
            utils::DebugPrint2("Using renderer backend %s", s_RenderBackends[i].name);
            break;
        }
        utils::DebugPrint2("Renderer creation for backend %s failed", s_RenderBackends[i].name);
    }

    g_GameWindow.lastActiveAppValue = 1;
}

ZunResult GameWindow::InitD3dRendering()
{
    if (!g_GfxBackend)
    {
        g_GameErrorContext.Fatal(TH_ERR_D3D_INIT_FAILED);
        return ZUN_ERROR;
    }

    ZunVec3 eye;
    ZunVec3 at;
    ZunVec3 up;
    f32 half_width;
    f32 half_height;
    f32 aspect_ratio;
    f32 field_of_view_y;
    f32 camera_distance;

    g_AnmManager->CreateTextureObject();
    g_AnmManager->dummyTextureHandle = g_AnmManager->currentTextureHandle;
    g_GfxBackend->SetTextureImage(1, 1, PIXEL_RGBA, PIXEL_UNSIGNED_BYTE, NULL);

    if (!g_Supervisor.cfg.windowed)
    {
        if ((((g_Supervisor.cfg.opts >> GCOS_FORCE_16BIT_COLOR_MODE) & 1) == 1))
        {

            g_Supervisor.cfg.colorMode16bit = 1;
        }
        else if (g_Supervisor.cfg.colorMode16bit == 0xff)
        {

            g_Supervisor.cfg.colorMode16bit = 0;
            g_GameErrorContext.Log(TH_ERR_SCREEN_INIT_32BITS);

        }

        if (!((g_Supervisor.cfg.opts >> GCOS_FORCE_60FPS) & 1))
        {

        }
        else
        {

        }

    }

    g_Supervisor.vsyncEnabled = 1;

    g_Supervisor.lockableBackbuffer = 1;

    half_width = (float)GAME_WINDOW_WIDTH / 2.0;
    half_height = (float)GAME_WINDOW_HEIGHT / 2.0;
    aspect_ratio = (float)GAME_WINDOW_WIDTH / (float)GAME_WINDOW_HEIGHT;
    field_of_view_y = 0.52359879;
    camera_distance = half_height / ZUN_TANF(field_of_view_y / 2.0f);
    up.x = 0.0;
    up.y = 1.0;
    up.z = 0.0;
    at.x = half_width;
    at.y = -half_height;
    at.z = 0.0;
    eye.x = half_width;
    eye.y = -half_height;
    eye.z = -camera_distance;

    ZunMatrix viewMatrix = createViewMatrix(eye, at, up);
    g_AnmManager->SetTransformMatrix(MATRIX_VIEW, viewMatrix);
    g_Supervisor.viewMatrix = viewMatrix;

    ZunMatrix perspectiveMatrix = perspectiveMatrixFromFOV(field_of_view_y, aspect_ratio, 100.0f, 10000.0f);
    g_AnmManager->SetTransformMatrix(MATRIX_PROJECTION, perspectiveMatrix);
    g_Supervisor.projectionMatrix = perspectiveMatrix;

    g_Supervisor.viewport.Get();

    InitD3dDevice();
    ScreenEffect::SetViewport(0);
    g_GameWindow.isAppClosing = 0;
    g_Supervisor.lastFrameTime = 0;
    g_Supervisor.framerateMultiplier = 0.0;
    return ZUN_SUCCESS;
}

void GameWindow::InitD3dDevice(void)
{
    AnmManager *anm1;
    AnmManager *anm2;
    AnmManager *anm3;
    AnmManager *anm4;

    g_GfxBackend->Enable(CAPS_BLEND);

    g_GfxBackend->SetBlendMode(BLEND_INV_SRC_ALPHA);

    if (((g_Supervisor.cfg.opts >> GCOS_TURN_OFF_DEPTH_TEST) & 1) == 0)
    {
        g_GfxBackend->Enable(CAPS_DEPTH_TEST);
        g_AnmManager->SetDepthMask(true);
        g_AnmManager->SetDepthFunc(DEPTH_FUNC_LEQUAL);
    }

    g_AnmManager->SetFogColor(0xFF'A0'A0'A0);
    g_AnmManager->SetFogRange(1'000.0f, 5'000.0f);

    if (g_AnmManager != NULL)
    {
        anm1 = g_AnmManager;
        anm1->currentBlendMode = 0xff;
        anm4 = g_AnmManager;
        anm4->currentTextureHandle = 0;
    }
    g_Stage.skyFogNeedsSetup = 1;
    return;
}
