/*
 * Doom 3 BFG for the PlayStation 5 - SDL2's audio driver for the console.
 *
 * One playback device: an AudioOut main port, 48 kHz stereo 16-bit, a grain
 * of 256 frames. sceAudioOutOutput blocks while the console plays the grain
 * before, which paces SDL's audio thread. SDL converts what a title asks for.
 *
 * After the SDL3 driver of the DevilutionX port.
 * SPDX-License-Identifier: Zlib
 */
#include "SDL_internal.h"

#ifdef SDL_AUDIO_DRIVER_PS5

#include "SDL_audio.h"
#include "audio/SDL_audio_c.h"
#include "audio/SDL_sysaudio.h"

int sceAudioOutInit(void);
int sceAudioOutOpen(int32_t user, int32_t type, int32_t index, uint32_t grain, uint32_t rate, uint32_t format);
int sceAudioOutOutput(int32_t port, const void *samples);
int sceAudioOutClose(int32_t port);

#define PS5_AUDIO_GRAIN               256
#define PS5_AUDIO_ALREADY_INITIALISED ((int)0x8026000e)
#define PS5_AUDIO_USER_SYSTEM         0xff
#define PS5_AUDIO_PORT_MAIN           0
#define PS5_AUDIO_FORMAT_S16_STEREO   1

#define _THIS SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    int port;
    Uint8 *buffer;
};

static int PS5AUDIO_OpenDevice(_THIS, const char *devname)
{
    int init;

    this->hidden = (struct SDL_PrivateAudioData *)SDL_calloc(1, sizeof(*this->hidden));
    if (!this->hidden) {
        return SDL_OutOfMemory();
    }
    this->hidden->port = -1;

    this->spec.format = AUDIO_S16LSB;
    this->spec.channels = 2;
    this->spec.freq = 48000;
    this->spec.samples = PS5_AUDIO_GRAIN;
    SDL_CalculateAudioSpec(&this->spec);

    this->hidden->buffer = (Uint8 *)SDL_calloc(1, this->spec.size);
    if (!this->hidden->buffer) {
        return SDL_OutOfMemory();
    }

    init = sceAudioOutInit();
    if (init < 0 && init != PS5_AUDIO_ALREADY_INITIALISED) {
        return SDL_SetError("sceAudioOutInit: 0x%08x", (unsigned)init);
    }
    this->hidden->port = sceAudioOutOpen(PS5_AUDIO_USER_SYSTEM, PS5_AUDIO_PORT_MAIN, 0, PS5_AUDIO_GRAIN, 48000,
                                         PS5_AUDIO_FORMAT_S16_STEREO);
    if (this->hidden->port < 0) {
        return SDL_SetError("sceAudioOutOpen: 0x%08x", (unsigned)this->hidden->port);
    }
    return 0;
}

static void PS5AUDIO_PlayDevice(_THIS)
{
    (void)sceAudioOutOutput(this->hidden->port, this->hidden->buffer);
}

static void PS5AUDIO_WaitDevice(_THIS)
{
    /* PlayDevice blocks */
}

static Uint8 *PS5AUDIO_GetDeviceBuf(_THIS)
{
    return this->hidden->buffer;
}

static void PS5AUDIO_CloseDevice(_THIS)
{
    if (this->hidden) {
        if (this->hidden->port >= 0) {
            (void)sceAudioOutOutput(this->hidden->port, NULL); /* drain */
            (void)sceAudioOutClose(this->hidden->port);
        }
        SDL_free(this->hidden->buffer);
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static SDL_bool PS5AUDIO_Init(SDL_AudioDriverImpl *impl)
{
    impl->OpenDevice = PS5AUDIO_OpenDevice;
    impl->PlayDevice = PS5AUDIO_PlayDevice;
    impl->WaitDevice = PS5AUDIO_WaitDevice;
    impl->GetDeviceBuf = PS5AUDIO_GetDeviceBuf;
    impl->CloseDevice = PS5AUDIO_CloseDevice;
    impl->OnlyHasDefaultOutputDevice = SDL_TRUE;
    return SDL_TRUE;
}

AudioBootStrap PS5AUDIO_bootstrap = {
    "ps5", "PlayStation 5 audio driver", PS5AUDIO_Init, SDL_FALSE
};

#endif /* SDL_AUDIO_DRIVER_PS5 */
