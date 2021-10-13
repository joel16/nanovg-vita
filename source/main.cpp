extern "C" {
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gpu_es4/psp2_pvr_hint.h>

unsigned int sceLibcHeapSize = 64 * 1024 * 1024;

#include "nanovg.h"
#define NANOVG_GLES2_IMPLEMENTATION	// Use GLES2 implementation.
#include "nanovg_gl.h"

#include "demo.h"

#define SCE_KERNEL_CPU_MASK_SHIFT		(16)
#define SCE_KERNEL_CPU_MASK_USER_0		(0x01 << SCE_KERNEL_CPU_MASK_SHIFT)
#define SCE_KERNEL_CPU_MASK_USER_1		(0x02 << SCE_KERNEL_CPU_MASK_SHIFT)
#define SCE_KERNEL_CPU_MASK_USER_2		(0x04 << SCE_KERNEL_CPU_MASK_SHIFT)
}

// Global vars
static EGLint screen_width = 0, screen_height = 0;

// Based on init/deinit egl functions with minor changes from https://github.com/Adubbz/nanovg-deko3d-example/blob/master/source/test_gl.cpp
namespace EGL {
    static EGLDisplay s_display = EGL_NO_DISPLAY;
    static EGLContext s_context = EGL_NO_CONTEXT;
    static EGLSurface s_surface = EGL_NO_SURFACE;

    static bool Init(void) {
        EGLConfig configs[2];
        EGLint numConfigs = 0;
        EGLint major = 0, minor = 0;
        
        static const EGLint framebufferAttributeList[] =  {
            EGL_BUFFER_SIZE,     EGL_DONT_CARE,
            EGL_RED_SIZE,        8,
            EGL_GREEN_SIZE,      8,
            EGL_BLUE_SIZE,       8,
            EGL_ALPHA_SIZE,      8,
            EGL_STENCIL_SIZE,    8,
            EGL_DEPTH_SIZE,      16,
            EGL_SAMPLES,         4,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
        };

        static const EGLint contextAttributeList[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

        // Connect to the EGL default display
        s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (!s_display) {
            sceClibPrintf("Could not connect to display! error: %d", eglGetError());
            goto _fail0;
        }
        
        // Initialize the EGL display connection
        eglInitialize(s_display, &major, &minor);
        
        // Select OpenGL (Core) as the desired graphics API
        if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
            sceClibPrintf("Could not set API! error: %d", eglGetError());
            goto _fail1;
        }

        if (eglGetConfigs(s_display, configs, 2, &numConfigs) == EGL_FALSE) {
            sceClibPrintf("Cound not get configs! error: %d", eglGetError());
            goto _fail1;
        }
        
        // Get an appropriate EGL framebuffer configuration    
        eglChooseConfig(s_display, framebufferAttributeList, configs, 2, &numConfigs);
        if (numConfigs == 0) {
            sceClibPrintf("No config found! error: %d", eglGetError());
            goto _fail1;
        }

        Psp2NativeWindow win;
        win.type = PSP2_DRAWABLE_TYPE_WINDOW;
        win.windowSize = PSP2_WINDOW_960X544;
        
        if (sceKernelGetModel() == SCE_KERNEL_MODEL_VITATV)
            win.windowSize = PSP2_WINDOW_1920X1088;
        
        win.numFlipBuffers = 2;
        win.flipChainThrdAffinity = SCE_KERNEL_CPU_MASK_USER_1;
        
        // Create an EGL window surface
        s_surface = eglCreateWindowSurface(s_display, configs[0], &win, nullptr);
        if (!s_surface) {
            sceClibPrintf("Surface creation failed! error: %d", eglGetError());
            goto _fail1;
        }
        
        // Create an EGL rendering context
        s_context = eglCreateContext(s_display, configs[0], EGL_NO_CONTEXT, contextAttributeList);
        if (!s_context) {
            sceClibPrintf("Context creation failed! error: %d", eglGetError());
            goto _fail2;
        }
        
        // Connect the context to the surface
        eglMakeCurrent(s_display, s_surface, s_surface, s_context);
        eglQuerySurface(s_display, s_surface, EGL_WIDTH, &screen_width);
        eglQuerySurface(s_display, s_surface, EGL_HEIGHT, &screen_height);
        eglSwapInterval(s_display, 0);
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

    static EGLBoolean SwapBuffers(void) {
        return eglSwapBuffers(s_display, s_surface);
    }
}

int main(int argc, char *argv[]) {
    // Load modules required for libGLESv2 and PVRSRV
    sceKernelLoadStartModule("vs0:sys/external/libfios2.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("vs0:sys/external/libc.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("app0:libgpu_es4_ext.suprx", 0, nullptr, 0, nullptr, nullptr);
    sceKernelLoadStartModule("app0:libIMGEGL.suprx", 0, nullptr, 0, nullptr, nullptr);
    
    PVRSRV_PSP2_APPHINT hint;
    PVRSRVInitializeAppHint(&hint);
    PVRSRVCreateVirtualAppHint(&hint);

    if (!EGL::Init())
        return EXIT_FAILURE;

    NVGcontext *vg = nullptr;
    vg = nvgCreateGLES2(NVG_DEBUG);

    if (vg == nullptr) {
        sceClibPrintf("Could not init nanovg.\n");
        return -1;
    }

    DemoData data;
    if (loadDemoData(vg, &data) == -1)
        return -1;

    bool done = false;
    int blowup = 0, screenshot = 0, premult = 0;
    int winWidth = screen_width, winHeight = screen_height, fbWidth = screen_width, fbHeight = screen_height;
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
            
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
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
        EGL::SwapBuffers();
        
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
