/*
 * Doom 3 BFG for the PlayStation 5 - the title's own frame, for PS5 DevLoop's grab.
 *
 * DevLoop asks by leaving /app0/devloop-shot: removing it is the check, one
 * system call that fails on every ordinary launch. The renderer then hands
 * over a frame it read back, which is written here as a new PNG (at most 1280
 * wide) in /app0/userdata/screenshots, a folder that holds nothing else.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "platform.h"

#define SHOT_REQUEST "/app0/devloop-shot"
#define SHOT_FOLDER  "/app0/userdata/screenshots"

bool
ps5_shot_requested(void)
{
   static unsigned calls;

   return ++calls % 4 == 0 && unlink(SHOT_REQUEST) == 0;
}

static void
chunk(FILE *file, const char *type, const uint8_t *data, uint32_t size)
{
   uint8_t head[8] = { size >> 24, size >> 16, size >> 8, size, type[0], type[1], type[2], type[3] };
   uint32_t crc = crc32(crc32(0, head + 4, 4), data, size);
   uint8_t tail[4] = { crc >> 24, crc >> 16, crc >> 8, crc };

   fwrite(head, 1, 8, file);
   if (size)
      fwrite(data, 1, size, file);
   fwrite(tail, 1, 4, file);
}

/* pixels: width x height, four bytes each, rows top first; bgr says which end red is at */
void
ps5_shot_write(const uint8_t *pixels, int width, int height, bool bgr)
{
   static unsigned count;
   const int step = (width + 1279) / 1280;
   const int w = width / step, h = height / step;
   const size_t raw_size = (size_t)h * (1 + (size_t)w * 3);
   uint8_t *raw = malloc(raw_size);
   uLongf packed_size = compressBound(raw_size);
   uint8_t *packed = malloc(packed_size);
   char path[128];
   FILE *file;

   if (!raw || !packed)
      goto done;
   for (int y = 0; y < h; y++) {
      const uint8_t *in = pixels + (size_t)y * step * width * 4;
      uint8_t *out = raw + (size_t)y * (1 + (size_t)w * 3);
      *out++ = 0; /* no filter */
      for (int x = 0; x < w; x++, in += (size_t)step * 4, out += 3) {
         out[0] = in[bgr ? 2 : 0];
         out[1] = in[1];
         out[2] = in[bgr ? 0 : 2];
      }
   }
   if (compress2(packed, &packed_size, raw, raw_size, 1) != Z_OK)
      goto done;
   mkdir(SHOT_FOLDER, 0777);
   snprintf(path, sizeof path, SHOT_FOLDER "/shot-%04u.png", count++);
   file = fopen(path, "wb");
   if (file) {
      const uint8_t header[13] = { w >> 24, w >> 16, w >> 8, w, h >> 24, h >> 16, h >> 8, h, 8, 2, 0, 0, 0 };
      fwrite("\x89PNG\r\n\x1a\n", 1, 8, file);
      chunk(file, "IHDR", header, sizeof header);
      chunk(file, "IDAT", packed, (uint32_t)packed_size);
      chunk(file, "IEND", NULL, 0);
      fclose(file);
      chmod(path, 0666);
      say("devloop: %s (%dx%d)", path, w, h);
   }
done:
   free(raw);
   free(packed);
}
