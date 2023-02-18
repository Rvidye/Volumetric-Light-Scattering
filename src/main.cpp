// Header Files
#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<Mmsystem.h>
#include<string>
#include<fstream>
#include<sstream>
#include<map>
#include<string>
#include<vector>

// OpenGL Header Files
#include<GL/glew.h> // this must be above gl.h
#include<GL/gl.h>

#include"vmath.h"
using namespace vmath;

// Image Loading Library
#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

// Assimp
#include<assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

#include"OGL.h"

#include"Bone.h"
#include"AnimMesh.h"
#include"AnimModel.h"
#include"Animation.h"
#include"Animator.h"

#define WIN_WIDTH 640
#define WIN_HEIGHT 360
#define FBO_SIZE 1024
#define SHADOW_WIDTH_HEIGHT 4096

// OpenGL Libraries
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"OpenGL32.lib")
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"assimp.lib")

// Global Variable
HWND ghwnd = NULL;
HDC ghdc = NULL;
HGLRC ghrc = NULL;
BOOL gbFullScreen = FALSE;
BOOL gbActiveWindow = FALSE;
TCHAR str[125];

int screenWidth, screenHeight;

FILE* glLog;

// Program Struct

struct Program
{
    GLuint program;
    struct {
        GLuint projection;
        GLuint view;
        GLuint model;
        GLuint diffuse;
    }uniforms;
};

Program myprog,modelProgram, godRaysProgram, sceneRender;

GLuint simpleDepthProgram, shadowMapProgram;

GLuint vao;
GLuint texture;
GLuint texture2;
GLuint texture3;
GLuint boneindex;

mat4 projectionMatrix;

Model* backpack,*natraj;

GLuint finalBoneMatricesUniform[100];

std::vector<Vertex> pcntCube;
GLuint cube_vao;

GLuint fboScene;
GLuint texScene;
GLuint rboScene;

GLuint fboDepth;
GLuint texDepth;

float moonPos;

int curretScene = 0;
static float t = 0.0f;
// Global Function Declarations
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
void GLAPIENTRY ErrorCallback(GLenum src, GLenum type, GLuint id, GLenum saverity, GLsizei length, const GLchar* message, const void* userParam);

// Main Function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int iCmdShow)
{
    // Function Declaration
    int initialize(void);
    void display(void);
    void update(void);
    void uninitialize(void);
    // Variable Declaration
    WNDCLASSEX wndclass;
    HWND hwnd;
    MSG msg;
    TCHAR szAppName[] = TEXT("MyWindow");
    BOOL bDone = FALSE;
    int iRetVal = 0;

    // Code

    if(fopen_s(&gpFile,"Log.txt","w") != 0)
    {
        MessageBox(NULL,TEXT("Creation Of Log File Failed.\nExitting ..."),TEXT("File I/O Error"),MB_ICONERROR);
        exit(0);
    }
    else
    {
        fprintf(gpFile,"Log File Created Sucessfully.\n");
        fclose(gpFile);
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth -  WIN_WIDTH) / 2;
    int y = (screenHeight - WIN_HEIGHT) / 2;

    // Initialization Of WNDCLASSEX Structure
    wndclass.cbSize         = sizeof(WNDCLASSEX);
    wndclass.style          = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.hInstance      = hInstance;
    wndclass.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH); 
    wndclass.hIcon          = LoadIcon(hInstance,MAKEINTRESOURCE(BAT_ICON));
    wndclass.hCursor        = LoadCursor(NULL,IDC_ARROW);
    wndclass.lpszClassName  = szAppName;
    wndclass.lpszMenuName   = NULL;
    wndclass.hIconSm        = LoadIcon(hInstance,MAKEINTRESOURCE(BAT_ICON));

    // Registering Above WndClass
    RegisterClassEx(&wndclass);

    // Create Window
    hwnd = CreateWindowEx(  
                            WS_EX_APPWINDOW,
                            szAppName,
                            TEXT("Template"),
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                            x,
                            y,
                            WIN_WIDTH,
                            WIN_HEIGHT,
                            NULL,
                            NULL,
                            hInstance,
                            NULL
                        );

    ghwnd = hwnd;

    iRetVal = initialize();

    if(iRetVal == -1)
    {
        Log("Error", "choose pixel format failed");
        uninitialize();
    }

    if(iRetVal == -2)
    {
        Log("Error", "set pixel format failed");
        uninitialize();        
    }

    if(iRetVal == -3)
    {
        Log("Error", "Create wgl context failed");
        uninitialize();        
    }

    if(iRetVal == -4)
    {
        Log("Error", "make current context failed");
        uninitialize();        
    }

    if(iRetVal == -5)
    {
        Log("Error", "glewinit failed");
        uninitialize();        
    }

    // Show window
    ShowWindow(hwnd,iCmdShow);

    // Foregrounding and Focusing The Window
    // ghwnd or hwnd will work but hwnd is for local functions.
    SetForegroundWindow(hwnd);

    SetFocus(hwnd);

    // Special loop
    while(!bDone)
    {
        if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
                bDone = TRUE;
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            if(gbActiveWindow)
            {
                // Render The Scene
                display();
                // Update the Scene
                update();
            }
        }
    }
    uninitialize();
    return (int)msg.wParam;
}

