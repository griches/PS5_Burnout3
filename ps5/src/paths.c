/*
 * Burnout 3 for the PlayStation 5 - a working folder for a title that has none.
 *
 * Takedawn finds its data relative to where it runs ("Burnout 3 Takedown/default.xbe",
 * then a chdir into the data's root), and the toolkit opens the disc's files and
 * the save folder by relative paths too. A title on the console has no working
 * folder of its own: chdir is refused (EPERM) and libc's getcwd is a null import
 * (its __getcwd is only in libkernel_sys). So the folder is kept here: it starts
 * as /app0, the title's folder, chdir moves it (to a folder that exists), getcwd
 * answers it, and the path functions the game and its libraries call are wrapped
 * at link time (tools/link-title.sh: --wrap) to take a relative path from it.
 * Absolute paths pass through.
 *
 * SPDX-License-Identifier: MIT
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#define TITLE_DIR "/app0"

/* The platform layer's (ps5platform/libc.h), bound by PS5_Vulkan's link recipe */
DIR *ps5_opendir(const char *path);
int ps5_access(const char *path, int mode);

static char working[1024] = TITLE_DIR;

/* "a/./b/../c" as "a/c", from an absolute path: a title's folders hold no symbolic links */
static void
tidy(const char *full, char *out, size_t size)
{
   size_t length = 0;

   for (const char *part = full; *part;) {
      size_t count;
      while (*part == '/')
         part++;
      count = strcspn(part, "/");
      if (count == 2 && part[0] == '.' && part[1] == '.') {
         while (length > 0 && out[--length] != '/')
            ;
      } else if (count > 0 && !(count == 1 && part[0] == '.') && length + count + 2 < size) {
         out[length++] = '/';
         memcpy(out + length, part, count);
         length += count;
      }
      part += count;
   }
   if (length == 0)
      out[length++] = '/';
   out[length] = 0;
}

/* A relative path, taken from the working folder; anything else as it is */
static const char *
from_working(const char *path, char *buffer, size_t size)
{
   char joined[2048];

   if (path == NULL || path[0] == '/' || path[0] == '\0')
      return path;
   snprintf(joined, sizeof joined, "%s/%s", working, path);
   tidy(joined, buffer, size);
   return buffer;
}

#define PATH_BUFFER char buffer[1024]
#define WORKING(path) from_working((path), buffer, sizeof buffer)

FILE *__real_fopen(const char *path, const char *mode);
FILE *
__wrap_fopen(const char *path, const char *mode)
{
   PATH_BUFFER;
   return __real_fopen(WORKING(path), mode);
}

FILE *__real_freopen(const char *path, const char *mode, FILE *stream);
FILE *
__wrap_freopen(const char *path, const char *mode, FILE *stream)
{
   PATH_BUFFER;
   return __real_freopen(WORKING(path), mode, stream);
}

int __real_open(const char *path, int flags, ...);
int
__wrap_open(const char *path, int flags, ...)
{
   PATH_BUFFER;
   mode_t mode = 0;
   if (flags & O_CREAT) {
      va_list arguments;
      va_start(arguments, flags);
      mode = (mode_t)va_arg(arguments, int);
      va_end(arguments);
   }
   return __real_open(WORKING(path), flags, mode);
}

int __real_stat(const char *path, struct stat *status);
int
__wrap_stat(const char *path, struct stat *status)
{
   PATH_BUFFER;
   return __real_stat(WORKING(path), status);
}

int __real_lstat(const char *path, struct stat *status);
int
__wrap_lstat(const char *path, struct stat *status)
{
   PATH_BUFFER;
   return __real_lstat(WORKING(path), status);
}

int __real_mkdir(const char *path, mode_t mode);
int
__wrap_mkdir(const char *path, mode_t mode)
{
   PATH_BUFFER;
   return __real_mkdir(WORKING(path), mode);
}

int __real_rmdir(const char *path);
int
__wrap_rmdir(const char *path)
{
   PATH_BUFFER;
   return __real_rmdir(WORKING(path));
}

int __real_unlink(const char *path);
int
__wrap_unlink(const char *path)
{
   PATH_BUFFER;
   return __real_unlink(WORKING(path));
}

int __real_remove(const char *path);
int
__wrap_remove(const char *path)
{
   PATH_BUFFER;
   return __real_remove(WORKING(path));
}

int __real_rename(const char *from, const char *to);
int
__wrap_rename(const char *from, const char *to)
{
   char buffer[1024], buffer2[1024];
   return __real_rename(from_working(from, buffer, sizeof buffer),
                        from_working(to, buffer2, sizeof buffer2));
}

int __real_utime(const char *path, const struct utimbuf *times);
int
__wrap_utime(const char *path, const struct utimbuf *times)
{
   PATH_BUFFER;
   return __real_utime(WORKING(path), times);
}

DIR *
__wrap_opendir(const char *path)
{
   PATH_BUFFER;
   return ps5_opendir(WORKING(path));
}

int
__wrap_access(const char *path, int mode)
{
   PATH_BUFFER;
   return ps5_access(WORKING(path), mode);
}

/* The working folder, for getcwd and chdir (tools/link-title.sh binds them here) */
char *
ps5bo_getcwd(char *buffer, size_t size)
{
   size_t length = strlen(working) + 1;

   if (buffer == NULL) {
      size = size ? size : length;
      buffer = malloc(size);
      if (buffer == NULL)
         return NULL;
   }
   if (size < length) {
      errno = ERANGE;
      return NULL;
   }
   memcpy(buffer, working, length);
   return buffer;
}

int
ps5bo_chdir(const char *path)
{
   char target[1024];
   struct stat status;

   if (path == NULL || *path == 0) {
      errno = ENOENT;
      return -1;
   }
   from_working(path, target, sizeof target);
   if (path[0] == '/')
      tidy(path, target, sizeof target);
   if (__real_stat(target, &status) != 0)
      return -1;
   if (!S_ISDIR(status.st_mode)) {
      errno = ENOTDIR;
      return -1;
   }
   snprintf(working, sizeof working, "%s", target);
   return 0;
}

/* realpath by the text, from the working folder, for a path that exists */
char *
ps5bo_realpath(const char *path, char *resolved)
{
   char out[1024];
   struct stat status;

   if (path == NULL || *path == 0) {
      errno = path ? ENOENT : EINVAL;
      return NULL;
   }
   if (path[0] == '/')
      tidy(path, out, sizeof out);
   else
      from_working(path, out, sizeof out);
   if (__real_stat(out, &status) != 0) {
      errno = ENOENT;
      return NULL;
   }
   if (resolved == NULL && (resolved = malloc(1024)) == NULL)
      return NULL;
   snprintf(resolved, 1024, "%s", out);
   return resolved;
}
