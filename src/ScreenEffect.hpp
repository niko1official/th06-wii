#pragma once

#include "Chain.hpp"
#include "ZunColor.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"

struct ZunRect
{
    f32 left;
    f32 top;
    f32 right;
    f32 bottom;
};

enum ScreenEffects
{
    SCREEN_EFFECT_FADE_IN,
    SCREEN_EFFECT_SHAKE,
    SCREEN_EFFECT_FADE_OUT,
};

struct ScreenEffect
{

    static ScreenEffect *RegisterChain(i32 effect, u32 ticks, u32 effectParam1, u32 effectParam2,
                                       u32 unusedEffectParam);

    static ZunResult AddedCallback(ScreenEffect *effect);
    static ZunResult DeletedCallback(ScreenEffect *effect);

    static ChainCallbackResult DrawFadeIn(ScreenEffect *effect);
    static ChainCallbackResult CalcFadeIn(ScreenEffect *effect);
    static ChainCallbackResult ShakeScreen(ScreenEffect *effect);
    static ChainCallbackResult DrawFadeOut(ScreenEffect *effect);
    static ChainCallbackResult CalcFadeOut(ScreenEffect *effect);

    static void DrawSquare(const ZunRect *rect, ZunColor rectColor);
    static void Clear(ZunColor color);
    static void SetViewport(ZunColor color);

    ScreenEffects usedEffect;
    ChainElem *calcChainElement;
    ChainElem *drawChainElement;
    u32 unused;
    i32 fadeAlpha;
    i32 effectLength;
    i32 genericParam;
    i32 shakinessParam;
    i32 unusedParam;
    ZunTimer timer;
};
