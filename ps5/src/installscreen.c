/*
 * Burnout 3 for the PlayStation 5 - a picture while the first start sets up
 * the game data.
 *
 * The install (ps5/src/firstrun.c) runs before the game, so before anything
 * draws. This opens the console's display itself, as PS5_Vulkan's demo
 * renderer does (two 1920x1080 buffers in direct memory the display reads,
 * written by the CPU in the display's 64 KiB tile layout, its 5x7 font), and
 * redraws ten times a second on a thread of its own: the bar, the share done,
 * the time left and a clock, so the screen moves even while the installer is
 * between files. Stopped, it hands the display back for the game's own.
 *
 * The tile layout and the font are PS5_Vulkan's (src/demo_renderer.cpp).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "platform.h"

size_t sceKernelGetDirectMemorySize(void);
int sceKernelAllocateDirectMemory(int64_t start, int64_t end, size_t length, size_t alignment,
                                  int type, int64_t *physical);
int sceKernelMapDirectMemory(void **address, size_t length, int protection, int flags,
                             int64_t physical, size_t alignment);
int sceKernelMunmap(void *address, size_t length);
int sceKernelReleaseDirectMemory(int64_t start, size_t length);
int sceVideoOutOpen(int32_t user, int32_t bus, int32_t index, const void *parameter);
int sceVideoOutClose(int32_t handle);
int sceVideoOutSetFlipRate(int32_t handle, int32_t rate);
void sceVideoOutSetBufferAttribute2(void *attribute, uint64_t format, uint32_t tiling, uint32_t width,
                                    uint32_t height, uint64_t option, uint32_t dcc, uint64_t clear);
int sceVideoOutRegisterBuffers2(int32_t handle, int32_t set, int32_t start, void *buffers,
                                int32_t count, void *attribute, int32_t category, void *option);
int sceVideoOutUnregisterBuffers(int32_t handle, int32_t set);
int sceVideoOutSubmitFlip(int32_t handle, int32_t index, uint32_t mode, int64_t argument);

#define WIDTH 1920u
#define HEIGHT 1080u
#define FRAME_BYTES 0x1000000u
#define MEMORY_BYTES (FRAME_BYTES * 2u)
#define MEMORY_ALIGNMENT 0x200000u
#define MEMORY_WC_GARLIC 3
#define MAP_CPU_GPU 0x33
#define FORMAT_RGBA8_SRGB UINT64_C(0x8000000022000000)

/* 0xAABBGGRR */
#define C_BACK 0xff401812u
#define C_PANEL 0xff5a2a1eu
#define C_RED 0xff3a2ae6u
#define C_WHITE 0xffffffffu
#define C_GREY 0xffc8bcb4u
#define C_GOLD 0xff40c8ffu

struct buffer {
   void *data, *metadata, *reserved[2];
};

static struct {
   int video;
   int64_t physical;
   uint8_t *memory;
   pthread_t thread;
   volatile int stop;
   volatile double fraction;
   time_t started;
   int running;
} s = { .video = -1 };

static size_t
tiled(unsigned x, unsigned y)
{
   const uint32_t offset = ((y << 4) & 0x70u) ^ ((y << 5) & 0xf00u) ^ ((y << 9) & 0x1000u) ^
                           ((y << 8) & 0x4000u) ^ ((x << 2) & 0xcu) ^ ((x << 5) & 0x380u) ^
                           ((x << 4) & 0x400u) ^ ((x << 6) & 0x800u) ^ ((x << 9) & 0xa000u);
   const uint32_t blocks_per_row = (WIDTH + 127u) >> 7;
   return ((size_t)((y >> 7) * blocks_per_row + (x >> 7)) << 16) + offset;
}

static void
fill(uint8_t *frame, unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color)
{
   unsigned right = x + w > WIDTH ? WIDTH : x + w, bottom = y + h > HEIGHT ? HEIGHT : y + h;
   for (unsigned row = y; row < bottom; row++)
      for (unsigned col = x; col < right; col++)
         *(uint32_t *)(frame + tiled(col, row)) = color;
}

