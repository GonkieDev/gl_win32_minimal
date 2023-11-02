
//~ [h] includes
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 
#endif
#include <windows.h>

#include <gl/gl.h>

// NOTE(gsp): replace this with your own GL loader,
// it's only here to not clutter and
// should make it easier to put into your program
#include "gl_defines.h"

//~
#define BUILD_DEBUG 1

//~ base types
#include <stdint.h>
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float  f32;
typedef double f64;
typedef i32 b32;

#define function static
#define global static

#define Log(literalStr) OutputDebugStringA(literalStr "\n")

//~ Forward decl
function b32 GL_InitDummyContext(void);
function b32 GL_InitFunctionPointers(void);
function LRESULT WindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//~ globals
global HINSTANCE os_w32_hInstance = 0;
global HGLRC gl_w32_context = 0; 
global wgl_create_context_attribs_arb *wglCreateContextAttribsARB = 0;
global wgl_choose_pixel_format_arb *wglChoosePixelFormatARB = 0;

global b32 running = 1;
u32 clientWidth = 720;
u32 clientHeight = 480;

//~
int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    (void)(pCmdLine); (void)(hPrevInstance); (void)(nCmdShow);
    os_w32_hInstance = hInstance;
    
    //-
    if (!GL_InitDummyContext())
    {
        Log("Failed to init GL dummy context");
        return 0;
    }
    
    HWND hwnd = 0;
    HDC  hdc  = 0;
    
    WCHAR *windowTitle = L"GL Win32 Minimal";
    WCHAR *windowClassName = L"TestWindowClass";
    
    //- Register window class
    {
        WNDCLASSEXW wcex = {0};
        wcex.cbSize = sizeof(wcex);
        wcex.lpfnWndProc = WindowCallback;
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.hInstance = os_w32_hInstance;
        wcex.lpszClassName = windowClassName;
        wcex.hCursor = LoadCursor(0, IDC_ARROW);
        if (!RegisterClassExW(&wcex))
        {
            Log("Failed to register window class.");
            return 0;
        }
    }
    
    //- Create window
    {
        DWORD extendedWindowStyle = 0;
        DWORD windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX |
            WS_THICKFRAME;
        
        {
            // gsp: Obtain border size
            RECT borderRect = {0, 0, 0, 0};
            AdjustWindowRectEx(&borderRect, windowStyle, 0, extendedWindowStyle);
            u32 borderWidth  = borderRect.right - borderRect.left;
            u32 borderHeight = borderRect.bottom - borderRect.top;
            // gsp: Grow window size by size of os border
            clientWidth  += borderWidth;
            clientHeight += borderHeight;
        }
        
        hwnd = CreateWindowExW(extendedWindowStyle, windowClassName, windowTitle, windowStyle,
                               CW_USEDEFAULT, CW_USEDEFAULT, (i32)clientWidth, (i32)clientHeight, 
                               0, 0, os_w32_hInstance, 0);
    }
    
    //- Create window
    if (!hwnd)
    {
        Log("Failed to create main window.");
        return 0;
    }
    
    hdc = GetDC(hwnd);
    if (!hdc)
    {
        Log("Failed to get device context for main window");
        return 0;
    }
    
    ShowWindow(hwnd, SW_SHOW);
    
    //- Init GL Context
    int pixelFormatAttribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0
    };
    
    int pixelFormat;
    UINT numOfFormats = 0;
    wglChoosePixelFormatARB(hdc, pixelFormatAttribs, /* pfAttribList */ 0, 1, &pixelFormat, &numOfFormats);
    if (!numOfFormats)
    {
        Log("Failed to setup OpenGL pixel format.");
        return 0;
    }
    
    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(hdc, pixelFormat, sizeof(pfd), &pfd);
    if (!SetPixelFormat(hdc, pixelFormat, &pfd))
    {
        Log("Failed to set the OpenGL pixel format.");
        return 0;
    }
    
    int glAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 0,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if BUILD_DEBUG // gsp: enable this if you want debug mode
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        0
    };
    
    gl_w32_context = wglCreateContextAttribsARB(hdc, 0, glAttribs);
    if (!gl_w32_context)
    {
        Log("Failed to create OpenGL rendering context.");
        return 0;
    }
    
    if (!wglMakeCurrent(hdc, gl_w32_context))
    {
        Log("Failed to activate OpenGL rendering context.");
        return 0;
    }
    
    glViewport(0, 0, clientWidth, clientHeight);
    
    //- Main loop
    while(running)
    {
        glClearColor(0.788f, 0.419f, 0.349f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        Log("test!");
        
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        SwapBuffers(hdc);
    }
    
    return 0;
}

function b32
GL_InitDummyContext(void)
{
    b32 result = 0;
    
    WNDCLASSA glWindowClass = {0};
    glWindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    glWindowClass.lpfnWndProc = DefWindowProcA;
    glWindowClass.hInstance = os_w32_hInstance;
    glWindowClass.lpszClassName = "DummyWGLWindowClass";
    
    if (RegisterClassA(&glWindowClass))
    {
        HWND dummyWindow = CreateWindowExA(0, glWindowClass.lpszClassName,"Dummy WGL Window", 0,
                                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                           0, 0, os_w32_hInstance, 0);
        
        if (dummyWindow)
        {
            HDC dummyDC = GetDC(dummyWindow);
            
            PIXELFORMATDESCRIPTOR pfd = {0};
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            pfd.cColorBits = 32;
            pfd.cAlphaBits = 8;
            pfd.iLayerType = PFD_MAIN_PLANE;
            pfd.cDepthBits = 8;
            pfd.cStencilBits = 8;
            
            int pixelFormat = ChoosePixelFormat(dummyDC, &pfd);
            if (pixelFormat)
            {
                if (SetPixelFormat(dummyDC, pixelFormat, &pfd))
                {
                    HGLRC dummyContext = wglCreateContext(dummyDC);
                    if (dummyContext)
                    {
                        if (wglMakeCurrent(dummyDC, dummyContext))
                        {
                            wglCreateContextAttribsARB = (wgl_create_context_attribs_arb*)wglGetProcAddress("wglCreateContextAttribsARB");
                            wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb*)wglGetProcAddress("wglChoosePixelFormatARB");
                            
                            result = (0 != wglCreateContextAttribsARB) && (0 != wglChoosePixelFormatARB);
                            result &= GL_InitFunctionPointers();
                            
                            wglMakeCurrent(dummyDC, 0);
                        }
                        else
                        {
                            Log("Failed to activate dummy OpenGL rendering context.");
                        }
                        
                        wglDeleteContext(dummyContext);
                    }
                    else
                    {
                        Log("Failed to create dummy OpenGL context.");
                    }
                }
                else
                {
                    Log("Failed to set pixel format.");
                }
            }
            else
            {
                Log("Failed to find a suitable pixel format.");
            }
            
            ReleaseDC(dummyWindow, dummyDC);
            DestroyWindow(dummyWindow);
        }
        else
        {
            Log("Failed to create a dummy opengl window.");
        }
    }
    else
    {
        Log("Failed to register a dummy opengl window.");
    }
    
    return result;
}

function b32
GL_InitFunctionPointers(void)
{
    return 1;
}

function LRESULT
WindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
        PostQuitMessage(0);
        case WM_DESTROY:
        case WM_QUIT:
        {
            running = 0;
        } break;
        
        case WM_SIZE:
        {
            u32 width = LOWORD(lParam);
            u32 height = HIWORD(lParam);
            clientWidth  = width;
            clientHeight = height;
            glViewport(0, 0, clientWidth, clientHeight);
        } break;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}