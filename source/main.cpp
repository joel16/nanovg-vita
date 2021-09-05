extern "C" {
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <gpu_es4/psp2_pvr_hint.h>

unsigned int sceLibcHeapSize = 64 * 1024 * 1024;

#include "nanovg.h"
#define NANOVG_GLES2_IMPLEMENTATION	// Use GL2 implementation.
#include "nanovg_gl.h"

#include "demo.h"
}

static EGLDisplay s_display;
static EGLContext s_context;
static EGLSurface s_surface;

// Based on init/deinit egl functions with minor changes from https://github.com/Adubbz/nanovg-deko3d-example/blob/master/source/test_gl.cpp
namespace EGL {
    static bool Init(void) {
        // Connect to the EGL default display
        s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (!s_display) {
            sceClibPrintf("Could not connect to display! error: %d", eglGetError());
            goto _fail0;
        }
        
        // Initialize the EGL display connection
        eglInitialize(s_display, nullptr, nullptr);
        
        // Select OpenGL (Core) as the desired graphics API
        if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
            sceClibPrintf("Could not set API! error: %d", eglGetError());
            goto _fail1;
        }
        
        // Get an appropriate EGL framebuffer configuration
        EGLConfig configs[2];
        EGLint numConfigs;
        
        static const EGLint framebufferAttributeList[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE,     8,
            EGL_GREEN_SIZE,   8,
            EGL_BLUE_SIZE,    8,
            EGL_ALPHA_SIZE,   8,
            EGL_DEPTH_SIZE,   24,
            EGL_STENCIL_SIZE, 8,
            EGL_NONE
        };
        
        if (eglGetConfigs(s_display, configs, 2, &numConfigs) == EGL_FALSE) {
            sceClibPrintf("Could not get configs! error: %d", eglGetError());
            goto _fail1;
        }
        
        eglChooseConfig(s_display, framebufferAttributeList, configs, 1, &numConfigs);
        if (numConfigs == 0) {
            sceClibPrintf("No config found! error: %d", eglGetError());
            goto _fail1;
        }
        
        // Create an EGL window surface
        s_surface = eglCreateWindowSurface(s_display, configs[0], (EGLNativeWindowType)0, nullptr);
        if (!s_surface) {
            sceClibPrintf("Surface creation failed! error: %d", eglGetError());
            goto _fail1;
        }
        
        // Create an EGL rendering context
        static const EGLint contextAttributeList[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        
        s_context = eglCreateContext(s_display, configs[0], EGL_NO_CONTEXT, contextAttributeList);
        if (!s_context) {
            sceClibPrintf("Context creation failed! error: %d", eglGetError());
            goto _fail2;
        }
        
        // Connect the context to the surface
        eglMakeCurrent(s_display, s_surface, s_surface, s_context);
        return true;
        
_fail2:
        eglDestroySurface(s_display, s_surface);
        s_surface = nullptr;
_fail1:
        eglTerminate(s_display);
        s_display = nullptr;
_fail0:
        return false;
    }

    static void Exit(void) {
        if (s_display) {
            eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            if (s_context) {
                eglDestroyContext(s_display, s_context);
                s_context = nullptr;
            }
            if (s_surface) {
                eglDestroySurface(s_display, s_surface);
                s_surface = nullptr;
            }
            
            eglTerminate(s_display);
            s_display = nullptr;
        }
    }
}

int main(int argc, char *argv[]) {
    const SceSize max_dst_size = 64;

    // Load modules required for libGLESv2 and PVRSRV
    sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("app0:libgpu_es4_ext.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("app0:libIMGEGL.suprx", 0, nullptr, 0, nullptr, nullptr);
    
    PVRSRV_PSP2_APPHINT hint;
    PVRSRVInitializeAppHint(&hint);
    sceClibSnprintf(hint.szGLES2, max_dst_size, "app0:module/libGLESv2.suprx");
    sceClibSnprintf(hint.szWindowSystem, max_dst_size, "app0:module/libpvrPSP2_WSEGL.suprx");
    PVRSRVCreateVirtualAppHint(&hint);

    if (!EGL::Init())
        return EXIT_FAILURE;

    NVGcontext *vg = nullptr;
    vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);

    if (vg == nullptr) {
        sceClibPrintf("Could not init nanovg.\n");
        return -1;
    }

    DemoData data;
    if (loadDemoData(vg, &data) == -1)
        return -1;

    bool done = false;
    int blowup = 0, screenshot = 0, premult = 0;
    int winWidth = 960, winHeight = 544, fbWidth = 960, fbHeight = 544;
    unsigned int pressed = 0;
    SceCtrlData pad = {}, old_pad = {};
    double prevt = sceKernelGetProcessTimeWide();
    double frequency = 1000000.0;
    
    while (!done) {
        double t = sceKernelGetProcessTimeWide();
        
        // Calculate pixel ration for hi-dpi devices.
        float pxRatio = (float)fbWidth / (float)winWidth;
        
        // Update and render
        glViewport(0, 0, fbWidth, fbHeight);
        
        if (premult)
            glClearColor(0, 0, 0, 0);
        else
            glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
            
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        
        nvgBeginFrame(vg, winWidth, winHeight, pxRatio);
        renderDemo(vg, 0.0, 0.0, winWidth, winHeight, (float)(t - prevt) / frequency, blowup, &data);
        nvgEndFrame(vg);
        
        if (screenshot) {
            screenshot = 0;
            saveScreenShot(fbWidth, fbHeight, premult, "ux0:data/nanovg-screenshot.png");
        }
        
        glEnable(GL_DEPTH_TEST);
        eglSwapBuffers(s_display, s_surface);
        
        // Handle input
        sceClibMemset(&pad, 0, sizeof(SceCtrlData));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        pressed = pad.buttons & ~old_pad.buttons;
        old_pad = pad;

        if (pressed & SCE_CTRL_TRIANGLE)
            blowup = !blowup;
        if (pressed & SCE_CTRL_SQUARE)
            premult = !premult;
        if (pressed & SCE_CTRL_SELECT)
            screenshot = 1;
            
        if (pressed & SCE_CTRL_START)
            done = true;
    }
    
    freeDemoData(vg, &data);
    
    // Deinitialize our scene
    nvgDeleteGLES2(vg);
    
    // Deinitialize EGL
    EGL::Exit();
    sceKernelExitProcess(0);
    return EXIT_SUCCESS;
}
