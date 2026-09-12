#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>
#include <cstdio>
#include <cstring>

#ifdef GEKKO
#include <fat.h>
#include <unistd.h>

static void ChangeToExecutableDirectory(int argc, char *argv[])
{

    if (argc > 0 && argv && argv[0])
    {
        const char *lastSlash = std::strrchr(argv[0], '/');
        const size_t length = lastSlash ? (size_t)(lastSlash - argv[0]) : 0;
        if (length > 0 && length < 1024)
        {
            char directory[1024];
            std::memcpy(directory, argv[0], length);
            directory[length] = '\0';
            if (chdir(directory) == 0)
                return;
        }
    }
    const char *fallbackDirectories[] = {"sd:/apps/th06", "usb:/apps/th06"};
    for (const char *directory : fallbackDirectories)
    {
        if (chdir(directory) == 0)
            return;
    }
}
#endif

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameWindow.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "i18n.hpp"
#include "utils.hpp"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

#ifdef GEKKO

    if (!fatInitDefault())
        return 1;
    ChangeToExecutableDirectory(argc, argv);
#endif

    i32 renderResult = 0;

    if (g_Supervisor.LoadConfig(TH_CONFIG_FILE) != ZUN_SUCCESS)
    {
        g_GameErrorContext.Flush();
        return -1;
    }

restart:
    GameWindow::CreateGameWindow();

    g_AnmManager = new AnmManager();

    if (GameWindow::InitD3dRendering() != ZUN_SUCCESS)
    {
        g_GameErrorContext.Flush();
        return 1;
    }

    g_SoundPlayer.InitializeDSound();
    Controller::GetJoystickCaps();
    Controller::ResetKeyboard();

    if (Supervisor::RegisterChain() != ZUN_SUCCESS)
    {
        goto stop;
    }
#ifndef GEKKO
    if (!g_Supervisor.cfg.windowed)
    {
        SDL_ShowCursor(SDL_DISABLE);
    }
#endif

    g_GameWindow.curFrame = 0;

    while (true)
    {
#ifndef GEKKO
        SDL_Event e;

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                goto stop;
            }
        }
#endif

        renderResult = g_GameWindow.Render();
        if (renderResult != 0)
        {
            break;
        }

    }

stop:
    g_Chain.Release();
    g_SoundPlayer.Release();

    delete g_AnmManager;
    g_AnmManager = NULL;

    if (g_GfxBackend != NULL)
        delete g_GfxBackend;
    SDL_Quit();

    if (renderResult == 2)
    {
        g_GameErrorContext.ResetContext();

        g_GameErrorContext.Log(TH_ERR_OPTION_CHANGED_RESTART);

        if (!g_Supervisor.cfg.windowed)
        {
            SDL_ShowCursor(SDL_ENABLE);
        }

        goto restart;
    }

    FileSystem::WriteDataToFile(TH_CONFIG_FILE, &g_Supervisor.cfg, sizeof(g_Supervisor.cfg));

    SDL_ShowCursor(SDL_ENABLE);
    g_GameErrorContext.Flush();
    return 0;
}
