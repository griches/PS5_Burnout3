/*
 * Doom 3 BFG for the PlayStation 5 - SDL2's joystick driver for the console.
 *
 * Up to four controllers, one per signed-in user, read through the title's
 * platform layer (ps5/src/platform.h). Each is a game controller with a
 * mapping of its own, and reports itself as a DualSense so a title shows the
 * right glyphs.
 *
 * After the SDL3 driver of the DevilutionX port.
 * SPDX-License-Identifier: Zlib
 */
#include "SDL_internal.h"

#ifdef SDL_JOYSTICK_PS5

#include "SDL_events.h"
#include "SDL_joystick.h"
#include "joystick/SDL_sysjoystick.h"
#include "joystick/SDL_joystick_c.h"

#include "platform.h"

/* Joystick buttons, in this order */
static const uint32_t ps5_buttons[] = {
    PAD_CROSS, PAD_CIRCLE, PAD_SQUARE, PAD_TRIANGLE,
    PAD_L1, PAD_R1, PAD_L3, PAD_R3,
    PAD_OPTIONS, PAD_TOUCH_PAD,
    PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT,
};
#define PS5_BUTTONS ((int)SDL_arraysize(ps5_buttons))
#define PS5_AXES    6 /* left x, left y, right x, right y, L2, R2 */

static SDL_bool ps5_pad_opened;
static uint32_t ps5_connected;                    /* a bit a player, as SDL knows them */
static SDL_JoystickID ps5_player_id[PAD_PLAYERS]; /* player -> SDL's instance */
static int ps5_device_player[PAD_PLAYERS];        /* device index -> player */
static int ps5_devices;

static void PS5_ListDevices(void)
{
    int player;

    ps5_devices = 0;
    for (player = 0; player < PAD_PLAYERS; player++) {
        if (ps5_connected & (1u << player)) {
            ps5_device_player[ps5_devices++] = player;
        }
    }
}

static int PS5_PlayerOf(SDL_Joystick *joystick)
{
    int player;

    for (player = 0; player < PAD_PLAYERS; player++) {
        if ((ps5_connected & (1u << player)) && ps5_player_id[player] == joystick->instance_id) {
            return player;
        }
    }
    return -1;
}

static void PS5_JoystickDetect(void)
{
    struct pad pad;
    uint32_t now;
    int player;

    if (!ps5_pad_opened) {
        return;
    }
    pad_poll(&pad);
    now = pad_players();
    for (player = 0; player < PAD_PLAYERS; player++) {
        const uint32_t bit = 1u << player;
        if ((now & bit) && !(ps5_connected & bit)) {
            ps5_connected |= bit;
            ps5_player_id[player] = SDL_GetNextJoystickInstanceID();
            PS5_ListDevices();
            say("pad: player %d is SDL joystick %d", player, (int)ps5_player_id[player]);
            SDL_PrivateJoystickAdded(ps5_player_id[player]);
        } else if (!(now & bit) && (ps5_connected & bit)) {
            ps5_connected &= ~bit;
            PS5_ListDevices();
            SDL_PrivateJoystickRemoved(ps5_player_id[player]);
        }
    }
}

static int PS5_JoystickInit(void)
{
    ps5_pad_opened = pad_open() ? SDL_TRUE : SDL_FALSE;
    PS5_JoystickDetect();
    return 0;
}

static int PS5_JoystickGetCount(void)
{
    return ps5_devices;
}

static const char *PS5_JoystickGetDeviceName(int device_index)
{
    return "DualSense Wireless Controller";
}

static const char *PS5_JoystickGetDevicePath(int device_index)
{
    return NULL;
}

static int PS5_JoystickGetDeviceSteamVirtualGamepadSlot(int device_index)
{
    return -1;
}

static int PS5_JoystickGetDevicePlayerIndex(int device_index)
{
    return ps5_device_player[device_index];
}

static void PS5_JoystickSetDevicePlayerIndex(int device_index, int player_index)
{
}

static SDL_JoystickGUID PS5_JoystickGetDeviceGUID(int device_index)
{
    /* Sony's DualSense, with a driver signature no mapping database entry has:
     * the mapping is this driver's */
    return SDL_CreateJoystickGUID(SDL_HARDWARE_BUS_BLUETOOTH, 0x054c, 0x0ce6, 0, NULL,
                                  PS5_JoystickGetDeviceName(device_index), 'p', 0);
}

static SDL_JoystickID PS5_JoystickGetDeviceInstanceID(int device_index)
{
    return ps5_player_id[ps5_device_player[device_index]];
}

static int PS5_JoystickOpen(SDL_Joystick *joystick, int device_index)
{
    joystick->nbuttons = PS5_BUTTONS;
    joystick->naxes = PS5_AXES;
    joystick->nhats = 0;
    say("pad: SDL opened joystick %d", (int)joystick->instance_id);
    return 0;
}

static int PS5_JoystickRumble(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    const int player = PS5_PlayerOf(joystick);
    if (player < 0) {
        return SDL_SetError("the controller is gone");
    }
    pad_player_vibrate(player, low_frequency_rumble / 65535.0f, high_frequency_rumble / 65535.0f);
    return 0;
}

static int PS5_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left, Uint16 right)
{
    return SDL_Unsupported();
}

static Uint32 PS5_JoystickGetCapabilities(SDL_Joystick *joystick)
{
    return SDL_JOYCAP_RUMBLE | SDL_JOYCAP_LED;
}

