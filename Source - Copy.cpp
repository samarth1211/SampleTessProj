#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<string>
#include<vector>

#include<gl\glew.h>
#include<gl\GL.h>

//DevIL
#include<IL\il.h>
//#include<IL\ilu.h>
//#include<IL\ilut.h>


#include "vmath.h"
#include "Camera.h"
#include "CommonHeader.h"

#define WIN_WIDTH	800
#define WIN_HEIGHT	600
#define DELTA 0.66666666666667f
#define MAX_VALUE_PER_PIXEL 65536

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"DevIL.lib")



LRESULT CALLBACK MainWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

bool g_bWindowActive = false;
HWND g_hwnd = NULL;
HDC  g_hdc = NULL;
HGLRC g_hrc = NULL;

WINDOWPLACEMENT wpPrev;
DWORD dwStyle;
bool g_bFullScreen = false;

FILE *g_pFile = NULL;

// Shaders
//GLuint iVertexShaderObject = 0;
//GLuint iFragmentShaderObject = 0;
GLuint g_ShaderProgramObject_Ter = 0;

// All Vertex Buffers
GLuint g_VertexArrayObject_ter = 0;
GLuint g_VertexBufferObject_Position_ter = 0;
GLuint g_VertexBufferObject_Color_ter = 0;

// Uniforms
GLint g_Uniform_Model_Matrix = 0;
GLint g_Uniform_View_Matrix = 0;
GLint g_Uniform_Projection_Matrix = 0;

GLint g_Uniform_HeightStep = 0;
GLint g_Uniform_GridSpacing = 0;
GLint g_Uniform_ViewPortDim = 0;
GLint g_Uniform_UseRoughness = 0;
GLint g_Uniform_pixelsPerEdge = 0;
GLint g_Uniform_culling = 0;
GLint g_Uniform_ScaleFactor = 0;
GLint g_Uniform_PatchSize = 0;

// sampler
GLuint g_uniform_TextureSampler;
GLint g_Uniform_Sampler_texUnit = 0;
GLint g_Uniform_Sampler_RoughFactor = 0;
GLint g_Uniform_Sampler_HeightMap = 0;
GLint g_uniform_sampler_NormalMap = 0;

// Projection
vmath::mat4 g_PersPectiveProjectionMatrix;

// Camera Keys
Camera camera(vmath::vec3(150.0f, 10.0f, 400.0f));
GLfloat g_fLastX = WIN_WIDTH / 2;
GLfloat g_fLastY = WIN_HEIGHT / 2;

GLfloat g_DeltaTime = 0.0f;
GLboolean g_bFirstMouse = true;
GLfloat g_fCurrrentWidth;
GLfloat g_fCurrrentHeight;

float g_Angle_Pyramid = 0;

// Textures
GLuint g_Texture_HeightMap = 0;
GLuint g_Texture_Roughness = 0;
GLuint g_Texture_NormalMap = 0;

/**		Terrain Values Start	**/
#define PATCHSIZE 64
unsigned short *g_iHeightsRef = NULL;
float g_fHeightStep = 900.0f;
int g_iScaleFactor = 2;
float g_fGridSpacing = 10.0f;
int g_iRoughnessMode = 1;
int g_iCulling = 1;
unsigned int g_iGridSizeX = 0;
unsigned int g_iGridSizeZ = 0;
int g_iPixelsPerEdge = 1;
int iNumIndices = 0;
std::string terrainName = "ps_height_1k.png";
std::string normalMapName = "ps_height_1k.png";
/**		Terrain Values Stop 	**/