// Callback Function
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    // Local Function Declaration
    void ToogleFullScreen(void);
    void resize(int,int);
    void uninitialize(void);
    // Local Variable 
    // Code
    int oldMouseX, oldMouseY;
    int offsetX, offsetY;
    vec3 dir;
    float speed = 0.5f;
    switch(iMsg)
    {
        case WM_CREATE:
            //fprintf(gpFile,"In WM_CREATE Message.\n");
            //sprintf(str,"!!! Press F To Enter FullScreen !!!");
        break;

        case WM_CHAR:
            switch(wParam)
            {
                case 'F':
                    case 'f':
                        //fprintf(gpFile,"In ToogleFullscreen.\n");
                        moonPos = -1.5f;
                        PlaySound(TEXT("resource\\shiva.wav"), NULL, SND_ASYNC);
                        ToogleFullScreen();
                break;
                case 'A':
                case 'a':
                    DebugCam.cameraPosition -= speed * normalize(cross(DebugCam.cameraFront, DebugCam.cameraUp));
                break;
                case 'W':
                case 'w':
                    DebugCam.cameraPosition += speed * DebugCam.cameraFront;
                    break;
                case 'S':
                case 's':
                    DebugCam.cameraPosition -= speed * DebugCam.cameraFront;
                    break;

                case 'D':
                case 'd':
                    DebugCam.cameraPosition += speed * normalize(cross(DebugCam.cameraFront, DebugCam.cameraUp));
                    break;
                case 'L':
                case 'l':
                break;
                case 'p':
                    //fopen_s(&gpFile, "Log.txt", "a");
                    //3fprintf(gpFile, "X = %f and Y = %f\n", x,y);
                    //fclose(gpFile);
                    break;
                default:
                break;
            }
        break;

        case WM_LBUTTONDOWN:
            DebugCam.lastMouseX = LOWORD(lParam);
            DebugCam.lastMouseY = HIWORD(lParam);
        break;
        
        case WM_LBUTTONUP:
            DebugCam.lastMouseX = -1;
            DebugCam.lastMouseY = -1;
        break;
        
        case WM_MOUSEMOVE:
            if (DebugCam.lastMouseX != -1 && DebugCam.lastMouseY != -1)
            {
                offsetX = LOWORD(lParam) - DebugCam.lastMouseX;
                offsetY = DebugCam.lastMouseY - HIWORD(lParam);

                DebugCam.lastMouseX = LOWORD(lParam);
                DebugCam.lastMouseY = HIWORD(lParam);

                offsetX *= 0.1f;
                offsetY *= 0.1f;

                DebugCam.cameraYaw += offsetX;
                DebugCam.cameraPitch += offsetY;

                if (DebugCam.cameraPitch > 89.0f)
                {
                    DebugCam.cameraPitch = 89.0f;
                }
                else if (DebugCam.cameraPitch < -89.0f)
                {
                    DebugCam.cameraPitch = -89.0f;
                }

                dir = vec3(cos(vmath::radians(DebugCam.cameraYaw)) * cos(vmath::radians(DebugCam.cameraPitch)), sin(vmath::radians(DebugCam.cameraPitch)), sin(vmath::radians(DebugCam.cameraYaw)) * cos(vmath::radians(DebugCam.cameraPitch)));
                DebugCam.cameraFront = vmath::normalize(dir);
            }
        break;
        case WM_SETFOCUS:
            //fprintf(gpFile,"Set Focus True.\n");
            gbActiveWindow = TRUE;
        break;

        case WM_KILLFOCUS:
            //fprintf(gpFile,"Set Focus False.\n");
            //gbActiveWindow = FALSE;
        break;
        
        case WM_ERASEBKGND:
            return 0;
        break;

        case WM_KEYDOWN:
            if(wParam == VK_ESCAPE)
            {
                //fprintf(gpFile,"Sending WM_CLOSE.\n");
                DestroyWindow(hwnd);
            }

            if (wParam == VK_SPACE)
            {
                curretScene = 1;
                moonPos = -1.5f;
            }

        break;

        case WM_SIZE:
            //fprintf(gpFile,"In WM SIZE message.\n");
            resize(LOWORD(lParam),HIWORD(lParam));
        break; 

        case WM_CLOSE:
            Log("LOG", "In WM_CLOSE Message.");
            DestroyWindow(hwnd);
        break;

        case WM_DESTROY:
            //uninitialize();
            Log("LOG", "In WM_DESTROY Message.");
            PostQuitMessage(0);
        break;

        default:
            break;
    }
    return DefWindowProc(hwnd,iMsg,wParam,lParam);
}

