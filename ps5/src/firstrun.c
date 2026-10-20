/*
 * Burnout 3 for the PlayStation 5 - the game's data on the first start.
 *
 * The player copies the title folder to the console and their own disc image
 * of the North America Xbox Burnout 3: Takedown into its dump/ folder. Before
 * Takedawn's main runs (ps5/src/startup.c), this finds the data and, the first
 * time, puts it in place with Takedawn's own installer (bo3_install_from,
 * vendor/takedawn/src/game/install.c: every file checked against the disc's
 * manifest, a stopped install carried on, the wrong disc refused with a
 * sentence that says why). Then the game runs from /app0 with --data /app0,
 * which needs the installer's marker, so unchecked files are never run.
 *
 *   /app0/.complete               installed: start the game
 *   /app0/Burnout 3 Takedown/     the files (an extracted folder put there by
 *                                 hand is checked where it is, nothing copied)
 *   /app0/dump/<any>.iso, .xiso   an image, unpacked once (it may be deleted after)
 *   /app0/dump/<folder>/          an extracted folder (or one level further down)
 *   /app0/<any>.iso               an image beside eboot.bin
 *
 * There is no picture during the install: the console's notifications say
 * what is happening, and why when nothing can be used.
 *
 * SPDX-License-Identifier: MIT
 */
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "platform.h"

#define TITLE_DIR "/app0"
#define GAME_DIR "Burnout 3 Takedown"
#define TITLE_FOLDER "/data/homebrew/PPSA99316"

/* Takedawn's installer with its disc manifest (vendor/takedawn/src/game/install.c) */
int bo3_install_from(const char *source, const char *dest, int check_only,
                     void (*progress)(void *ctx, double fraction, const char *text), void *ctx,
                     char *message, size_t n);

int sceKernelSendNotificationRequest(int device, void *request, size_t size, int blocking);

/* A notification on the screen (the title has no picture yet), and klog */
static void
notify(const char *format, ...)
{
   static struct {
      char unused[45];
      char message[3075];
   } request;
   va_list arguments;

   memset(&request, 0, sizeof request);
   va_start(arguments, format);
   vsnprintf(request.message, sizeof request.message, format, arguments);
   va_end(arguments);
   say("[firstrun] %s", request.message);
   sceKernelSendNotificationRequest(0, &request, sizeof request, 0);
}

static int
is_dir(const char *path)
{
   struct stat status;
   return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
}

static int
is_file(const char *path)
{
   struct stat status;
   return stat(path, &status) == 0 && S_ISREG(status.st_mode);
}

static int
ends_with(const char *name, const char *suffix)
{
   size_t n = strlen(name), m = strlen(suffix);
   if (n < m)
      return 0;
   for (size_t i = 0; i < m; i++)
      if (tolower((unsigned char)name[n - m + i]) != suffix[i])
         return 0;
   return 1;
}

/* The candidates, in the order they are tried */
#define MAX_SOURCES 16
static char sources[MAX_SOURCES][1024];
static int source_count;

static void
add(const char *path)
{
   if (source_count < MAX_SOURCES)
      snprintf(sources[source_count++], sizeof sources[0], "%s", path);
}

/* Images in a folder (.iso, .xiso), and with folders the extracted discs in it:
 * a folder holding default.xbe, or one whose folder does */
static void
scan(const char *folder, int folders)
{
   DIR *directory = opendir(folder);
   struct dirent *entry;
   char path[1024], inner[1100];

   if (directory == NULL)
      return;
   while ((entry = readdir(directory)) != NULL) {
      if (entry->d_name[0] == '.')
         continue;
      snprintf(path, sizeof path, "%s/%s", folder, entry->d_name);
      if ((ends_with(entry->d_name, ".iso") || ends_with(entry->d_name, ".xiso")) && is_file(path)) {
         add(path);
      } else if (folders && is_dir(path)) {
         snprintf(inner, sizeof inner, "%s/default.xbe", path);
         if (is_file(inner)) {
            add(path);
         } else {
            DIR *sub = opendir(path);
            struct dirent *e;
            while (sub != NULL && (e = readdir(sub)) != NULL) {
               char deeper[1100], xbe[1200];
               if (e->d_name[0] == '.')
                  continue;
               snprintf(deeper, sizeof deeper, "%s/%s", path, e->d_name);
               snprintf(xbe, sizeof xbe, "%s/default.xbe", deeper);
               if (is_file(xbe))
                  add(deeper);
            }
            if (sub != NULL)
               closedir(sub);
         }
      }
   }
   closedir(directory);
}

/* Progress: a notification at the start and at each quarter */
static int last_quarter;
static time_t started;

static void
progress(void *ctx, double fraction, const char *text)
{
   (void)ctx;
   (void)text;
   int quarter = (int)(fraction * 4.0);
   if (quarter > last_quarter && quarter < 4) {
      last_quarter = quarter;
      notify("Burnout 3: setting up the game data, %d%% done...", quarter * 25);
   }
}

int
ps5_first_run(void)
{
   char message[1024], first_problem[1024] = "", path[1100];
   int i;

   if (is_file(TITLE_DIR "/.complete"))
      return 0;

   source_count = 0;
   snprintf(path, sizeof path, TITLE_DIR "/" GAME_DIR "/default.xbe");
   if (is_file(path))
      add(TITLE_DIR "/" GAME_DIR);
   scan(TITLE_DIR "/dump", 1);
   scan(TITLE_DIR, 0);

   for (i = 0; i < source_count; i++) {
      int code = bo3_install_from(sources[i], TITLE_DIR, 1, NULL, NULL, message, sizeof message);
      say("[firstrun] %s: %s", sources[i], code == 0 ? "the right disc" : message);
      if (code != 0) {
         if (!first_problem[0])
            snprintf(first_problem, sizeof first_problem, "%s", message);
         continue;
      }
      notify("Burnout 3: setting up the game data from your disc. This takes a few minutes, "
             "once. Please wait.");
      last_quarter = 0;
      started = time(NULL);
      code = bo3_install_from(sources[i], TITLE_DIR, 0, progress, NULL, message, sizeof message);
      if (code == 0) {
         if (strstr(sources[i], "/" GAME_DIR) && is_dir(sources[i]) && !strstr(sources[i], "/dump/"))
            notify("Burnout 3: game data checked and ready (%ld s).", (long)(time(NULL) - started));
         else
            notify("Burnout 3: game data ready (%ld s). You can delete the disc image or folder in "
                   TITLE_FOLDER "/dump/ now.",
                   (long)(time(NULL) - started));
         return 0;
      }
      notify("Burnout 3: the game data could not be set up.\n%s", message);
      return -1;
   }
   if (first_problem[0])
      notify("Burnout 3: your disc image cannot be used.\n%s\nIt needs the North America (USA) Xbox "
             "disc of Burnout 3: Takedown, in " TITLE_FOLDER "/dump/.",
             first_problem);
   else
      notify("Burnout 3: no game data found.\nCopy your own image of the North America (USA) Xbox "
             "disc of Burnout 3: Takedown (.iso) into " TITLE_FOLDER "/dump/, then start the game "
             "again.");
   return -1;
}