static int PS5_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
    const int player = PS5_PlayerOf(joystick);
    if (player < 0) {
        return SDL_SetError("the controller is gone");
    }
    pad_player_light_bar(player, red, green, blue);
    return 0;
}

static int PS5_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
    return SDL_Unsupported();
}

static int PS5_JoystickSetSensorsEnabled(SDL_Joystick *joystick, SDL_bool enabled)
{
    return SDL_Unsupported();
}

static Sint16 PS5_Stick(float value)
{
    value = SDL_clamp(value, -1.0f, 1.0f);
    return (Sint16)(value * 32767.0f);
}

static Sint16 PS5_Trigger(float value)
{
    value = SDL_clamp(value, 0.0f, 1.0f);
    return (Sint16)(value * 65535.0f - 32768.0f);
}

static void PS5_JoystickUpdate(SDL_Joystick *joystick)
{
    const int player = PS5_PlayerOf(joystick);
    struct pad pad;
    int i;

    /* PS5_JoystickDetect polled the pads just before */
    if (player < 0 || !pad_player(player, &pad) || (pad.held & PAD_INTERCEPTED)) {
        return;
    }
    if (pad.pressed && SDL_getenv("SDL_PS5_PAD_LOG")) {
        say("pad: player %d pressed 0x%x", player, (unsigned)pad.pressed);
    }
    for (i = 0; i < PS5_BUTTONS; i++) {
        SDL_PrivateJoystickButton(joystick, (Uint8)i, (pad.held & ps5_buttons[i]) ? SDL_PRESSED : SDL_RELEASED);
    }
    SDL_PrivateJoystickAxis(joystick, 0, PS5_Stick(pad.left_x));
    SDL_PrivateJoystickAxis(joystick, 1, PS5_Stick(pad.left_y));
    SDL_PrivateJoystickAxis(joystick, 2, PS5_Stick(pad.right_x));
    SDL_PrivateJoystickAxis(joystick, 3, PS5_Stick(pad.right_y));
    SDL_PrivateJoystickAxis(joystick, 4, PS5_Trigger(pad.l2));
    SDL_PrivateJoystickAxis(joystick, 5, PS5_Trigger(pad.r2));
}

static void PS5_JoystickClose(SDL_Joystick *joystick)
{
    const int player = PS5_PlayerOf(joystick);
    if (player >= 0) {
        pad_player_vibrate(player, 0.0f, 0.0f);
    }
}

static void PS5_JoystickQuit(void)
{
}

static SDL_bool PS5_JoystickGetGamepadMapping(int device_index, SDL_GamepadMapping *out)
{
    SDL_zerop(out);
#define PS5_MAP(name, kind_, target_) out->name.kind = kind_, out->name.target = target_
    PS5_MAP(a, EMappingKind_Button, 0);
    PS5_MAP(b, EMappingKind_Button, 1);
    PS5_MAP(x, EMappingKind_Button, 2);
    PS5_MAP(y, EMappingKind_Button, 3);
    PS5_MAP(leftshoulder, EMappingKind_Button, 4);
    PS5_MAP(rightshoulder, EMappingKind_Button, 5);
    PS5_MAP(leftstick, EMappingKind_Button, 6);
    PS5_MAP(rightstick, EMappingKind_Button, 7);
    PS5_MAP(start, EMappingKind_Button, 8);
    /* the pad's CREATE button is the shell's: the touch pad's click is "back" */
    PS5_MAP(back, EMappingKind_Button, 9);
    PS5_MAP(dpup, EMappingKind_Button, 10);
    PS5_MAP(dpdown, EMappingKind_Button, 11);
    PS5_MAP(dpleft, EMappingKind_Button, 12);
    PS5_MAP(dpright, EMappingKind_Button, 13);
    PS5_MAP(leftx, EMappingKind_Axis, 0);
    PS5_MAP(lefty, EMappingKind_Axis, 1);
    PS5_MAP(rightx, EMappingKind_Axis, 2);
    PS5_MAP(righty, EMappingKind_Axis, 3);
    PS5_MAP(lefttrigger, EMappingKind_Axis, 4);
    PS5_MAP(righttrigger, EMappingKind_Axis, 5);
#undef PS5_MAP
    return SDL_TRUE;
}

SDL_JoystickDriver SDL_PS5_JoystickDriver = {
    PS5_JoystickInit,
    PS5_JoystickGetCount,
    PS5_JoystickDetect,
    PS5_JoystickGetDeviceName,
    PS5_JoystickGetDevicePath,
    PS5_JoystickGetDeviceSteamVirtualGamepadSlot,
    PS5_JoystickGetDevicePlayerIndex,
    PS5_JoystickSetDevicePlayerIndex,
    PS5_JoystickGetDeviceGUID,
    PS5_JoystickGetDeviceInstanceID,
    PS5_JoystickOpen,
    PS5_JoystickRumble,
    PS5_JoystickRumbleTriggers,
    PS5_JoystickGetCapabilities,
    PS5_JoystickSetLED,
    PS5_JoystickSendEffect,
    PS5_JoystickSetSensorsEnabled,
    PS5_JoystickUpdate,
    PS5_JoystickClose,
    PS5_JoystickQuit,
    PS5_JoystickGetGamepadMapping,
};

#endif /* SDL_JOYSTICK_PS5 */