void ToogleFullScreen(void)
{
    // Varriable Declarations
    static DWORD dwStyle;
    static WINDOWPLACEMENT wp;
    MONITORINFO mi;

    // Code
    wp.length = sizeof(WINDOWPLACEMENT);
    if(gbFullScreen == FALSE)
    {
        dwStyle = GetWindowLong(ghwnd,GWL_STYLE);
        if(dwStyle & WS_OVERLAPPEDWINDOW)
        {
            mi.cbSize = sizeof(MONITORINFO);
            if(GetWindowPlacement(ghwnd,&wp) && GetMonitorInfo(MonitorFromWindow(ghwnd,MONITORINFOF_PRIMARY),&mi))
            {
                SetWindowLong(ghwnd,GWL_STYLE,(dwStyle & (~WS_OVERLAPPEDWINDOW)));
                SetWindowPos(   ghwnd,HWND_TOPMOST,mi.rcMonitor.left,mi.rcMonitor.top,
                                mi.rcMonitor.right - mi.rcMonitor.left,
                                mi.rcMonitor.bottom - mi.rcMonitor.top,
                                SWP_NOZORDER|SWP_FRAMECHANGED);
            }

            ShowCursor(TRUE);
            gbFullScreen = TRUE;
        }
    }
    else
    {
        SetWindowLong(ghwnd,GWL_STYLE,dwStyle|WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(ghwnd,&wp);
        SetWindowPos(ghwnd,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER|SWP_FRAMECHANGED);
        ShowCursor(TRUE);
        gbFullScreen = FALSE;
    }
}

int initialize(void)
{
    // Function Declarations
    void resize(int,int);
    void printGLInfo(void);
    void uninitialize(void);

    /*
        OpenGL Context Setup Start
    */

    // Variable Declarations
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormatIndex = 0;

    // Code
    ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits = 8;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32; // 24 is also allowed

    // Get DC
    ghdc = GetDC(ghwnd);

    // Choose Pixel Format
    iPixelFormatIndex = ChoosePixelFormat(ghdc,&pfd);

    if(iPixelFormatIndex == 0)
        return -1;

    // Set The Choosen Pixel Format
    if(SetPixelFormat(ghdc,iPixelFormatIndex,&pfd) == FALSE)
        return -2;

    // Create OpenGL Rendering Index

    ghrc = wglCreateContext(ghdc);

    if(ghrc == NULL)
        return -3;

    // Make Rendering as current context and rendering context
    if(wglMakeCurrent(ghdc,ghrc) == FALSE)
        return -4;

    // Here Starts OpenGL Code :

    //glew initialization
    if(glewInit() != GLEW_OK)
        return -5;

    // print opengl info
    //printGLInfo(); 
    
    /*
        OpenGL Context Setup End
    */

    /*
        OpenGL Default State Setup Start
    */

    GLuint vsh = loadShader("resource\\shaders\\basic.vert", GL_VERTEX_SHADER);
    GLuint fsh = loadShader("resource\\shaders\\basic.frag", GL_FRAGMENT_SHADER);

    myprog.program = glCreateProgram();
    glAttachShader(myprog.program, vsh);
    glAttachShader(myprog.program, fsh);
    glBindAttribLocation(myprog.program,0,"vPos");
    glBindAttribLocation(myprog.program, 1, "vCol");
    glLinkProgram(myprog.program);

    checkError(myprog.program,false);

    myprog.uniforms.projection = glGetUniformLocation(myprog.program, "pMat");
    myprog.uniforms.view = glGetUniformLocation(myprog.program, "vMat");
    myprog.uniforms.model = glGetUniformLocation(myprog.program, "mMat");
    myprog.uniforms.diffuse = glGetUniformLocation(myprog.program, "diffuse");

    //
    vsh = loadShader("resource\\shaders\\model.vert",GL_VERTEX_SHADER);
    fsh = loadShader("resource\\shaders\\model.frag", GL_FRAGMENT_SHADER);

    modelProgram.program = glCreateProgram();
    glAttachShader(modelProgram.program, vsh);
    glAttachShader(modelProgram.program, fsh);
    glBindAttribLocation(modelProgram.program, 0, "a_position");
    glBindAttribLocation(modelProgram.program, 1, "a_normal");
    glBindAttribLocation(modelProgram.program, 2, "a_texcoord");
    glBindAttribLocation(modelProgram.program, 3, "a_tangent");
    glBindAttribLocation(modelProgram.program, 4, "a_bitangent");
    glLinkProgram(modelProgram.program);

    checkError(modelProgram.program,false);

    modelProgram.uniforms.projection = glGetUniformLocation(modelProgram.program,"u_Projection");
    modelProgram.uniforms.view = glGetUniformLocation(modelProgram.program, "u_View");
    modelProgram.uniforms.model = glGetUniformLocation(modelProgram.program, "u_Model");

    vsh = loadShader("resource\\shaders\\godrays.vert",GL_VERTEX_SHADER);
    fsh = loadShader("resource\\shaders\\godrays.frag", GL_FRAGMENT_SHADER);

    godRaysProgram.program = glCreateProgram();
    glAttachShader(godRaysProgram.program, vsh);
    glAttachShader(godRaysProgram.program, fsh);
    glLinkProgram(godRaysProgram.program);

    checkError(godRaysProgram.program,false);

    fsh = loadShader("resource\\shaders\\texRender.frag", GL_FRAGMENT_SHADER);
    sceneRender.program = glCreateProgram();
    glAttachShader(sceneRender.program, vsh);
    glAttachShader(sceneRender.program, fsh);
    glLinkProgram(sceneRender.program);

    checkError(sceneRender.program, false);

    vsh = loadShader("resource\\shaders\\simpleDepth.vert", GL_VERTEX_SHADER);
    fsh = loadShader("resource\\shaders\\simpleDepth.frag", GL_FRAGMENT_SHADER);

    simpleDepthProgram = glCreateProgram();
    glAttachShader(simpleDepthProgram, vsh);
    glAttachShader(simpleDepthProgram, fsh);
    glLinkProgram(simpleDepthProgram);

    checkError(simpleDepthProgram, false);

    vsh = loadShader("resource\\shaders\\shadowMap.vert", GL_VERTEX_SHADER);
    fsh = loadShader("resource\\shaders\\shadowMap.frag", GL_FRAGMENT_SHADER);

    shadowMapProgram = glCreateProgram();
    glAttachShader(shadowMapProgram, vsh);
    glAttachShader(shadowMapProgram, fsh);
    glLinkProgram(shadowMapProgram);

    checkError(shadowMapProgram, false);

    // FBO Setup

    glGenTextures(1, &texScene);
    glBindTexture(GL_TEXTURE_2D, texScene);
    glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,1920,1080);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenRenderbuffers(1, &rboScene);
    glBindRenderbuffer(GL_RENDERBUFFER, rboScene);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, 1920, 1080);

    glGenFramebuffers(1, &fboScene);
    glBindFramebuffer(GL_FRAMEBUFFER, fboScene);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texScene, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboScene);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &texDepth);
    glBindTexture(GL_TEXTURE_2D, texDepth);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, vec4(1.0f, 1.0f, 1.0f, 1.0f));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 1920, 1080);

    glGenFramebuffers(1, &fboDepth);
    glBindFramebuffer(GL_FRAMEBUFFER, fboDepth);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texDepth, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    loadTexture("resource\\textures\\moon.png", &texture);
    loadTexture("resource\\textures\\brickwall.jpg", &texture2);
    loadTexture("resource\\textures\\natraj.jpg", &texture3);

    const GLfloat squareData[] =
    {
        +1.0f,-1.0f,0.0f,1.0f,0.0f,1.0f,0.0f,1.0f,1.0f,0.0f,0.0f,0.0f,1.0f,
        -1.0f,-1.0f,0.0f,1.0f,1.0f,0.0f,0.0f,1.0f,0.0f,0.0,0.0f,0.0f,1.0f,
        +1.0f,+1.0f,0.0f,1.0f,0.0f,0.0f,1.0f,1.0f,1.0f,1.0f,0.0f,0.0f,1.0f,
        -1.0f,+1.0f,0.0f,1.0f,1.0f,0.0f,1.0f,1.0f,0.0f,1.0f,0.0f,0.0f,1.0f,
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(squareData), squareData, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 13 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 13 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 13 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 13 * sizeof(float), (void*)(10 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);

    //clear screen using blue color:
    glClearColor(0.0f,0.0f,0.0f,1.0f);

    // Depth Related Changes
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glPolygonOffset(4.0f, 4.0f);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(ErrorCallback,0);

    /*
        OpenGL Default State Setup End
    */

    //D:\Programming\OpenGL\SimpleOpenGLFrameWork\resource\models\vampire
    //backpack = new Model("resource\\models\\backpack\\backpack.obj");
    backpack = new Model("resource\\models\\shivalowpoly.obj");
    backpack->printMeshVertex();

    natraj = new Model("resource\\models\\natraj.obj");
    natraj->printMeshVertex();

    // Debug Cam Setup !!!
    DebugCam.lastMouseX = -1;
    DebugCam.lastMouseY = -1;
    DebugCam.cameraYaw = -90.0f;
    DebugCam.cameraPitch = 0.0f;
    DebugCam.cameraFront = vec3(0.0f, 0.0f, -1.0f);
    DebugCam.cameraPosition = vec3(0.0f,0.0f,1.0f);
    DebugCam.cameraUp = vec3(0.0f,1.0f,0.0f);

    moonPos = -1.5f;

    //warmup resize call
    resize(WIN_WIDTH,WIN_HEIGHT);
    return 0;
}

