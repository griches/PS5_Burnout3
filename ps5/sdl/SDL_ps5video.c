/*
 * Doom 3 BFG for the PlayStation 5 - SDL2's video driver for the console.
 *
 * The console has one output and no windows: this gives SDL one display and
 * lets it keep its window bookkeeping (size, flags, events) for a window that
 * is the whole screen. Its Vulkan calls answer for the console: the "library"
 * is RADV, linked into the title, and a window's surface is VK_KHR_display's
 * first display in its first mode. libultraship's Vulkan renderer draws
 * through that.
 *
 * The display's size is the console's output (3840x2160 on a 4K television,
 * 1920x1080 otherwise), asked of VK_KHR_display at start-up, so SDL's idea of
 * the window is the swapchain's. SDL_PS5_DISPLAY=WIDTHxHEIGHT in the
 * environment overrides it.
 *
 * After the SDL3 driver of the DevilutionX port.
 * SPDX-License-Identifier: Zlib
 */
#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PS5

#include "video/SDL_sysvideo.h"
#include "events/SDL_events_c.h"
#include "events/SDL_keyboard_c.h"
#include "events/SDL_mouse_c.h"
#include "video/SDL_vulkan_internal.h"

#include "platform.h"

/* RADV's entry point: there is no Vulkan loader, the driver is in the title */
extern PFN_vkVoidFunction vk_icdGetInstanceProcAddr(VkInstance instance, const char *name);

static const char *const ps5_instance_extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_DISPLAY_EXTENSION_NAME };

static int PS5_Vulkan_LoadLibrary(_THIS, const char *path)
{
    _this->vulkan_config.vkGetInstanceProcAddr = (void *)vk_icdGetInstanceProcAddr;
    _this->vulkan_config.vkEnumerateInstanceExtensionProperties =
        (void *)vk_icdGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties");
    if (!_this->vulkan_config.vkEnumerateInstanceExtensionProperties) {
        return SDL_SetError("RADV has no vkEnumerateInstanceExtensionProperties");
    }
    SDL_strlcpy(_this->vulkan_config.loader_path, "radv", SDL_arraysize(_this->vulkan_config.loader_path));
    return 0;
}

static void PS5_Vulkan_UnloadLibrary(_THIS)
{
}

static SDL_bool PS5_Vulkan_GetInstanceExtensions(_THIS, SDL_Window *window, unsigned *count, const char **names)
{
    return SDL_Vulkan_GetInstanceExtensions_Helper(count, names, SDL_arraysize(ps5_instance_extensions),
                                                   ps5_instance_extensions);
}

static SDL_bool PS5_Vulkan_CreateSurface(_THIS, SDL_Window *window, VkInstance instance, VkSurfaceKHR *surface)
{
    return SDL_Vulkan_Display_CreateSurface(_this->vulkan_config.vkGetInstanceProcAddr, instance, surface);
}

/* The console's output, as VK_KHR_display reports its first display */
static SDL_bool PS5_OutputSize(int *w, int *h)
{
    PFN_vkCreateInstance create = (PFN_vkCreateInstance)vk_icdGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    VkApplicationInfo application;
    VkInstanceCreateInfo info;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice device = VK_NULL_HANDLE;
    uint32_t count = 1;
    SDL_bool found = SDL_FALSE;

    SDL_zero(application);
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.apiVersion = VK_API_VERSION_1_0;
    SDL_zero(info);
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &application;
    info.enabledExtensionCount = SDL_arraysize(ps5_instance_extensions);
    info.ppEnabledExtensionNames = ps5_instance_extensions;
    if (!create || create(&info, NULL, &instance) != VK_SUCCESS) {
        return SDL_FALSE;
    }
#define PS5_VK(name) PFN_##name name = (PFN_##name)vk_icdGetInstanceProcAddr(instance, #name)
    {
        PS5_VK(vkEnumeratePhysicalDevices);
        PS5_VK(vkGetPhysicalDeviceDisplayPropertiesKHR);
        PS5_VK(vkDestroyInstance);
        if (vkEnumeratePhysicalDevices && vkGetPhysicalDeviceDisplayPropertiesKHR &&
            vkEnumeratePhysicalDevices(instance, &count, &device) >= VK_SUCCESS && count == 1) {
            VkDisplayPropertiesKHR display;
            count = 1;
            if (vkGetPhysicalDeviceDisplayPropertiesKHR(device, &count, &display) >= VK_SUCCESS && count == 1 &&
                display.physicalResolution.width && display.physicalResolution.height) {
                *w = (int)display.physicalResolution.width;
                *h = (int)display.physicalResolution.height;
                found = SDL_TRUE;
            }
        }
        if (vkDestroyInstance) {
            vkDestroyInstance(instance, NULL);
        }
    }
#undef PS5_VK
    return found;
}

