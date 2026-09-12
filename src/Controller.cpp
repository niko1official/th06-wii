#include "Controller.hpp"

#ifndef GEKKO
#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "i18n.hpp"
#include "utils.hpp"
#endif

#ifdef GEKKO

#include <ogc/pad.h>
#include <wiiuse/wpad.h>

u16 Controller::GetJoystickCaps(void)
{

    PAD_Init();
    if (WPAD_Init() < 0)
        return 1;

    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS);
    return 0;
}

void Controller::ResetKeyboard(void)
{

}

u16 Controller::GetInput(void)
{
    u16 buttons = 0;

    WPAD_ScanPads();
    PAD_ScanPads();

    u32 wiimoteHeld = WPAD_ButtonsHeld(0);
    u32 gcHeld = PAD_ButtonsHeld(0);

    if (wiimoteHeld & (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN | WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT | WPAD_BUTTON_2 |
                       WPAD_BUTTON_1 | WPAD_BUTTON_A | WPAD_BUTTON_B | WPAD_BUTTON_PLUS | WPAD_BUTTON_HOME) ||
        gcHeld)
    {

        if (wiimoteHeld & WPAD_BUTTON_RIGHT)
            buttons |= TH_BUTTON_UP;
        if (wiimoteHeld & WPAD_BUTTON_LEFT)
            buttons |= TH_BUTTON_DOWN;
        if (wiimoteHeld & WPAD_BUTTON_UP)
            buttons |= TH_BUTTON_LEFT;
        if (wiimoteHeld & WPAD_BUTTON_DOWN)
            buttons |= TH_BUTTON_RIGHT;
        if (wiimoteHeld & WPAD_BUTTON_2)
            buttons |= TH_BUTTON_SHOOT;
        if (wiimoteHeld & WPAD_BUTTON_1)
            buttons |= TH_BUTTON_FOCUS;
        if (wiimoteHeld & WPAD_BUTTON_A)
            buttons |= TH_BUTTON_BOMB;
        if (wiimoteHeld & WPAD_BUTTON_B)
            buttons |= TH_BUTTON_SKIP;
        if (wiimoteHeld & WPAD_BUTTON_PLUS)
            buttons |= TH_BUTTON_MENU;
        if (wiimoteHeld & WPAD_BUTTON_HOME)
            buttons |= TH_BUTTON_HOME;

        if (gcHeld & PAD_BUTTON_UP)
            buttons |= TH_BUTTON_UP;
        if (gcHeld & PAD_BUTTON_DOWN)
            buttons |= TH_BUTTON_DOWN;
        if (gcHeld & PAD_BUTTON_LEFT)
            buttons |= TH_BUTTON_LEFT;
        if (gcHeld & PAD_BUTTON_RIGHT)
            buttons |= TH_BUTTON_RIGHT;
        if (gcHeld & PAD_BUTTON_A)
            buttons |= TH_BUTTON_SHOOT;
        if (gcHeld & PAD_BUTTON_B)
            buttons |= TH_BUTTON_BOMB;
        if (gcHeld & PAD_TRIGGER_R)
            buttons |= TH_BUTTON_FOCUS;
        if (gcHeld & PAD_TRIGGER_L)
            buttons |= TH_BUTTON_SKIP;
        if (gcHeld & PAD_BUTTON_START)
            buttons |= TH_BUTTON_MENU;
    }

    if (WPAD_Probe(0, NULL) == WPAD_ERR_NONE)
    {
        expansion_t exp;
        WPAD_Expansion(0, &exp);
        if (exp.type == WPAD_EXP_NUNCHUK)
        {
            s8 nx = exp.nunchuk.js.pos.x - exp.nunchuk.js.center.x;
            s8 ny = exp.nunchuk.js.pos.y - exp.nunchuk.js.center.y;
            const s8 DEAD_ZONE = 20;
            if (nx > DEAD_ZONE)
                buttons |= TH_BUTTON_RIGHT;
            if (nx < -DEAD_ZONE)
                buttons |= TH_BUTTON_LEFT;
            if (ny > DEAD_ZONE)
                buttons |= TH_BUTTON_UP;
            if (ny < -DEAD_ZONE)
                buttons |= TH_BUTTON_DOWN;
        }
    }
    else
    {
        s8 gx = PAD_StickX(0);
        s8 gy = PAD_StickY(0);
        const s8 DEAD_ZONE = 20;
        if (gx > DEAD_ZONE)
            buttons |= TH_BUTTON_RIGHT;
        if (gx < -DEAD_ZONE)
            buttons |= TH_BUTTON_LEFT;
        if (gy > DEAD_ZONE)
            buttons |= TH_BUTTON_UP;
        if (gy < -DEAD_ZONE)
            buttons |= TH_BUTTON_DOWN;
    }

    return buttons;
}