void printGLInfo(void)
{
    // Local variable declarations
    GLint numExtentions = 0;

    // code
    FILE* glFile = NULL;
    fopen_s(&glFile, "GLInfo.txt", "w");
    fprintf(glFile,"OpenGL Vendor : %s\n",glGetString(GL_VENDOR));
    fprintf(glFile,"OpenGL Renderer : %s\n",glGetString(GL_RENDERER));
    fprintf(glFile,"OpenGL Version : %s\n",glGetString(GL_VERSION));
    fprintf(glFile,"OpenGL GLSL Version : %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
    glGetIntegerv(GL_NUM_EXTENSIONS,&numExtentions);
    fprintf(glFile,"Number Of Supported Extensions : %d\n",numExtentions);
    //fprintf(gpFile,"OpenGL  : %s\n",glGetString(GL_VERSION));    
    for (int i = 0; i < numExtentions; i++)
    {
        /* code */
        fprintf(glFile,"%s\n",glGetStringi(GL_EXTENSIONS,i));
    }
    fclose(glFile);
    glFile = NULL;
}

void resize(int width,int height)
{
    // Code

    GLfloat aspectRatio = 0.0f;

    // to avoid divide by 0 error later in codebase.
    if(height == 0)
        height = 1;
    
    screenWidth = width;
    screenHeight = height;

    glViewport(0,0,(GLsizei)width,(GLsizei)height);
    projectionMatrix = vmath::perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void renderScene1()
{
    mat4 modelMatrix = mat4::identity();
    mat4 viewMatrix = mat4::identity();

    modelMatrix = vmath::translate(0.0f, moonPos, -5.0f);
    viewMatrix = vmath::lookat(DebugCam.cameraPosition, DebugCam.cameraFront, DebugCam.cameraUp);

    glUseProgram(myprog.program);
    glUniformMatrix4fv(myprog.uniforms.model, 1, GL_FALSE, modelMatrix);
    glUniformMatrix4fv(myprog.uniforms.view, 1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(myprog.uniforms.projection, 1, GL_FALSE, projectionMatrix);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(myprog.uniforms.diffuse, 0);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);

    modelMatrix = mat4::identity();

    modelMatrix *= vmath::translate(0.0f, 0.0f, -2.0f) * vmath::scale(0.1f, 0.1f, 0.1f);
    glUseProgram(modelProgram.program);
    glUniformMatrix4fv(modelProgram.uniforms.model, 1, GL_FALSE, modelMatrix);
    glUniformMatrix4fv(modelProgram.uniforms.view, 1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(modelProgram.uniforms.projection, 1, GL_FALSE, projectionMatrix);
    glUniform3fv(glGetUniformLocation(modelProgram.program, "viewPos"), 1, DebugCam.cameraPosition);
    glUniform3fv(glGetUniformLocation(modelProgram.program, "light.direction"), 1, vec3(1.0f, moonPos, 2.0f));
    glUniform3fv(glGetUniformLocation(modelProgram.program, "light.ambient"), 1, vec3(0.1f, 0.1f, 0.1f));
    glUniform3fv(glGetUniformLocation(modelProgram.program, "light.diffuse"), 1, vec3(1.0f, 1.0f, 1.0f));
    glUniform3fv(glGetUniformLocation(modelProgram.program, "light.specular"), 1, vec3(1.0f, 1.0f, 1.0f));
    glUniform3fv(glGetUniformLocation(modelProgram.program, "material.diffuseMat"), 1, vec3(1.0f, 1.0f, 1.0f));
    glUniform3fv(glGetUniformLocation(modelProgram.program, "material.specularMat"), 1, vec3(0.5f, 0.5f, 0.5f));
    glUniform1f(glGetUniformLocation(modelProgram.program, "material.shininess"), 90.0f);
    glUniform1f(glGetUniformLocation(modelProgram.program, "material.opacity"), 1.0f);
    backpack->Draw(modelProgram.program);
    glUseProgram(0);
}

void renderScene2()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture3);
    glUniformMatrix4fv(2, 1, GL_FALSE, vmath::translate(0.0f, 0.0f, 0.0f) * vmath::scale(0.1f, 0.1f, 0.1f));
    natraj->Draw(shadowMapProgram);
}

void display(void)
{
    // Code


    static float theta = M_PI;
    vec3 lightPos = vec3(2.0f * sin(theta), 2.0f, -3.0f);

    glClearBufferfv(GL_COLOR, 0, vec4(0.1f, 0.1f, 0.1f, 1.0f));
    glClearBufferfv(GL_DEPTH, 0, vec1(1.0f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, screenWidth, screenHeight);

    if (curretScene == 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fboScene);
        glClearBufferfv(GL_COLOR, 0, vec4(1.0f, 1.0f, 1.0f, 1.0f));
        glClearBufferfv(GL_DEPTH, 0, vec1(1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, screenWidth, screenHeight);
        renderScene1();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else
    {

        //DebugCam.cameraYaw = -90.0f;
        //DebugCam.cameraPitch = 0.0f;
        DebugCam.cameraFront = vec3(cos(radians(-90.0f)) * cos(radians(-30.0f)), sin(radians(-30.0f)), sin(radians(-90.0f)) * cos(radians(-30.0f)));
        DebugCam.cameraPosition = vec3(0.0f, 2.0f, 5.0f);
        DebugCam.cameraUp = vec3(0.0f, 1.0f, 0.0f);

        mat4 scaleBiasMatrix = mat4(
            vec4(0.5f, 0.0f, 0.0f, 0.0f),
            vec4(0.0f, 0.5f, 0.0f, 0.0f),
            vec4(0.0f, 0.0f, 0.5f, 0.0f),
            vec4(0.5f, 0.5f, 0.5f, 1.0f));

        mat4 lightSpaceMatrix = ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f) * lookat(lightPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));

        glBindFramebuffer(GL_FRAMEBUFFER, fboDepth);
        glUseProgram(simpleDepthProgram);
        glClearBufferfv(GL_DEPTH, 0, vec1(1.0f));
        glViewport(0, 0, 1920, 1080);
        glUseProgram(simpleDepthProgram);
        glUniformMatrix4fv(0, 1, GL_FALSE, lightSpaceMatrix);
        glEnable(GL_POLYGON_OFFSET_FILL);
        renderScene2();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDisable(GL_POLYGON_OFFSET_FILL);
        glUseProgram(shadowMapProgram);
        glBindFramebuffer(GL_FRAMEBUFFER, fboScene);
        glClearBufferfv(GL_COLOR, 0, vec4(0.1f, 0.1f, 0.1f, 1.0f));
        glClearBufferfv(GL_DEPTH, 0, vec1(1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, 1920, 1080);

        glUniformMatrix4fv(0, 1, GL_FALSE, projectionMatrix);
        glUniformMatrix4fv(1, 1, GL_FALSE, lookat(DebugCam.cameraPosition, DebugCam.cameraFront + DebugCam.cameraPosition, DebugCam.cameraUp));
        glUniformMatrix4fv(3, 1, GL_FALSE, scaleBiasMatrix * lightSpaceMatrix);
        glUniform3fv(4, 1, lightPos);
        glUniform3fv(5, 1, DebugCam.cameraPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texDepth);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glUniformMatrix4fv(2, 1, GL_FALSE, vmath::translate(0.0f, -1.0f, 0.0f) * vmath::rotate(90.0f, 1.0f, 0.0f, 0.0f) * vmath::scale(5.0f, 5.0f, 1.0f));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        renderScene2();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    /*

    */

    if (curretScene == 0)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, screenWidth, screenHeight);
        glUseProgram(godRaysProgram.program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texScene);
        glUniform1f(0, 1.1f);
        glUniform1f(1, 0.06f);
        glUniform1f(2, 0.98f);
        glUniform1f(3, 0.5f);
        glUniform1i(4, 100);
        glUniform2fv(5, 1, vec2(0.5f, moonPos * 0.5f + 0.5f));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUseProgram(0);
    }
    else
    {
        glClearBufferfv(GL_COLOR, 0, vec4(1.0f, 1.0f, 1.0f, 1.0f));
        glViewport(0, 0, screenWidth, screenHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(sceneRender.program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texScene);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glEnable(GL_DEPTH_TEST);
    }
    theta += t * 0.5f;

    SwapBuffers(ghdc);
}

void update(void)
{
    // Code
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0)
        timeStart = timeCur;
    t = (timeCur - timeStart) / 10000.0f;
    timeStart = timeCur;

    moonPos += t * 0.5f ;
}

void uninitialize(void)
{
    // Function Declarations

    void ToogleFullScreen(void);

    // Code

    if (backpack)
    {
        backpack->ModelCleanup();
        delete backpack;
        backpack = NULL;
    }

    if (vao)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }

    if (myprog.program)
    {
        glUseProgram(myprog.program);

        GLsizei num_attached_shaders;
        glGetProgramiv(myprog.program, GL_ATTACHED_SHADERS, &num_attached_shaders);
        GLuint* shader_objects = NULL;

        shader_objects = (GLuint*)malloc(num_attached_shaders);

        glGetAttachedShaders(myprog.program, num_attached_shaders, &num_attached_shaders, shader_objects);

        for (GLsizei i = 0; i < num_attached_shaders; i++)
        {
            glDetachShader(myprog.program, shader_objects[i]);
            glDeleteShader(shader_objects[i]);
            shader_objects[i] = 0;
        }

        free(shader_objects);

        glUseProgram(0);

        glDeleteProgram(myprog.program);
    }

    if(gbFullScreen)
    {
        ToogleFullScreen();
    }

    if(wglGetCurrentContext() == ghrc)
    {
        wglMakeCurrent(NULL,NULL);
    }

    if(ghrc)
    {
        wglDeleteContext(ghrc);
        ghrc = NULL;
    }

    if(ghdc)
    {
        ReleaseDC(ghwnd,ghdc);
    }

    if(ghwnd)
    {
        DestroyWindow(ghwnd);
    }

    if(gpFile)
    {
        Log("LOG", "Closing Log File.");
        gpFile = NULL;
    }
}

void GLAPIENTRY ErrorCallback(GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    fopen_s(&glLog, "GLLOG.txt", "a");
    fprintf(glLog,"GL CALLBACK: %s type = 0x%x, serverity = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
    fclose(glLog);
}
