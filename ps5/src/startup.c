/*
 * Burnout 3 for the PlayStation 5 - what runs before the game's main.
 *
 * The game's main is Takedawn's (vendor/takedawn/src/game/main.c), so the
 * console's start-up is a constructor: standard error into klog and the shell's
 * splash dismissed, the folders a title may keep its files in, and the
 * environment the console does not give a title.
 *
 * A title sees its own folder (/app0, which is /data/homebrew/<TITLE_ID>) and
 * nothing else of /data. The working folder starts there (ps5/src/paths.c):
 *
 *   /app0/Burnout 3 Takedown/   the game's files from your disc (default.xbe, Data/,
 *                               Tracks/, ...): Takedawn runs from them as a checkout
 *   /app0/userdata/             saves, settings.toml, the log (XDG_DATA_HOME, HOME)
 *   /app0/cache/                the shader cache (XDG_CACHE_HOME)
 *   /app0/env.txt               optional: NAME=value lines for the environment
 *                               (RECOMP_* options for a test run)
 *
 * SPDX-License-Identifier: MIT
 */
#include <SDL_stdinc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ps5platform/fp.h>

#include "platform.h"

#define TITLE_DIR "/app0"

/* Both copies of the environment: the console's libc keeps what setenv sets
 * apart from the `environ` SDL copies. */
static void
set(const char *name, const char *value, int overwrite)
{
   setenv(name, value, overwrite);
   SDL_setenv(name, value, overwrite);
}

/* The game leaves through exit() from several places (its Quit, a fatal start-up
 * error): the shell is asked to close the title, as when main returns. Without
 * that the console shows a title that ended by itself as crashed. */
void catchReturnFromMain(int status);

static void
leave(void)
{
   static int left;

   if (!left++)
      catchReturnFromMain(0);
}

/* Takedawn's main with the console's arguments: no setup window (there is no
 * other program to start; the data is the folder above), then whatever
 * /app0/args.txt has, one argument a line. */
int __real_main(int argc, char **argv, char **envp);

int
__wrap_main(int argc, char **argv, char **envp)
{
   static char no_setup[] = "--no-setup";
   static char extra[16][256];
   char *args[64];
   int count = 0, lines = 0;
   FILE *file;

   args[count++] = argc > 0 ? argv[0] : "eboot.bin";
   args[count++] = no_setup;
   for (int i = 1; i < argc && count < 40; i++)
      args[count++] = argv[i];
   if ((file = fopen(TITLE_DIR "/args.txt", "r")) != NULL) {
      while (lines < 16 && count < 63 && fgets(extra[lines], sizeof extra[lines], file)) {
         extra[lines][strcspn(extra[lines], "\r\n")] = 0;
         if (extra[lines][0] && extra[lines][0] != '#') {
            say("args.txt: %s", extra[lines]);
            args[count++] = extra[lines++];
         }
      }
      fclose(file);
   }
   args[count] = NULL;
   return __real_main(count, args, envp);
}

__attribute__((constructor)) static void
ps5_startup(void)
{
   platform_init(PS5_TITLE_NAME);
   /* IEEE floating point, as on a PC and the Xbox: a process here starts with
    * denormals flushed (MXCSR 0x9fe0), and every thread starts with its
    * creator's state, so the game's threads get this too */
   ps5_fp_ieee();
   atexit(leave);
   mkdir(TITLE_DIR "/userdata", 0777);
   mkdir(TITLE_DIR "/cache", 0777);
   /* PS5 DevLoop's grab takes the title's pictures from here, and removes them */
   mkdir(TITLE_DIR "/userdata/screenshots", 0777);
   chmod(TITLE_DIR "/userdata/screenshots", 0777);
   /* The per-user folders (the toolkit's recomp_save_dir.c): the title's own */
   set("HOME", TITLE_DIR "/userdata", 1);
   set("XDG_DATA_HOME", TITLE_DIR "/userdata", 1);
   set("XDG_CONFIG_HOME", TITLE_DIR "/userdata", 1);
   set("XDG_CACHE_HOME", TITLE_DIR "/cache", 1);

   FILE *file = fopen(TITLE_DIR "/env.txt", "r");
   if (file) {
      char line[512];
      while (fgets(line, sizeof line, file)) {
         line[strcspn(line, "\r\n")] = 0;
         char *equals = strchr(line, '=');
         if (line[0] == '#' || !equals || equals == line)
            continue;
         *equals = 0;
         set(line, equals + 1, 1);
         say("env.txt: %s=%s", line, equals + 1);
      }
      fclose(file);
   }
}
