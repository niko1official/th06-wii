#pragma once

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "StageMenu.hpp"
#include "ZunColor.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"

#define TEXT_RIGHT_ARROW 0x7f

struct AsciiManagerString
{
    char text[64];
    ZunVec3 position;
    ZunColor color;
    ZunVec2 scale;

    u32 isSelected;

    u32 isGui;
};

struct AsciiManagerPopup
{
    char digits[8];
    ZunVec3 position;
    ZunColor color;
    ZunTimer timer;
    u8 inUse;
    u8 characterCount;
};

struct WeirdPadding
{
    u32 unk;
};

struct AsciiManager
{
    AsciiManager();

    static ZunResult RegisterChain();
    static void CutChain();

    static ChainCallbackResult OnUpdate(AsciiManager *s);
    static ChainCallbackResult OnDrawMenus(AsciiManager *s);
    static ChainCallbackResult OnDrawPopups(AsciiManager *s);
    static ZunResult AddedCallback(AsciiManager *s);
    static ZunResult DeletedCallback(AsciiManager *s);

    void InitializeVms();

    void DrawStrings();
    void DrawPopupsWithHwVertexProcessing();
    void DrawPopupsWithoutHwVertexProcessing();

    void AddString(const ZunVec3 *position, const char *text);
    void AddFormatText(const ZunVec3 *position, const char *fmt, ...);
    void CreatePopup1(const ZunVec3 *position, i32 value, ZunColor color);
    void CreatePopup2(const ZunVec3 *position, i32 value, ZunColor color);

    void SetColor(ZunColor color)
    {
        this->color = color;
    }

    AnmVm vm0;
    AnmVm vm1;
    AsciiManagerString strings[256];
    i32 numStrings;
    ZunColor color;
    ZunVec2 scale;

    u32 isGui;

    bool isSelected;
    i32 nextPopupIndex1;
    i32 nextPopupIndex2;

    WeirdPadding unk3;

    StageMenu gameMenu;

    StageMenu retryMenu;
    AsciiManagerPopup popups[515];
};
extern AsciiManager g_AsciiManager;
