#pragma once

#include "AnmVm.hpp"
#include "inttypes.hpp"

struct StageMenu
{
    StageMenu();

    i32 OnUpdateGameMenu();
    i32 OnUpdateRetryMenu();

    void OnDrawGameMenu();
    void OnDrawRetryMenu();

    u32 curState;

    i32 numFrames;
    AnmVm menuSprites[6];
    AnmVm menuBackground;
};
