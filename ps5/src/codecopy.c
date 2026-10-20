/*
 * Burnout 3 for the PlayStation 5 - the title's code, readable.
 *
 * The toolkit services some of the Xbox's device registers (the sound chip, the
 * USB pads) by letting an access fault and decoding the instruction that made
 * it (vendor/xboxrecomp/src/platform/mmio_decode.h). The console's code is
 * execute-only: the decoder may not read it. tools/link-title.sh writes the
 * code section as a file beside eboot.bin (code.bin), and code.txt with the
 * section's link address and this file's anchor's; xr_code_bytes answers a pc
 * from that copy, read whole at start-up (never in the signal handler).
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform.h"

#define TITLE_DIR "/app0"

static const uint8_t *code;
static uintptr_t code_lo, code_hi;

/* A function whose link address code.txt has: the load's displacement */
__attribute__((noinline, used)) void
ps5_code_anchor(void)
{
   __asm__ __volatile__("");
}

const uint8_t *
xr_code_bytes(uintptr_t pc)
{
   /* 15 bytes: the longest x86 instruction */
   if (code == NULL || pc < code_lo || pc + 15 > code_hi)
      return NULL;
   return code + (pc - code_lo);
}

__attribute__((constructor)) static void
code_copy_load(void)
{
   unsigned long long text = 0, anchor = 0;
   long size;
   uint8_t *bytes;
   FILE *file = fopen(TITLE_DIR "/code.txt", "r");

   if (file == NULL || fscanf(file, "%llx %llx", &text, &anchor) != 2 || !anchor) {
      if (file)
         fclose(file);
      say("code copy: no " TITLE_DIR "/code.txt; trapped device registers are not decoded");
      return;
   }
   fclose(file);
   if ((file = fopen(TITLE_DIR "/code.bin", "rb")) == NULL) {
      say("code copy: no " TITLE_DIR "/code.bin");
      return;
   }
   fseek(file, 0, SEEK_END);
   size = ftell(file);
   fseek(file, 0, SEEK_SET);
   bytes = size > 0 ? malloc((size_t)size) : NULL;
   if (bytes == NULL || fread(bytes, 1, (size_t)size, file) != (size_t)size) {
      fclose(file);
      free(bytes);
      say("code copy: code.bin could not be read");
      return;
   }
   fclose(file);
   code_lo = (uintptr_t)text + ((uintptr_t)&ps5_code_anchor - (uintptr_t)anchor);
   code_hi = code_lo + (uintptr_t)size;
   code = bytes;
   say("code copy: %ld KiB at %p", size >> 10, (void *)code_lo);
}