/* 5x7, rows top first, bit 4 the leftmost column */
static const struct {
   char c;
   uint8_t rows[7];
} font[] = {
   { ' ', { 0, 0, 0, 0, 0, 0, 0 } },        { '0', { 14, 17, 19, 21, 25, 17, 14 } },
   { '1', { 4, 12, 4, 4, 4, 4, 14 } },      { '2', { 14, 17, 1, 2, 4, 8, 31 } },
   { '3', { 30, 1, 1, 14, 1, 1, 30 } },     { '4', { 2, 6, 10, 18, 31, 2, 2 } },
   { '5', { 31, 16, 16, 30, 1, 1, 30 } },   { '6', { 14, 16, 16, 30, 17, 17, 14 } },
   { '7', { 31, 1, 2, 4, 8, 8, 8 } },       { '8', { 14, 17, 17, 14, 17, 17, 14 } },
   { '9', { 14, 17, 17, 15, 1, 1, 14 } },   { 'A', { 14, 17, 17, 31, 17, 17, 17 } },
   { 'B', { 30, 17, 17, 30, 17, 17, 30 } }, { 'C', { 14, 17, 16, 16, 16, 17, 14 } },
   { 'D', { 30, 17, 17, 17, 17, 17, 30 } }, { 'E', { 31, 16, 16, 30, 16, 16, 31 } },
   { 'F', { 31, 16, 16, 30, 16, 16, 16 } }, { 'G', { 14, 17, 16, 23, 17, 17, 14 } },
   { 'H', { 17, 17, 17, 31, 17, 17, 17 } }, { 'I', { 31, 4, 4, 4, 4, 4, 31 } },
   { 'J', { 7, 2, 2, 2, 18, 18, 12 } },     { 'K', { 17, 18, 20, 24, 20, 18, 17 } },
   { 'L', { 16, 16, 16, 16, 16, 16, 31 } }, { 'M', { 17, 27, 21, 21, 17, 17, 17 } },
   { 'N', { 17, 25, 21, 19, 17, 17, 17 } }, { 'O', { 14, 17, 17, 17, 17, 17, 14 } },
   { 'P', { 30, 17, 17, 30, 16, 16, 16 } }, { 'Q', { 14, 17, 17, 17, 21, 18, 13 } },
   { 'R', { 30, 17, 17, 30, 20, 18, 17 } }, { 'S', { 15, 16, 16, 14, 1, 1, 30 } },
   { 'T', { 31, 4, 4, 4, 4, 4, 4 } },       { 'U', { 17, 17, 17, 17, 17, 17, 14 } },
   { 'V', { 17, 17, 17, 17, 17, 10, 4 } },  { 'W', { 17, 17, 17, 21, 21, 21, 10 } },
   { 'X', { 17, 17, 10, 4, 10, 17, 17 } },  { 'Y', { 17, 17, 10, 4, 4, 4, 4 } },
   { 'Z', { 31, 1, 2, 4, 8, 16, 31 } },     { '%', { 24, 25, 2, 4, 8, 19, 3 } },
   { ':', { 0, 12, 12, 0, 12, 12, 0 } },    { '.', { 0, 0, 0, 0, 0, 12, 12 } },
   { ',', { 0, 0, 0, 0, 12, 4, 8 } },       { '-', { 0, 0, 0, 31, 0, 0, 0 } },
   { '\'', { 4, 4, 8, 0, 0, 0, 0 } },       { '!', { 4, 4, 4, 4, 4, 0, 4 } },
};

static const uint8_t *
glyph(char c)
{
   for (size_t i = 0; i < sizeof font / sizeof font[0]; i++)
      if (font[i].c == c)
         return font[i].rows;
   return font[0].rows;
}

static void
text(uint8_t *frame, unsigned x, unsigned y, const char *string, unsigned scale, uint32_t color)
{
   for (; *string; string++, x += 6 * scale) {
      const uint8_t *rows = glyph(*string);
      for (unsigned r = 0; r < 7; r++)
         for (unsigned c = 0; c < 5; c++)
            if (rows[r] & (16u >> c))
               fill(frame, x + c * scale, y + r * scale, scale, scale, color);
   }
}

static void
centred(uint8_t *frame, unsigned y, const char *string, unsigned scale, uint32_t color)
{
   unsigned width = (unsigned)strlen(string) * 6 * scale - scale;
   text(frame, width < WIDTH ? (WIDTH - width) / 2 : 0, y, string, scale, color);
}

/* Seconds into a first start on a PS5 at each tenth of the bytes, measured
 * from an image in dump/ (2026-10-20). The console's storage takes the first
 * 80% quickly and then slows right down, so the share of the bytes says
 * little about the time: the bar and the time left follow these times, scaled
 * by how this console is doing against them. */
static const double k_seconds[11] = { 0, 4, 8, 12, 17, 21, 30, 45, 70, 165, 351 };

static double
expected(double fraction)
{
   double at = (fraction < 0 ? 0 : fraction > 1 ? 1 : fraction) * 10.0;
   int i = at >= 10.0 ? 9 : (int)at;
   return k_seconds[i] + (k_seconds[i + 1] - k_seconds[i]) * (at - i);
}