static int PS5_VideoInit(_THIS)
{
    SDL_DisplayMode mode;
    int w = 1920, h = 1080;
    const char *size = SDL_getenv("SDL_PS5_DISPLAY");

    if (size && SDL_sscanf(size, "%dx%d", &w, &h) == 2) {
        say("video: %dx%d from SDL_PS5_DISPLAY", w, h);
    } else if (PS5_OutputSize(&w, &h)) {
        say("video: the console's output is %dx%d", w, h);
    } else {
        w = 1920;
        h = 1080;
        say("video: the console's output size is unknown, taking %dx%d", w, h);
    }
    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.w = w;
    mode.h = h;
    mode.refresh_rate = 60;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }
    return 0;
}

static int PS5_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    return 0; /* the output stays as it is */
}

static void PS5_VideoQuit(_THIS)
{
}

static void PS5_PumpEvents(_THIS)
{
}

static int PS5_CreateWindow(_THIS, SDL_Window *window)
{
    /* The window is the screen, and always has the pad's focus */
    SDL_SetKeyboardFocus(window);
    SDL_SetMouseFocus(window);
    return 0;
}

static void PS5_DestroyWindow(_THIS, SDL_Window *window)
{
}

static void PS5_SetWindowPosition(_THIS, SDL_Window *window)
{
}

static void PS5_SetWindowSize(_THIS, SDL_Window *window)
{
    /* The size stays the display's */
}

static void PS5_SetWindowFullscreen(_THIS, SDL_Window *window, SDL_VideoDisplay *display, SDL_bool fullscreen)
{
}

static void PS5_DeleteDevice(_THIS)
{
    SDL_free(_this);
}

static SDL_VideoDevice *PS5_CreateDevice(void)
{
    SDL_VideoDevice *device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return NULL;
    }
    device->VideoInit = PS5_VideoInit;
    device->VideoQuit = PS5_VideoQuit;
    device->SetDisplayMode = PS5_SetDisplayMode;
    device->PumpEvents = PS5_PumpEvents;
    device->CreateSDLWindow = PS5_CreateWindow;
    device->DestroyWindow = PS5_DestroyWindow;
    device->SetWindowPosition = PS5_SetWindowPosition;
    device->SetWindowSize = PS5_SetWindowSize;
    device->SetWindowFullscreen = PS5_SetWindowFullscreen;
    device->Vulkan_LoadLibrary = PS5_Vulkan_LoadLibrary;
    device->Vulkan_UnloadLibrary = PS5_Vulkan_UnloadLibrary;
    device->Vulkan_GetInstanceExtensions = PS5_Vulkan_GetInstanceExtensions;
    device->Vulkan_CreateSurface = PS5_Vulkan_CreateSurface;
    device->free = PS5_DeleteDevice;
    device->quirk_flags = VIDEO_DEVICE_QUIRK_FULLSCREEN_ONLY;
    return device;
}

VideoBootStrap PS5_bootstrap = {
    "ps5", "PlayStation 5 video driver",
    PS5_CreateDevice,
    NULL /* no ShowMessageBox implementation */
};

#endif /* SDL_VIDEO_DRIVER_PS5 */