bool g_bWireframe = true;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow)
{
	//int UnInitialize(void);
	int Initialize(void);
	void Update(void);
	void Render(void);

	// Windowing Elelments
	WNDCLASSEX wndclass;
	MSG msg;
	HWND hwnd = NULL;
	TCHAR szClassName[] = TEXT("Sam_OGL");
	RECT windowRect;

	// Game Loop Control
	bool bDone = false;

	// Initialization Status
	int iInitRet = 0;


	SecureZeroMemory((void*)&wndclass, sizeof(wndclass));
	wndclass.cbSize = sizeof(wndclass);
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc = MainWndProc;
	wndclass.lpszClassName = szClassName;
	wndclass.lpszMenuName = NULL;
	wndclass.hInstance = hInstance;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	wndclass.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wndclass.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(hInstance, IDC_ARROW);

	if (!RegisterClassEx(&wndclass))
	{
		MessageBox(NULL, TEXT("Issue...!!!"), TEXT("Could Not RegisterClass() "), MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	if ((fopen_s(&g_pFile, "SamLogFile.txt", "w+")) == 0)
	{
		fprintf_s(g_pFile, "File Opened Successfully. \n");
	}
	else
	{
		MessageBox(NULL, TEXT("Issue...!!!"), TEXT("Could not open File"), MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	SecureZeroMemory((void*)&windowRect, sizeof(windowRect));
	windowRect.left = 0;
	windowRect.top = 0;
	windowRect.bottom = WIN_HEIGHT;
	windowRect.right = WIN_WIDTH;
	AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);

	hwnd = CreateWindowEx(WS_EX_APPWINDOW, szClassName,
		TEXT("First_OpenGL_Window"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, TEXT("Issue...!!!"), TEXT("Could Not CreateWindow() "), MB_OK | MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	g_hwnd = hwnd;

	iInitRet = Initialize();
	switch (iInitRet)
	{
	case INIT_ALL_OK:
		fprintf_s(g_pFile, "Initialize Complete \n");
		break;
	case INIT_FAIL_NO_HDC:
		fprintf_s(g_pFile, "Failed to Get HDC \n");
		DestroyWindow(hwnd);
		break;
	case INIT_FAIL_NO_PIXEL_FORMAT:
		fprintf_s(g_pFile, "Failed to get PixelFormat \n");
		DestroyWindow(hwnd);
		break;
	case INIT_FAIL_SET_PIXEL_FORMAT:
		fprintf_s(g_pFile, "Failed to set Pixel Format \n");
		DestroyWindow(hwnd);
		break;
	case INIT_FAIL_BRIDGE_CONTEX_CREATION:
		fprintf_s(g_pFile, "Failed to wglCreateContext \n");
		DestroyWindow(hwnd);
		break;
	case INIT_FAIL_BRIDGE_CONTEX_SET:
		fprintf_s(g_pFile, "Failed to wglMakeCurrent \n");
		DestroyWindow(hwnd);
		break;
	case INIT_FAIL_GLEW_INIT:
		fprintf_s(g_pFile, "Failed to glewInit \n");
		DestroyWindow(hwnd);
		break;
	case INIT_LINK_SHADER_PROGRAM_FAILED:
		fprintf_s(g_pFile, "Failed to Link Shader Program Object \n");
		DestroyWindow(hwnd);
		break;
	case INIT_VERTEX_SHADER_COMPILATION_FAILED:
		fprintf_s(g_pFile, "Failed to Compile vertex Shader \n");
		DestroyWindow(hwnd);
		break;
	case INIT_FRAGMENT_SHADER_COMPILATION_FAILED:
		fprintf_s(g_pFile, "Failed to Compile fragment Shader \n");
		DestroyWindow(hwnd);
		break;
	default:
		fprintf_s(g_pFile, "Failed UnKnown Reasons \n");
		DestroyWindow(hwnd);
		break;
	}

	ShowWindow(hwnd, SW_SHOWNORMAL);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);


	while (bDone == false)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				bDone = true;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

		}
		else
		{
			if (g_bWindowActive)
			{
				Update();
			}
			// Show all Animations
			Render();

		}
	}


	//UnInitialize();

	return ((int)msg.wParam);
}


LRESULT CALLBACK MainWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	int UnInitialize(void);
	void FullScreen(void);
	bool Resize(int, int);
	switch (iMsg)
	{
	case WM_CREATE:
		PostMessage(hwnd, WM_KEYDOWN, (WPARAM)0x46, (LPARAM)NULL);
		break;

	case WM_SETFOCUS:
		g_bWindowActive = true;
		break;

	case WM_KILLFOCUS:
		g_bWindowActive = false;
		break;

	case WM_KEYDOWN:

		switch (LOWORD(wParam))
		{
		case VK_ESCAPE:
			DestroyWindow(hwnd);
			break;

		case 0x41:// A is pressed
			camera.ProcessKeyBoard(E_LEFT, DELTA);
			break;
		case 0x44:// D is pressed
			camera.ProcessKeyBoard(E_RIGHT, DELTA);
			break;
		case 0x57:// W is pressed
			camera.ProcessKeyBoard(E_FORWARD, DELTA);
			break;
		case 0x53:// S is pressed
			camera.ProcessKeyBoard(E_BACKARD, DELTA);
			break;

			// Arraow Keys
		case VK_UP:
			g_iPixelsPerEdge++;
			if (g_iPixelsPerEdge >= 64)
				g_iPixelsPerEdge = 64;
			break;
		case VK_DOWN:
			g_iPixelsPerEdge--;
			if (g_iPixelsPerEdge <= 1)
				g_iPixelsPerEdge = 1;

			break;
		case VK_LEFT:
			break;
		case VK_RIGHT:
			break;
		case 0x46: // 'f' or 'F'
			//MessageBox(hwnd, TEXT("F is pressed"), TEXT("Status"), MB_OK);
			FullScreen();
			break;

		case 0x45:// e
			if (g_bWireframe)
			{
				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
				g_bWireframe = false;
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				g_bWireframe = true;
			}
			break;

		default:
			break;
		}
		break;
	case WM_MOUSEMOVE: // g_fLastX  g_fLastY
	{
		GLfloat xPos = LOWORD(lParam);
		GLfloat yPos = HIWORD(lParam);

		if (g_bFirstMouse)
		{
			g_fLastX = xPos;
			g_fLastY = yPos;

			g_bFirstMouse = false;
		}

		GLfloat xOffset = xPos - g_fLastX;
		GLfloat yOffset = g_fLastY - yPos;

		g_fLastX = xPos;
		g_fLastY = yPos;

		/*g_fLastX = g_fCurrrentWidth / 2;
		g_fLastY = g_fCurrrentHeight / 2;*/

		camera.ProcessMouseMovements(xOffset, yOffset);
	}
	break;

	case WM_MOUSEWHEEL:
	{
		GLfloat xPos = LOWORD(lParam);
		GLfloat yPos = HIWORD(lParam);
		camera.ProcessMouseScrool(xPos, yPos);
	}
	break;

	case WM_SIZE:
		g_fCurrrentWidth = LOWORD(lParam);
		g_fCurrrentHeight = HIWORD(lParam);
		Resize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_ERASEBKGND:
		return(0);
		//break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		UnInitialize();
		PostQuitMessage(0);
		break;

	default:
		break;
	}

	return (DefWindowProc(hwnd, iMsg, wParam, lParam));
}

int Initialize(void)
{
	bool Resize(int, int);
	bool LoadHeightMaapWithImage(std::string heightMap, int *gridSizeX, int *gridSizeZ);
	int iPixelIndex = 0;
	PIXELFORMATDESCRIPTOR pfd;

	// Shader Programs
	GLuint iVertexShaderObject = 0;
	GLuint iTESControlShaderObject = 0;
	GLuint iTESEvaluationShaderObject = 0;
	GLuint iGeometryShaderObject = 0;
	GLuint iFragmentShaderObject = 0;

	GLenum err = NULL; // GLEW Error codes

	SecureZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cAlphaBits = 8;

	g_hdc = GetDC(g_hwnd);
	if (g_hdc == NULL)
	{
		return INIT_FAIL_NO_HDC;
	}

	iPixelIndex = ChoosePixelFormat(g_hdc, &pfd);
	if (iPixelIndex == 0)
	{
		return INIT_FAIL_NO_PIXEL_FORMAT;
	}

	if (SetPixelFormat(g_hdc, iPixelIndex, &pfd) == FALSE)
	{
		return INIT_FAIL_SET_PIXEL_FORMAT;
	}

	g_hrc = wglCreateContext(g_hdc);
	if (g_hrc == NULL)
	{
		return INIT_FAIL_BRIDGE_CONTEX_CREATION;
	}

	if (wglMakeCurrent(g_hdc, g_hrc) == FALSE)
	{
		return INIT_FAIL_BRIDGE_CONTEX_SET;
	}

	// Enables Feature Required for Programable Pipeline
	err = glewInit();
	if (err != GLEW_OK)
	{
		return INIT_FAIL_GLEW_INIT;
	}

	// GL information Start
	fprintf_s(g_pFile, "SHADER_INFO : Vendor is : %s\n", glGetString(GL_VENDOR));
	fprintf_s(g_pFile, "SHADER_INFO : Renderer is : %s\n", glGetString(GL_RENDER));
	fprintf_s(g_pFile, "SHADER_INFO : OpenGL Version is : %s\n", glGetString(GL_VERSION));
	fprintf_s(g_pFile, "SHADER_INFO : GLSL Version is : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	//fprintf_s(g_pFile, "SHADER_INFO : Extention is : %s \n", glGetString(GL_EXTENSIONS));
	// GL information End

	/// Sam : all Shader Code Start

	/*Vertex Shader Start*/
	iVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
	const GLchar *vertexShaderSourceCode = "#version 450 core"	\
		"\n" \
		"layout (location = 0)in vec2 vPosition;" \
		"out vec2 posV;" \
		"void main(void)" \
		"{" \
		"	posV = vPosition;"	\
		"}";

	glShaderSource(iVertexShaderObject, 1, (const GLchar**)&vertexShaderSourceCode, NULL);

	// Compile it
	glCompileShader(iVertexShaderObject);
	GLint iInfoLogLength = 0;
	GLint iShaderCompileStatus = 0;
	GLchar *szInfoLog = NULL;
	glGetShaderiv(iVertexShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus==GL_FALSE)
	{
		glGetShaderiv(iVertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog!=NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(iVertexShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile,"ERROR : Vertex Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				return INIT_VERTEX_SHADER_COMPILATION_FAILED;
				//DestroyWindow(g_hwnd);
				//exit(EXIT_FAILURE);
			}
		}
	}

	/*Vertex Shader End*/

	/*Tess control Shader Start*/
	iTESControlShaderObject = glCreateShader(GL_TESS_CONTROL_SHADER);
	const GLchar *tessControlShaderSourceCode = 
		"#version 450 core"	\
		"\n"	\
		"layout(vertices=1)out;\n" \
		"in vec2 posV[];\n"	\
		"out vec2 posTC[];"	\
		"uniform float heightStep;\n"	\
		"uniform float gridSpacing;\n"	\
		"uniform ivec2 viewportDim;\n"	\
		"layout (binding =1)uniform sampler2D roughFactor;\n"	\
		"layout (binding =0)uniform sampler2D heightMap;\n"	\
		"uniform int useRoughness;\n"	\
		"uniform int pixelsPerEdge;\n"	\
		"uniform int culling;\n"	\
		"uniform int scaleFactor;\n"	\
		"uniform float patchSize;\n"	\
		"uniform mat4 u_model_matrix,u_view_matrix,u_projection_matrix;" \
		"float height(float u, float v)\n"	\
		"{\n"	\
		"	return (texture(heightMap, vec2(u,v)).r  * heightStep);\n"	\
		"}\n"	\
		"bool segmentInFrustum(vec4 p1,vec4 p2)\n"	\
		"{\n"	\
		"	if((p1.x < -p1.w && p2.x < -p2.w) || (p1.x > p1.w && p2.x > p2.w) ||(p1.z < -p1.w && p2.z < -p2.w) || (p1.z > p1.w && p2.z > p2.w))"	\
		"	{"	\
		"		return false;"	\
		"	}"	\
		"	else"	\
		"	{"	\
		"		return true;"	\
		"	}"	\
		"}\n"	\
		"float screenSphereSize(vec4 p1, vec4 p2)\n"	\
		"{\n"	\
		"	vec4 viewCenter = (p1+p2) * 0.5;\n"	\
		"	vec4 viewUp = viewCenter;\n"	\
		"	viewUp.y += distance(p1,p2);\n"	\
		"	vec4 p1Proj = viewCenter;\n"	\
		"	vec4 p2Proj = viewUp;\n"	\
		"	//vec4 p1Proj = u_projection_matrix * viewCenter;\n"	\
		"	//vec4 p2Proj = u_projection_matrix * viewUp;\n"	\
		"	vec4 p1NDC, p2NDC;\n"	\
		"	p1NDC = p1Proj/p1Proj.w;\n"	\
		"	p2NDC = p2Proj/p2Proj.w;\n"	\
		"	return( clamp(length((p2NDC.xy - p1NDC.xy) * viewportDim * 0.5) / (pixelsPerEdge), 1.0, patchSize));\n"	\
		"}\n"	\
		"float getRoughness(vec2 disp)\n"	\
		"{\n"	\
		"	return (pow(( 1.8 - texture(roughFactor, posV[0]+ disp /textureSize(roughFactor,0)).x ),4));\n"	\
		"}\n"	\
		"void main(void)" \
		"{" \
		"	mat4 pvm = u_projection_matrix*u_view_matrix*u_model_matrix;\n"	\
		"	int scaleF;\n"	\
		"	if(scaleFactor==0)"	\
		"	{"	\
		"		scaleF = 1;"	\
		"	}"	\
		"	else"	\
		"	{"	\
		"		scaleF = scaleFactor;"	\
		"	}"	\
		"	vec2 iLevel;\n"	\
		"	vec4 oLevel;\n"	\
		"	vec4 posTransV[4];\n"	\
		"	vec2 pAux;\n"	\
		"	vec2 posTCAux[4];\n"	\
		"	ivec2 tSize = textureSize(heightMap,0) * scaleF;\n"	\
		"	float div = patchSize / tSize.x;\n"	\
		"	posTC[gl_InvocationID] = posV[gl_InvocationID];\n"	\
		"	posTCAux[0] = posV[0];\n"	\
		"	posTCAux[1] = posV[0] + vec2(0.0, div);\n"	\
		"	posTCAux[2] = posV[0] + vec2(div,0.0);\n"	\
		"	posTCAux[3] = posV[0] + vec2(div,div);\n"	\
		"	pAux = posTCAux[0] * tSize * gridSpacing;\n"	\
		"	posTransV[0] = pvm * vec4(pAux[0], height(posTCAux[0].x,posTCAux[0].y), pAux[1], 1.0);\n"	\
		"	pAux = posTCAux[1] * tSize * gridSpacing;\n"	\
		"	posTransV[1] = pvm * vec4(pAux[0], height(posTCAux[1].x,posTCAux[1].y), pAux[1], 1.0);\n"	\
		"	pAux = posTCAux[2] * tSize * gridSpacing;\n"	\
		"	posTransV[2] = pvm * vec4(pAux[0], height(posTCAux[2].x,posTCAux[2].y), pAux[1], 1.0);\n"	\
		"	pAux = posTCAux[3] * tSize * gridSpacing;\n"	\
		"	posTransV[3] = pvm * vec4(pAux[0], height(posTCAux[3].x,posTCAux[3].y), pAux[1], 1.0);\n"	\
		"	if (culling == 0||(segmentInFrustum(posTransV[gl_InvocationID], posTransV[gl_InvocationID+1])||segmentInFrustum(posTransV[gl_InvocationID],posTransV[gl_InvocationID+2])||segmentInFrustum(posTransV[gl_InvocationID+2],posTransV[gl_InvocationID+3])||segmentInFrustum(posTransV[gl_InvocationID+3],posTransV[gl_InvocationID+1])))"	\
		"	{"	\
		"		if (useRoughness == 1)"	\
		"		{"	\
		"			float roughness[4];\n"	\
		"			float roughnessForCentralPatch = getRoughness(vec2(0.5));\n"	\
		"			roughness[0] = max(roughnessForCentralPatch, getRoughness(vec2(-0.5,0.5)));\n"	\
		"			roughness[1] = max(roughnessForCentralPatch, getRoughness(vec2(0.5,-0.5)));\n"	\
		"			roughness[2] = max(roughnessForCentralPatch, getRoughness(vec2(1.5,0.5)));\n"	\
		"			roughness[3] = max(roughnessForCentralPatch, getRoughness(vec2(0.5,1.5)));\n"	\
		"			oLevel =vec4(clamp(screenSphereSize(posTransV[gl_InvocationID], posTransV[gl_InvocationID+1]) * roughness[0],1,patchSize),"	\
		"			clamp(screenSphereSize(posTransV[gl_InvocationID+0], posTransV[gl_InvocationID+2]) * roughness[1],1,patchSize),"	\
		"			clamp(screenSphereSize(posTransV[gl_InvocationID+2], posTransV[gl_InvocationID+3]) * roughness[2],1,patchSize),"	\
		"			clamp(screenSphereSize(posTransV[gl_InvocationID+3], posTransV[gl_InvocationID+1]) * roughness[3],1,patchSize));\n"	\
		"			iLevel=vec2(max(oLevel[1],oLevel[3]),max(oLevel[0],oLevel[2]));\n"	\
		"			\n"	\
		"		}\n"	\
		"		else if (useRoughness == 0)"	\
		"		{"	\
		"			oLevel = vec4(screenSphereSize(posTransV[gl_InvocationID], posTransV[gl_InvocationID+1]),"	\
		"			screenSphereSize(posTransV[gl_InvocationID+0],posTransV[gl_InvocationID+2]),"	\
		"			screenSphereSize(posTransV[gl_InvocationID+2],posTransV[gl_InvocationID+3]),"	\
		"			screenSphereSize(posTransV[gl_InvocationID+3],posTransV[gl_InvocationID+1]));\n"	\
		"			iLevel = vec2(max(oLevel[1] , oLevel[3]) , max(oLevel[0] , oLevel[2]) );\n"	\
		"		}"	\
		"		else"	\
		"		{"	\
		"			oLevel = vec4(patchSize);\n"	\
		"			iLevel = vec2(patchSize);\n"	\
		"		}"	\
		"	}"	\
		"	else if(culling == 1)"	\
		"	{"	\
		"		oLevel = vec4(0);\n"	\
		"		iLevel = vec2(0);\n"	\
		"	}"	\
		"	else"	\
		"	{"	\
		"		oLevel = vec4(patchSize);\n"	\
		"		iLevel = vec2(patchSize);\n"	\
		"	}"	\
		"	gl_TessLevelOuter[0] = oLevel[0];\n"	\
		"	gl_TessLevelOuter[1] = oLevel[1];\n"	\
		"	gl_TessLevelOuter[2] = oLevel[2];\n"	\
		"	gl_TessLevelOuter[3] = oLevel[3];\n"	\
		"	gl_TessLevelInner[0] = iLevel[0];\n"	\
		"	gl_TessLevelInner[1] = iLevel[1];\n"	\
		"}";

	glShaderSource(iTESControlShaderObject, 1, (const GLchar**)&tessControlShaderSourceCode, NULL);

	// Compile it
	glCompileShader(iTESControlShaderObject);
	iInfoLogLength = 0;
	iShaderCompileStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(iTESControlShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(iTESControlShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(iTESControlShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile, "ERROR : Tessrlation Control Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				return INIT_VERTEX_SHADER_COMPILATION_FAILED;
				//DestroyWindow(g_hwnd);
				//exit(EXIT_FAILURE);
			}
		}
	}

	/*Tess control Shader End*/

	/*Tess Eval Shader Start*/
	iTESEvaluationShaderObject = glCreateShader(GL_TESS_EVALUATION_SHADER);
	const GLchar *tessEvalShaderSourceCode = 
		"#version 450 core"	\
		"\n" \
		"layout(quads, fractional_even_spacing, cw) in;\n" \
		"uniform mat4 u_model_matrix;\n" \
		"uniform mat4 u_view_matrix;\n" \
		"uniform mat4 u_projection_matrix;\n" \
		"layout (binding =0)uniform sampler2D heightMap;\n" \
		"uniform float heightStep;\n" \
		"uniform float gridSpacing;\n" \
		"uniform int scaleFactor;\n" \
		"uniform float patchSize;\n" \
		"in vec2 posTC[];\n" \
		"out vec2 uvTE;\n" \
		"out vec4 finalPos;\n"	\
		"float height(float u, float v)\n" \
		"{\n" \
		"	return (texture(heightMap, vec2(u,v)).r * heightStep);\n" \
		"}\n" \
		"void main(void)" \
		"{" \
		"	ivec2 tSize = textureSize(heightMap,0) * scaleFactor;\n"	\
		"	float div = tSize.x * (1.0/patchSize);\n"	\
		"	uvTE = posTC[0].xy + (gl_TessCoord.st/div);\n"	\
		"	vec4 res;\n"	\
		"	res.x = uvTE.s * tSize.x * gridSpacing;\n"	\
		"	res.z = uvTE.t * tSize.y * gridSpacing;\n"	\
		"	res.y = height(uvTE.s, uvTE.t);\n"	\
		"	res.w = 1.0;\n"	\
		"	gl_Position= u_projection_matrix*u_view_matrix*u_model_matrix*res;\n"	\
		"	//finalPos = gl_Position;\n"	\
		"}";

	glShaderSource(iTESEvaluationShaderObject, 1, (const GLchar**)&tessEvalShaderSourceCode, NULL);

	// Compile it
	glCompileShader(iTESEvaluationShaderObject);
	iInfoLogLength = 0;
	iShaderCompileStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(iTESEvaluationShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(iTESEvaluationShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(iTESEvaluationShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile, "ERROR : Tess Eval Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				return INIT_VERTEX_SHADER_COMPILATION_FAILED;
				//DestroyWindow(g_hwnd);
				//exit(EXIT_FAILURE);
			}
		}
	}
	/*Tess Eval Shader End*/

	/*Geometry Shader Start
	iGeometryShaderObject = glCreateShader(GL_GEOMETRY_SHADER);
	const GLchar *geometryShaderSourceCode = 
		"#version 450 core"	\
		"\n" \
		"layout(triangles)in;\n" \
		"layout(triangle_strip,max_vertices=3)out;\n" \
		"in vec2 uvTE[3];\n" \
		"out vec3 out_gNormal;\n" \
		"out vec2 out_gTexCoord;\n" \
		"uniform mat4 u_model_matrix;\n" \
		"uniform mat4 u_view_matrix;\n" \
		"uniform mat4 u_projection_matrix;\n" \
		"uniform sampler2D normalMap;\n"	\
		"vec3 NormalFromTexture(vec3 normal)\n"	\
		"{\n"	\
		"	vec3 retNormal;\n"	\
		"	retNormal.x = normal.x - 0.5;\n"	\
		"	retNormal.y = normal.y - 0.5;\n"	\
		"	retNormal.z = normal.z - 0.5;\n"	\
		"	retNormal = inverse(transpose(mat3(u_view_matrix*u_model_matrix)))*normalize(retNormal);\n"	\
		"	return retNormal;\n"	\
		"}"	\
		"void main(void)" \
		"{\n" \
		"	out_gNormal = NormalFromTexture(texture(normalMap,uvTE[0]).rgb);\n"	\
		"	out_gTexCoord = uvTE[0];\n"	\
		"	gl_Position = gl_in[0].gl_Position;\n"	\
		"	EmitVertex();\n"	\
		"	out_gNormal = NormalFromTexture(texture(normalMap,uvTE[1]).rgb);\n"	\
		"	out_gTexCoord = uvTE[1];\n"	\
		"	gl_Position = gl_in[1].gl_Position;\n"	\
		"	EmitVertex();\n"	\
		"	out_gNormal = NormalFromTexture(texture(normalMap,uvTE[2]).rgb);\n"	\
		"	out_gTexCoord = uvTE[2];\n"	\
		"	gl_Position = gl_in[2].gl_Position;\n"	\
		"	EmitVertex();\n"	\
		"	EndPrimitive();\n"	\
		"}";

	glShaderSource(iGeometryShaderObject, 1, (const GLchar**)&geometryShaderSourceCode, NULL);

	// Compile it
	glCompileShader(iGeometryShaderObject);
	iInfoLogLength = 0;
	iShaderCompileStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(iGeometryShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iGeometryShaderObject == GL_FALSE)
	{
		glGetShaderiv(iGeometryShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(iGeometryShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile, "ERROR : Geometry Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				return INIT_VERTEX_SHADER_COMPILATION_FAILED;
			}
		}
	}
	/*Geometry Shader End*/

	/*Fragment Shader Start*/
	iFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar *fragmentShaderSourceCode = "#version 450 core"	\
		"\n"	\
		"in vec2 uvTE;\n"	\
		"in vec4 finalPos;\n"	\
		"out vec4 FragColor;\n"	\
		"layout(binding =0)uniform sampler2D heightMap;\n"	\
		"uniform sampler2D texUnit;\n"	\
		"uniform float heightStep;\n"	\
		"uniform float gridSpacing;\n"	\
		"uniform int scaleFactor;\n"	\
		"float height(float u, float v)\n"	\
		"{\n"	\
		"	return (texture(heightMap, vec2(u,v)).r * heightStep);\n"	\
		"}\n"	\
		"\n"	\
		"void main(void)"	\
		"{"	\
		"	vec4 color;\n"	\
		"	float intensity;\n"	\
		"	vec3 lightDir,n;\n"	\
		"	FragColor = vec4(1.0);"	\
		"}";

	glShaderSource(iFragmentShaderObject, 1, (const GLchar**)&fragmentShaderSourceCode, NULL);
	glCompileShader(iFragmentShaderObject);
	iInfoLogLength = 0;
	iShaderCompileStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(iFragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus==GL_FALSE)
	{
		glGetShaderiv(iFragmentShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog!=NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(iFragmentShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf(g_pFile,"ERROR: Fragment Shader Compilation Log : %s \n",szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				return INIT_FRAGMENT_SHADER_COMPILATION_FAILED;
			}
		}
	}
	/*Fragment Shader End*/

	/* Shader Program Start */
	g_ShaderProgramObject_Ter = glCreateProgram();
	glAttachShader(g_ShaderProgramObject_Ter, iVertexShaderObject);
	glAttachShader(g_ShaderProgramObject_Ter, iTESControlShaderObject);
	glAttachShader(g_ShaderProgramObject_Ter, iTESEvaluationShaderObject);
	//glAttachShader(g_ShaderProgramObject_Ter, iGeometryShaderObject);
	glAttachShader(g_ShaderProgramObject_Ter, iFragmentShaderObject);
	glBindAttribLocation(g_ShaderProgramObject_Ter, SAM_ATTRIBUTE_POSITION, "vPosition");
	//glBindAttribLocation(g_ShaderProgramObject_Ter, SAM_ATTRIBUTE_COLOR, "vColor");
	glLinkProgram(g_ShaderProgramObject_Ter);

	GLint iShaderLinkStatus = 0;
	iInfoLogLength = 0;
	glGetProgramiv(g_ShaderProgramObject_Ter, GL_LINK_STATUS, &iShaderLinkStatus);
	if (iShaderLinkStatus==GL_FALSE)
	{
		glGetProgramiv(g_ShaderProgramObject_Ter, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog!=NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(g_ShaderProgramObject_Ter, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile, "ERROR : Linking Shader Program Objects Failed %s \n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				return INIT_LINK_SHADER_PROGRAM_FAILED;
				//DestroyWindow(g_hwnd);
				//exit(EXIT_FAILURE);
			}
		}
	}
	/* Shader Program End */

	/*Setup Uniforms Start*/
	g_Uniform_Model_Matrix = glGetUniformLocation(g_ShaderProgramObject_Ter,"u_model_matrix");
	g_Uniform_Projection_Matrix = glGetUniformLocation(g_ShaderProgramObject_Ter, "u_projection_matrix");
	g_Uniform_View_Matrix = glGetUniformLocation(g_ShaderProgramObject_Ter, "u_view_matrix");

	g_Uniform_HeightStep = glGetUniformLocation(g_ShaderProgramObject_Ter, "heightStep");
	g_Uniform_GridSpacing = glGetUniformLocation(g_ShaderProgramObject_Ter, "gridSpacing");
	g_Uniform_ViewPortDim = glGetUniformLocation(g_ShaderProgramObject_Ter, "viewportDim");
	g_Uniform_Sampler_RoughFactor = glGetUniformLocation(g_ShaderProgramObject_Ter, "roughFactor");
	g_Uniform_Sampler_HeightMap = glGetUniformLocation(g_ShaderProgramObject_Ter, "heightMap");
	g_Uniform_UseRoughness = glGetUniformLocation(g_ShaderProgramObject_Ter, "useRoughness");
	g_Uniform_pixelsPerEdge = glGetUniformLocation(g_ShaderProgramObject_Ter, "pixelsPerEdge");
	g_Uniform_culling = glGetUniformLocation(g_ShaderProgramObject_Ter, "culling");
	g_Uniform_ScaleFactor = glGetUniformLocation(g_ShaderProgramObject_Ter, "scaleFactor");
	g_Uniform_PatchSize = glGetUniformLocation(g_ShaderProgramObject_Ter, "patchSize");
	g_Uniform_Sampler_texUnit = glGetUniformLocation(g_ShaderProgramObject_Ter, "texUnit");
	g_uniform_sampler_NormalMap = glGetUniformLocation(g_ShaderProgramObject_Ter,"normalMap");
	/*Setup Uniforms End*/

	/* Fill Buffers Start*/
	
	/* Fill Buffers End*/
	/// Sam : all Shader Code End

	//float f = 0.1f * g_iScaleFactor * MAX_VALUE_PER_PIXEL;
	//g_fHeightStep = f;

	ilInit();
	//iluInit();
	//ilutInit();
	
	int TWidth, Theight;
	bool status = LoadHeightMaapWithImage(terrainName,&TWidth,&Theight);

	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);

	glClearColor(0.125f, 0.125f, 0.125f, 1.0f);

	g_PersPectiveProjectionMatrix = vmath::mat4::identity();

	Resize(WIN_WIDTH, WIN_HEIGHT);

	//glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

	return INIT_ALL_OK;
}

void Update(void)
{
	/*g_Angle_Pyramid = g_Angle_Pyramid + 0.5f;
	if (g_Angle_Pyramid >= 360.0f)
		g_Angle_Pyramid = 0;*/
}

void Render(void)
{

	TCHAR str[255];
	wsprintf(str, TEXT("OpenGL Programmable Pipeline Window : [ Pixels per edge = %d ]"), g_iPixelsPerEdge);
	SetWindowText(g_hwnd, str);


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	vmath::mat4 modelMatrix = vmath::mat4::identity();
	vmath::mat4 viewMatrix = vmath::mat4::identity();
	vmath::mat4 rotationMatrix = vmath::mat4::identity();
	vmath::mat4 scaleMatrix = vmath::mat4::identity();

	glUseProgram(g_ShaderProgramObject_Ter);
	
	modelMatrix = vmath::translate(0.0f, 0.0f, 0.0f);
	//rotationMatrix = vmath::rotate(g_Angle_Pyramid, 0.0f, 1.0f, 0.0f);
	modelMatrix = modelMatrix * rotationMatrix;
	glPatchParameteri(GL_PATCH_VERTICES, 1);
	glUniformMatrix4fv(g_Uniform_Model_Matrix, 1, GL_FALSE, modelMatrix);
	//glUniformMatrix4fv(g_Uniform_View_Matrix, 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(g_Uniform_View_Matrix, 1, GL_FALSE, camera.GetViewMatrix());
	glUniformMatrix4fv(g_Uniform_Projection_Matrix, 1, GL_FALSE, g_PersPectiveProjectionMatrix);

	glUniform1i(g_Uniform_ScaleFactor, g_iScaleFactor);
	glUniform1f(g_Uniform_PatchSize, PATCHSIZE*1.0f);
	glUniform1i(g_Uniform_culling, g_iCulling);
	glUniform1i(g_Uniform_pixelsPerEdge, g_iPixelsPerEdge);
	glUniform1i(g_Uniform_UseRoughness, g_iRoughnessMode);
	glUniform2iv(g_Uniform_ViewPortDim, 1,vmath::ivec2((int)g_fCurrrentWidth, (int)g_fCurrrentHeight));
	glUniform1f(g_Uniform_GridSpacing, g_fGridSpacing);

	/*float f = 0.1f * g_iScaleFactor * 256;
	g_fHeightStep = f;*/


	glUniform1f(g_Uniform_HeightStep, g_fHeightStep);
	

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,g_Texture_HeightMap);
	glUniform1i(g_Uniform_Sampler_HeightMap, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_Texture_HeightMap);
	glUniform1i(g_Uniform_Sampler_RoughFactor, 1);


	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, g_Texture_HeightMap);
	glUniform1i(g_Uniform_Sampler_texUnit, 2);

	glBindVertexArray(g_VertexArrayObject_ter);
	glDrawArrays(GL_PATCHES,0,iNumIndices);
	glBindVertexArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

	SwapBuffers(g_hdc);
}