const u8 *Controller::GetControllerState()
{

    static u8 dummy[32] = {0};
    return dummy;
}

#else

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>

static u16 g_FocusButtonConflictState;
static u8 *keyboardState;

u16 Controller::GetJoystickCaps(void)
{

    return 0;
}

#define JOYSTICK_MIDPOINT(min, max) ((min + max) / 2)
#define JOYSTICK_BUTTON_PRESSED(button, x, y) (x > y ? button : 0)
#define JOYSTICK_BUTTON_PRESSED_INVERT(button, x, y) (x < y ? button : 0)
#define KEYBOARD_KEY_PRESSED(button, x) keyboardState[x] ? button : 0

u16 Controller::GetControllerInput(u16 buttons)
{

    u32 shootPressed;

    i16 stickX;
    i16 stickY;

    if (g_Supervisor.gameController != NULL)
    {

        shootPressed = SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.shootButton,
                                                     TH_BUTTON_SHOOT, g_Supervisor.gameController);

        if (g_ControllerMapping.shootButton != g_ControllerMapping.focusButton)
        {
            SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.focusButton, TH_BUTTON_FOCUS,
                                          g_Supervisor.gameController);
        }
        else
        {
            if (shootPressed != 0)
            {
                if (g_FocusButtonConflictState < 16)
                {
                    g_FocusButtonConflictState++;
                }

                if (g_FocusButtonConflictState >= 8)
                {
                    buttons |= TH_BUTTON_FOCUS;
                }
            }
            else
            {
                if (g_FocusButtonConflictState > 8)
                {
                    g_FocusButtonConflictState -= 8;
                }
                else
                {
                    g_FocusButtonConflictState = 0;
                }
            }
        }

        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.bombButton, TH_BUTTON_BOMB,
                                      g_Supervisor.gameController);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.menuButton, TH_BUTTON_MENU,
                                      g_Supervisor.gameController);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.upButton, TH_BUTTON_UP,
                                      g_Supervisor.gameController);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.downButton, TH_BUTTON_DOWN,
                                      g_Supervisor.gameController);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.leftButton, TH_BUTTON_LEFT,
                                      g_Supervisor.gameController);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.rightButton, TH_BUTTON_RIGHT,
                                      g_Supervisor.gameController);
        SetButtonFromControllerInputs(&buttons, g_Supervisor.cfg.controllerMapping.skipButton, TH_BUTTON_SKIP,
                                      g_Supervisor.gameController);

        if (SDL_GameControllerHasAxis(g_Supervisor.gameController, SDL_CONTROLLER_AXIS_LEFTX) &&
            SDL_GameControllerHasAxis(g_Supervisor.gameController, SDL_CONTROLLER_AXIS_LEFTY))
        {
            stickX = SDL_GameControllerGetAxis(g_Supervisor.gameController, SDL_CONTROLLER_AXIS_LEFTX);
            stickY = SDL_GameControllerGetAxis(g_Supervisor.gameController, SDL_CONTROLLER_AXIS_LEFTY);
        }
        else if (SDL_GameControllerHasAxis(g_Supervisor.gameController, SDL_CONTROLLER_AXIS_RIGHTX) &&
                 SDL_GameControllerHasAxis(g_Supervisor.gameController, SDL_CONTROLLER_AXIS_RIGHTY))
        {
            stickX = SDL_GameControllerGetAxis(g_Supervisor.gameController, SDL_CONTROLLER_AXIS_RIGHTX);
            stickY = SDL_GameControllerGetAxis(g_Supervisor.gameController, SDL_CONTROLLER_AXIS_RIGHTY);
        }
        else
        {
            return buttons;
        }

        buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_RIGHT, stickX, JOYSTICK_MIDPOINT(0, INT16_MAX));
        buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_LEFT, -stickX, JOYSTICK_MIDPOINT(0, INT16_MAX));

        buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_DOWN, stickY, JOYSTICK_MIDPOINT(0, INT16_MAX));
        buttons |= JOYSTICK_BUTTON_PRESSED(TH_BUTTON_UP, -stickY, JOYSTICK_MIDPOINT(0, INT16_MAX));

    }

    return buttons;
}