static void
draw(uint8_t *frame, double fraction, long elapsed)
{
   char line[64];
   const unsigned bar_x = 360, bar_y = 600, bar_w = 1200, bar_h = 56;
   const double total = k_seconds[10];
   double so_far = expected(fraction), share = so_far / total, pace = 1.0;
   long left;

   /* this console against the measured one, once there is enough to go on */
   if (so_far >= 8.0 && elapsed > 0) {
      pace = elapsed / so_far;
      pace = pace < 0.5 ? 0.5 : pace > 3.0 ? 3.0 : pace;
   }
   left = (long)((total - so_far) * pace);
   if (so_far < 8.0 && left < (long)total - elapsed)
      left = (long)total - elapsed;
   if (left < 0)
      left = 0;

   fill(frame, 0, 0, WIDTH, HEIGHT, C_BACK);
   centred(frame, 170, "BURNOUT 3", 16, C_RED);
   centred(frame, 360, "FIRST START: SETTING UP THE GAME DATA", 5, C_WHITE);
   centred(frame, 430, "THIS ONLY HAPPENS ONCE", 5, C_GOLD);
   fill(frame, bar_x - 6, bar_y - 6, bar_w + 12, bar_h + 12, C_WHITE);
   fill(frame, bar_x, bar_y, bar_w, bar_h, C_PANEL);
   if (share > 0)
      fill(frame, bar_x, bar_y, (unsigned)(bar_w * (share > 1 ? 1 : share)), bar_h, C_RED);
   snprintf(line, sizeof line, "%d%%", (int)(share * 100.0));
   centred(frame, 700, line, 7, C_WHITE);
   if (left >= 90)
      snprintf(line, sizeof line, "ABOUT %ld MIN LEFT", (left + 30) / 60);
   else if (left > 0)
      snprintf(line, sizeof line, "ABOUT A MINUTE LEFT");
   else
      snprintf(line, sizeof line, "NEARLY DONE");
   centred(frame, 790, line, 4, C_GREY);
   snprintf(line, sizeof line, "TIME %ld:%02ld", elapsed / 60, elapsed % 60);
   centred(frame, 850, line, 4, C_GREY);
   centred(frame, 980, "PLEASE DON'T CLOSE THE GAME", 3, C_GREY);
}

static void
flush(void *address, size_t length)
{
   for (uint8_t *at = address, *end = at + length; at < end; at += 64)
      __asm__ volatile("clflush (%0)" : : "r"(at) : "memory");
   __asm__ volatile("mfence" ::: "memory");
}

static void *
redraw(void *unused)
{
   int index = 0;

   (void)unused;
   while (!s.stop) {
      uint8_t *frame = s.memory + (size_t)index * FRAME_BYTES;
      draw(frame, s.fraction, (long)(time(NULL) - s.started));
      flush(frame, FRAME_BYTES);
      sceVideoOutSubmitFlip(s.video, index, 1, index + 1);
      index ^= 1;
      usleep(100000);
   }
   return NULL;
}

void
install_screen_progress(double fraction)
{
   s.fraction = fraction;
}

int
install_screen_start(void)
{
   struct buffer buffers[2];
   uint8_t attribute[80];
   size_t pool = sceKernelGetDirectMemorySize();
   void *mapped = NULL;

   if (s.running)
      return 0;
   if (pool < MEMORY_BYTES)
      return -1;
   if ((s.video = sceVideoOutOpen(0xff, 0, 0, NULL)) < 0) {
      say("[firstrun] screen: the display could not be opened (0x%08x)", (unsigned)s.video);
      return -1;
   }
   if (sceKernelAllocateDirectMemory(0, (int64_t)pool, MEMORY_BYTES, MEMORY_ALIGNMENT, MEMORY_WC_GARLIC,
                                     &s.physical) < 0 ||
       sceKernelMapDirectMemory(&mapped, MEMORY_BYTES, MAP_CPU_GPU, 0, s.physical, MEMORY_ALIGNMENT) < 0) {
      say("[firstrun] screen: no memory for it");
      sceVideoOutClose(s.video);
      s.video = -1;
      return -1;
   }
   s.memory = mapped;
   s.started = time(NULL);
   s.fraction = 0;
   for (int i = 0; i < 2; i++) {
      draw(s.memory + (size_t)i * FRAME_BYTES, 0, 0);
      buffers[i] = (struct buffer){ s.memory + (size_t)i * FRAME_BYTES, NULL, { NULL, NULL } };
   }
   flush(s.memory, MEMORY_BYTES);
   memset(attribute, 0, sizeof attribute);
   sceVideoOutSetFlipRate(s.video, 0);
   sceVideoOutSetBufferAttribute2(attribute, FORMAT_RGBA8_SRGB, 0, WIDTH, HEIGHT, 0, 0, 0);
   if (sceVideoOutRegisterBuffers2(s.video, 0, 0, buffers, 2, attribute, 0, NULL) < 0) {
      say("[firstrun] screen: the display took no buffers");
      sceKernelMunmap(s.memory, MEMORY_BYTES);
      sceKernelReleaseDirectMemory(s.physical, MEMORY_BYTES);
      sceVideoOutClose(s.video);
      s.video = -1;
      return -1;
   }
   s.stop = 0;
   if (pthread_create(&s.thread, NULL, redraw, NULL) != 0) {
      sceVideoOutSubmitFlip(s.video, 0, 1, 1);
      s.thread = 0;
   }
   s.running = 1;
   say("[firstrun] screen: up");
   return 0;
}

void
install_screen_stop(void)
{
   if (!s.running)
      return;
   s.stop = 1;
   if (s.thread)
      pthread_join(s.thread, NULL);
   usleep(50000);
   sceVideoOutUnregisterBuffers(s.video, 0);
   sceVideoOutClose(s.video);
   sceKernelMunmap(s.memory, MEMORY_BYTES);
   sceKernelReleaseDirectMemory(s.physical, MEMORY_BYTES);
   s.video = -1;
   s.running = 0;
   say("[firstrun] screen: down");
}