void FullScreen(void)
{
	MONITORINFO mi = { sizeof(mi) };
	dwStyle = GetWindowLong(g_hwnd, GWL_STYLE);
	if (g_bFullScreen == false)
	{
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			if (GetWindowPlacement(g_hwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(g_hwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(g_hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(g_hwnd, HWND_TOP,
					mi.rcMonitor.left, mi.rcMonitor.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}
		ShowCursor(FALSE);
		g_bFullScreen = true;
	}
	else
	{
		SetWindowLong(g_hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(g_hwnd, &wpPrev);
		SetWindowPos(g_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
		ShowCursor(TRUE);
		g_bFullScreen = false;
	}
}

bool Resize(int iWidth, int iHeight)
{
	if (iHeight <= 0)
	{
		iHeight = 1;
	}

	glViewport(0, 0, (GLsizei)iWidth, (GLsizei)iHeight);

	g_PersPectiveProjectionMatrix = vmath::perspective(45.0f, (float)iWidth / (float)iHeight, 0.1f, 400000.0f);

	return true;
}

int UnInitialize(void)
{
	if (g_bFullScreen == true)
	{
		SetWindowLong(g_hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(g_hwnd, &wpPrev);
		SetWindowPos(g_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
		ShowCursor(TRUE);
		g_bFullScreen = false;
	}

	if (g_iHeightsRef)
	{
		free(g_iHeightsRef);
		g_iHeightsRef = NULL;
	}

	if (g_VertexBufferObject_Color_ter)
	{
		glDeleteBuffers(1, &g_VertexBufferObject_Color_ter);
		g_VertexBufferObject_Color_ter = NULL;
	}

	if (g_VertexBufferObject_Position_ter)
	{
		glDeleteBuffers(1, &g_VertexBufferObject_Position_ter);
		g_VertexBufferObject_Position_ter = NULL;
	}

	if (g_VertexArrayObject_ter)
	{
		glDeleteVertexArrays(1, &g_VertexArrayObject_ter);
		g_VertexArrayObject_ter = NULL;
	}

	glUseProgram(0);

	if (g_ShaderProgramObject_Ter)
	{
		GLsizei iShaderCount;
		GLsizei iShaderNumber;


		glUseProgram(g_ShaderProgramObject_Ter);
		glGetProgramiv(g_ShaderProgramObject_Ter,GL_ATTACHED_SHADERS,&iShaderCount);
		GLuint *pShaders = (GLuint*) calloc(iShaderCount,sizeof(GLuint));

		if (pShaders)
		{
			glGetAttachedShaders(g_ShaderProgramObject_Ter, iShaderCount,&iShaderCount, pShaders);
			for (iShaderNumber = 0; iShaderNumber < iShaderCount; iShaderNumber++)
			{
				glDetachShader(g_ShaderProgramObject_Ter,pShaders[iShaderNumber]);
				glDeleteShader(pShaders[iShaderNumber]);
				pShaders[iShaderNumber] = 0;
			}
			free(pShaders);
			pShaders = NULL;
		}

		glDeleteProgram(g_ShaderProgramObject_Ter);
		g_ShaderProgramObject_Ter = NULL;

		glUseProgram(0);

	}


	if (wglGetCurrentContext() == g_hrc)
	{
		wglMakeCurrent(NULL, NULL);
	}

	if (g_hrc)
	{
		wglDeleteContext(g_hrc);
		g_hrc = NULL;
	}

	if (g_hdc)
	{
		ReleaseDC(g_hwnd, g_hdc);
		g_hdc = NULL;
	}


	if (g_pFile)
	{
		fprintf_s(g_pFile, "Closing File \n");
		fclose(g_pFile);
		g_pFile = NULL;
	}
	return 0;
}


/**		Load Terrain Height map Start	**/
/**
Loads Following things

// Geometry
1. Vertices
2. Indices

// Textures
3. Height Map of Terrain
4. Roughness(diffs)


**/
bool LoadHeightMaapWithImage(std::string heightMap,int *gridSizeX,int *gridSizeZ)
{

	float getHeight(int i,int j);
	void terCrossProduct(float *a, float *b, float *res);
	void terNormalize(float *a);
	float terDotProduct(float *a, float *b);


	ILboolean success;
	
	//int iGrigSizeX = 0, iGrigSizeZ = 0;
	unsigned int imageID;

	ilGenImages(1, &imageID);
	// Load Height Map
	ilBindImage(imageID); /* Binding of DevIL image name */
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
	success = ilLoadImage((ILstring)heightMap.c_str());

	if (!success)
	{
		fprintf_s(g_pFile,"Could not find height map => %s\n", heightMap.c_str());
		ilDeleteImages(1, &imageID);
		return false;
	}

	/* Convert image to LUMINANCE - 16 bits */
	ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_SHORT);
	g_iGridSizeX = ilGetInteger(IL_IMAGE_WIDTH);
	g_iGridSizeZ = ilGetInteger(IL_IMAGE_HEIGHT);

	*gridSizeZ = g_iGridSizeZ;
	*gridSizeX = g_iGridSizeX;

	unsigned char *heightsAux = (unsigned char *)ilGetData();

	g_iHeightsRef = (unsigned short *)malloc(sizeof(unsigned short) * g_iGridSizeX * g_iGridSizeZ);
	memcpy(g_iHeightsRef, heightsAux, sizeof(unsigned short) * g_iGridSizeX * g_iGridSizeZ);

	if (g_fHeightStep == 0.0f)
		g_fHeightStep = 0.005f;

	for (size_t i = 0; i < g_iGridSizeX; i++)
	{
		for (size_t j = 0; j < g_iGridSizeZ; j++)
		{
			fprintf_s(g_pFile,"g_iHeightsRef = %d, heightsAux = %d\n", g_iHeightsRef[i*g_iGridSizeZ + j], heightsAux[i*g_iGridSizeZ + j]);
		}

	}

	/* Create and load heighmap texture to OpenGL */
	glGenTextures(1, &g_Texture_HeightMap); /* Texture name generation */
	glBindTexture(GL_TEXTURE_2D, g_Texture_HeightMap);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// GL_COMPRESSED_RED_RGTC1 <=> GL_16
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16,
		g_iGridSizeX, g_iGridSizeZ,0, GL_RED, GL_UNSIGNED_SHORT,g_iHeightsRef);
	glBindTexture(GL_TEXTURE_2D, 0);

	g_iGridSizeX = (int)(g_iGridSizeX*g_iScaleFactor);
	g_iGridSizeZ = (int)(g_iGridSizeZ*g_iScaleFactor);

	if (g_fHeightStep == 0.0f)
	{
		g_fHeightStep = 0.005f;
	}

	int patchSize = 1;
	int nopX = (g_iGridSizeX / PATCHSIZE);
	int nopZ = (g_iGridSizeZ / PATCHSIZE);
	int numPatches = nopX * nopZ;


	// Compute Roughness
	float difH;
	float *fRoughness = (float *)malloc(sizeof(float) * nopX * nopZ);

	// First compute the average normal of the patch
	float avgN[3], n0[3], n1[3], n2[3], n3[3];
	float p0[3], p1[3], p2[3], p3[3];
	float max = 0.0f, min = 1.0f;
	for (int i = 0; i < nopZ; ++i) 
	{
		for (int j = 0; j < nopX; ++j) 
		{

			p0[0] = 0.0f;
			p0[1] = getHeight(i*PATCHSIZE, j*PATCHSIZE);
			p0[2] = 0.0f;

			p1[0] = 0.0f;
			p1[1] = getHeight((i + 1)*PATCHSIZE, j*PATCHSIZE);
			p1[2] = g_fGridSpacing*PATCHSIZE;

			p2[0] = g_fGridSpacing*PATCHSIZE;
			p2[1] = getHeight(i*PATCHSIZE, (j + 1)*PATCHSIZE);
			p2[2] = 0.0f;

			p3[0] = g_fGridSpacing*PATCHSIZE;
			p3[1] = getHeight((i + 1)*PATCHSIZE, (j + 1)*PATCHSIZE);
			p3[2] = g_fGridSpacing*PATCHSIZE;

			float p01[3], p02[3], p13[3], p23[3];

			p01[0] = p1[0] - p0[0];
			p01[1] = p1[1] - p0[1];
			p01[2] = p1[2] - p0[2];

			p02[0] = p2[0] - p0[0];
			p02[1] = p2[1] - p0[1];
			p02[2] = p2[2] - p0[2];

			p13[0] = p3[0] - p1[0];
			p13[1] = p3[1] - p1[1];
			p13[2] = p3[2] - p1[2];

			p23[0] = p3[0] - p2[0];
			p23[1] = p3[1] - p2[1];
			p23[2] = p3[2] - p2[2];

			
			terCrossProduct(p01, p02, n0);
			terCrossProduct(p23, p02, n2);
			terCrossProduct(p23, p13, n3);
			terCrossProduct(p01, p13, n1);

			terNormalize(n0);
			terNormalize(n1);
			terNormalize(n2);
			terNormalize(n3);

			avgN[0] = n0[0] + n1[0] + n2[0] + n3[0];
			avgN[1] = n0[1] + n1[1] + n2[1] + n3[1];
			avgN[2] = n0[2] + n1[2] + n2[2] + n3[2];

			terNormalize(avgN);

			// Then compute the normal at each point and accumulate its difference to the average

			difH = 0.0f;
			float normal[3], deltaHi, deltaHj;
			float maxDif = 0.0f, minDif = 1.0f;
			for (int k = 0; k < PATCHSIZE; ++k)
				for (int l = 0; l < PATCHSIZE; ++l) {

					deltaHi = getHeight(j*PATCHSIZE + k, i*PATCHSIZE + l + 1) - getHeight(j*PATCHSIZE + k, i*PATCHSIZE + l - 1);
					deltaHj = getHeight(j*PATCHSIZE + k + 1, i*PATCHSIZE + l) - getHeight(j*PATCHSIZE + k - 1, i*PATCHSIZE + l);

					normal[0] = -2 * g_fGridSpacing * deltaHi;
					normal[1] = 4 * g_fGridSpacing * g_fGridSpacing;
					normal[2] = -2 * g_fGridSpacing * deltaHj;

					terNormalize(normal);
					float aux = terDotProduct(normal, avgN);
					aux *= aux;
					if (aux < minDif) minDif = aux;
					if (aux > maxDif) maxDif = aux;
					difH += aux;


				}
			fRoughness[i*nopX + j] = minDif;//difH / (64*64);
			if (fRoughness[i*nopX+j]>max)max=fRoughness[i*nopX+j];
			if (fRoughness[i*nopX+j]<min)min=fRoughness[i*nopX+j];
		}
	}


	glGenTextures(1, &g_Texture_Roughness); 
	glBindTexture(GL_TEXTURE_2D, g_Texture_Roughness);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F,
		nopX, nopZ,
		0, GL_RED, GL_FLOAT,
		fRoughness);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Calculate and fill and vertices
	iNumIndices = numPatches*patchSize;

	float *vertices = (float*)malloc(sizeof(float) * 2 * iNumIndices);
	unsigned int  *indices = (unsigned int *)malloc(sizeof(unsigned int) * numPatches * patchSize);

	int patchNumber;
	for (int i = 0; i < nopX; ++i) {

		for (int j = 0; j < nopZ; ++j) {

			patchNumber = i * nopZ + j;

			vertices[(patchNumber * patchSize) * 2] = (i * PATCHSIZE) * 1.0f / g_iGridSizeX;
			vertices[(patchNumber * patchSize) * 2 + 1] = (j * PATCHSIZE) * 1.0f / g_iGridSizeZ;

		}
	}

	for (int i = 0; i < iNumIndices; ++i) 
	{
		indices[i] = i;
	}


	// Fill Data to OpenGL
	glGenVertexArrays(1,&g_VertexArrayObject_ter);
	glBindVertexArray(g_VertexArrayObject_ter);

	glGenBuffers(1,&g_VertexBufferObject_Position_ter);
	glBindBuffer(GL_ARRAY_BUFFER, g_VertexBufferObject_Position_ter);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(float) * (iNumIndices) * 2,
		vertices,
		GL_STATIC_DRAW);

	glEnableVertexAttribArray(SAM_ATTRIBUTE_POSITION);
	glVertexAttribPointer(SAM_ATTRIBUTE_POSITION, 2, GL_FLOAT, 0, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	ilDeleteImages(1, &imageID);
	if (vertices)
	{
		free(vertices);
		vertices = NULL;
	}

	if (indices)
	{
		free(indices);
		indices = 0;
	}
	return true;

}

float getHeight(int i, int j)
{
	int iAux, jAux;

	iAux = (i / g_iScaleFactor);
	jAux = (j / g_iScaleFactor);

	if (iAux < 0)
		iAux = 0;
	else if (iAux >= (int)(g_iGridSizeX / g_iScaleFactor))
		iAux = (g_iGridSizeX / g_iScaleFactor) - 1;

	if (jAux < 0)
		jAux = 0;
	else if (jAux >= (int)(g_iGridSizeZ / g_iScaleFactor))
		jAux = (g_iGridSizeZ / (int)g_iScaleFactor) - 1;

	return(g_iHeightsRef[(int)(iAux * g_iGridSizeX / g_iScaleFactor) + jAux] * g_fHeightStep / 65536);
}

void terCrossProduct(float *a, float *b, float *res)
{
	res[0] = a[1] * b[2] - b[1] * a[2];
	res[1] = a[2] * b[0] - b[2] * a[0];
	res[2] = a[0] * b[1] - b[0] * a[1];
}

float terDotProduct(float *a, float *b)
{
	float res = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

	return res;
}

// Normalize a vec3
void terNormalize(float *a)
{
	float mag = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);

	a[0] /= mag;
	a[1] /= mag;
	a[2] /= mag;
}
/**		Load Terrain Height map Stop	**/