u32 Controller::SetButtonFromDirectInputJoystate(u16 *outButtons, i16 controllerButtonToTest,
                                                 enum TouhouButton touhouButton, const u8 *inputButtons)
{
    if (controllerButtonToTest < 0)
    {
        return 0;
    }

    *outButtons |= (inputButtons[controllerButtonToTest] & 0x80 ? touhouButton & 0xFFFF : 0);

    return inputButtons[controllerButtonToTest] & 0x80 ? touhouButton & 0xFFFF : 0;
}

u32 Controller::SetButtonFromControllerInputs(u16 *outButtons, i16 controllerButtonToTest,
                                              enum TouhouButton touhouButton, SDL_GameController *controller)
{
    u8 pressed;

    if (controllerButtonToTest < 0)
    {
        return 0;
    }

    pressed = SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)controllerButtonToTest);

    *outButtons |= pressed ? touhouButton & 0xFFFF : 0;

    return pressed ? touhouButton & 0xFFFF : 0;
}

static u8 g_ControllerData[SDL_CONTROLLER_BUTTON_MAX];

const u8 *Controller::GetControllerState()
{

    if (g_Supervisor.gameController != NULL)
    {
        memset(&g_ControllerData, 0, sizeof(g_ControllerData));

        SDL_Joystick *joystick = SDL_GameControllerGetJoystick(g_Supervisor.gameController);

        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
        {
            if (SDL_GameControllerGetButton(g_Supervisor.gameController, (SDL_GameControllerButton)i))
            {
                g_ControllerData[i] = 0x80;
            }
        }
    }

    return g_ControllerData;

}

u16 Controller::GetInput(void)
{
    u16 buttons = 0;

    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, SDL_SCANCODE_UP);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, SDL_SCANCODE_DOWN);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, SDL_SCANCODE_LEFT);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, SDL_SCANCODE_RIGHT);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP, SDL_SCANCODE_KP_8);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN, SDL_SCANCODE_KP_2);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_LEFT, SDL_SCANCODE_KP_4);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_RIGHT, SDL_SCANCODE_KP_6);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_LEFT, SDL_SCANCODE_KP_7);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_UP_RIGHT, SDL_SCANCODE_KP_9);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_LEFT, SDL_SCANCODE_KP_1);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_DOWN_RIGHT, SDL_SCANCODE_KP_3);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_HOME, SDL_SCANCODE_HOME);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SHOOT, SDL_SCANCODE_Z);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_BOMB, SDL_SCANCODE_X);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, SDL_SCANCODE_LSHIFT);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_FOCUS, SDL_SCANCODE_RSHIFT);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_MENU, SDL_SCANCODE_ESCAPE);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, SDL_SCANCODE_LCTRL);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_SKIP, SDL_SCANCODE_RCTRL);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_Q, SDL_SCANCODE_Q);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_S, SDL_SCANCODE_S);
    buttons |= KEYBOARD_KEY_PRESSED(TH_BUTTON_ENTER, SDL_SCANCODE_RETURN);

    return Controller::GetControllerInput(buttons);
}

void Controller::ResetKeyboard(void)
{
    keyboardState = (u8 *)SDL_GetKeyboardState(NULL);

    SDL_StartTextInput();
    SDL_StopTextInput();
}

#endif
