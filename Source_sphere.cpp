#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>

#include<gl\glew.h>
#include<gl\GL.h>

#include "resource.h"
#include"vmath.h"


#define WIN_WIDTH	800
#define WIN_HEIGHT	600

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")

//using namespace std;

enum InitErrorCodes
{
	INIT_VERTEX_SHADER_COMPILATION_FAILED = -9,
	INIT_FRAGMENT_SHADER_COMPILATION_FAILED,
	INIT_LINK_SHADER_PROGRAM_FAILED,
	INIT_FAIL_GLEW_INIT ,
	INIT_FAIL_BRIDGE_CONTEX_SET,
	INIT_FAIL_BRIDGE_CONTEX_CREATION,
	INIT_FAIL_SET_PIXEL_FORMAT,
	INIT_FAIL_NO_PIXEL_FORMAT,
	INIT_FAIL_NO_HDC,
	INIT_ALL_OK,
};

enum attributeBindLocations
{
	SAM_ATTRIBUTE_POSITION = 0,
	SAM_ATTRIBUTE_COLOR,
	SAM_ATTRIBUTE_NORMAL,
	SAM_ATTRIBUTE_TEXTURE0,
};


LRESULT CALLBACK MainWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

bool g_bWindowActive = false;
bool g_bWireframe = false;
HWND g_hwnd = NULL;
HDC  g_hdc = NULL;
HGLRC g_hrc = NULL;

WINDOWPLACEMENT wpPrev;
DWORD dwStyle;
bool g_bFullScreen = false;

FILE *g_pFile = NULL;

// Shaders
GLuint g_VertexShaderObject_Sphere;
GLuint g_FragmentShaderObject_Sphere;
GLuint g_ShaderProgramObject_Sphere;

// All Vertex Buffers
GLuint g_VertexArrayObject_Sphere = 0;
GLuint g_VertexBufferObject_Sphere_Position = 0;
GLuint g_VertexBufferObject_Sphere_Normal = 0;
GLuint g_VertexBufferObject_Sphere_Texture = 0;
GLuint g_VertexBufferObject_Sphere_Element = 0;

GLuint g_iNumElements = 0;
GLuint g_iMaxElements = 0;
GLuint g_iNumVertices = 0;

// Sphere Functions Start
GLushort *g_iElements;
GLfloat  *g_fVerts;
GLfloat  *g_fNorms;
GLfloat  *g_fTexCoords;

// Uniforms
GLuint g_Uniform_Model_Matrix_Sphere = 0;
GLuint g_Uniform_View_Matrix_Sphere = 0;
GLuint g_Uniform_Projection_Matrix_Sphere = 0;
GLuint g_Uniform_LightPosition_Sphere = 0;
GLuint g_Uniform_Earth_Day = 0;
GLuint g_Uniform_Earth_Night = 0;
GLuint g_Uniform_Earth_Cloud = 0;
GLuint g_Uniform_Earth_Gloss = 0;
// sampler
GLuint g_Texture_Earth_Night;
GLuint g_Texture_Earth_Day;
GLuint g_Texture_Earth_Cloud;
GLuint g_Texture_Earth_Gloss;

// Animation
float g_RotateSphere = -150.0f;

// Projection
vmath::mat4 g_PersPectiveProjectionMatrix;



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
		TEXT("Orange Book : MultiTexturing"),
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

		case 0x46: // 'f' or 'F'
			//MessageBox(hwnd, TEXT("F is pressed"), TEXT("Status"), MB_OK);
			FullScreen();
			break;

		case 0x57:
			if (g_bWireframe == true)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
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

	case WM_SIZE:
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

	int LoadGLTextures(GLuint *texture, TCHAR imageResourceId[],short sOffset);
	void UnInitialize(void);
	GLuint GetIndexCountSphere();
	GLuint GetVertexCountSphere();
	void MakeSphere(GLfloat fRadius, GLint iSlices, GLint iStacks);

	// Check Max Texture Units
	int iMaxTextureUnits = 0;

	int iPixelIndex = 0;
	PIXELFORMATDESCRIPTOR pfd;

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
	
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &iMaxTextureUnits);
	fprintf_s(g_pFile, "SHADER_INFO : Max Texture Units : %d\n", iMaxTextureUnits);

	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &iMaxTextureUnits);
	fprintf_s(g_pFile, "SHADER_INFO : Max Combined Texture Image Units : %d\n", iMaxTextureUnits);
	//fprintf_s(g_pFile, "SHADER_INFO : Extention is : %s \n", glGetString(GL_EXTENSIONS)); 
	// GL information End

	/// Sam : all Shader Code Start

	g_VertexShaderObject_Sphere = glCreateShader(GL_VERTEX_SHADER);

	// give source code to shader
	const GLchar *vertexShaderSourceCode = "#version 450 core"	\
		"\n" \
		"layout(location=0)in vec4 vPosition;\n"	\
		"layout(location=2)in vec3 vNormal;\n"	\
		"layout(location=3)in vec2 vTexture0_Coord;\n"	\

		"uniform mat4 u_model_matrix;\n"	\
		"uniform mat4 u_view_matrix;\n"	\
		"uniform mat4 u_projection_matrix;\n"	\
		"uniform vec3 u_LightPosition;\n"	\

		"layout(location=0)out float Diffuse;\n"	\
		"layout(location=1)out vec3 Specular;\n"	\
		"layout(location=2)out vec2 TexCoord;\n"	\

		"void main (void)"	\
		"{\n"	\
		"	mat4 MVMatrix		= u_view_matrix * u_model_matrix;\n"	\
		"	mat4 MVPMatrix		= u_projection_matrix * u_view_matrix * u_model_matrix;\n"	\
		"	mat3 NormalMatrix	= inverse(transpose(mat3(MVMatrix)));\n"	\

		"	vec3 ecPosition		= vec3(MVMatrix * vPosition);\n"	\
		"	vec3 tnorm			= normalize( NormalMatrix * vNormal );\n"	\
		"	vec3 lightVec		= normalize( u_LightPosition - ecPosition );\n"	\
		"	vec3 reflectVec		= reflect(-lightVec,tnorm);\n"	\
		"	vec3 viewVec		= normalize(-ecPosition);\n"	\
		"	float spec			= clamp(dot(reflectVec,viewVec),0.0f,1.0f);\n"	\
		"	spec				= pow(spec,8.0f);\n"	\
		"	Specular			= spec * vec3(1.0,0.941,0.898) * 0.3f;"	\
		"	Diffuse				= max(dot(lightVec,tnorm),0.0f);"	\
		"	TexCoord			= vTexture0_Coord.st;\n"	\
		"	gl_Position			= MVPMatrix * vPosition;\n"	\
		"}";
	glShaderSource(g_VertexShaderObject_Sphere, 1, (const GLchar**)&vertexShaderSourceCode, NULL);

	// Compile Source Code
	glCompileShader(g_VertexShaderObject_Sphere);
	GLint iInfoLogLength = 0;
	GLint iShaderCompileStatus = 0;
	char *szInfoLog = NULL;
	glGetShaderiv(g_VertexShaderObject_Sphere, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(g_VertexShaderObject_Sphere, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(g_VertexShaderObject_Sphere, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile, "Error : Vertex Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				return INIT_VERTEX_SHADER_COMPILATION_FAILED;
			}

		}

	}

	//***** Fragment Shader *****
	g_FragmentShaderObject_Sphere = glCreateShader(GL_FRAGMENT_SHADER);

	const GLchar *fragmentShaderSourceCode = "#version 450 core"	\
		"\n"	\
		"layout(location=0)in float Diffuse;\n"	\
		"layout(location=1)in vec3  Specular;\n"	\
		"layout(location=2)in vec2  TexCoord;\n"	\

		"layout(location=0)out vec4 FragColor;\n"	\

		"layout(binding=0)uniform sampler2D EarthTextureDay;\n"	\
		"layout(binding=1)uniform sampler2D EarthTextureNight;\n"	\
		"layout(binding=2)uniform sampler2D EarthTextureCloud;\n"	\
		"layout(binding=3)uniform sampler2D EarthTextureGloss;\n"	\

		"void main (void)" \
		"{"	\
		"	vec4 clouds		= texture(EarthTextureCloud,TexCoord);\n"	\
		"	vec4 gloss		= texture(EarthTextureGloss,TexCoord);\n"	\
		"	vec3 dayTime	= (texture(EarthTextureDay,TexCoord).rgb * Diffuse + Specular * gloss.rgb) * (1.0 - clouds.rgb) + clouds.rgb * Diffuse;;\n"	\
		"	vec3 nightTime  = texture(EarthTextureNight,TexCoord).rgb * (1.0 - clouds.rgb) * 2.0f;\n"	\
		"	vec3 color		= dayTime;\n"	\
		"	if(Diffuse < 0.1);\n"	\
		"	{;\n"	\
		"		color = mix(nightTime,dayTime,(Diffuse + 0.1)*1.0f);\n"	\
		"	};\n"	\
		"	FragColor = vec4(color,1.0f);\n"	\
		"}";
	glShaderSource(g_FragmentShaderObject_Sphere, 1, (const GLchar **)&fragmentShaderSourceCode, NULL);

	//"	vec3 dayTime	= (texture(EarthTextureDay,TexCoord).rgb * Diffuse + Specular * gloss.b) * (1.0 - clouds.b) + clouds.b * Diffuse;;\n"

	// Compile Source Code
	glCompileShader(g_FragmentShaderObject_Sphere);
	iInfoLogLength = 0;
	iShaderCompileStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(g_FragmentShaderObject_Sphere, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(g_FragmentShaderObject_Sphere, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(g_FragmentShaderObject_Sphere, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile, "Error : Fragment Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				return INIT_FRAGMENT_SHADER_COMPILATION_FAILED;
			}
		}
	}

	//***** Shader Program *****
	// Create
	g_ShaderProgramObject_Sphere = glCreateProgram();
	// Attach Vertex Shader
	glAttachShader(g_ShaderProgramObject_Sphere, g_VertexShaderObject_Sphere);
	// Attach Fragment Shader
	glAttachShader(g_ShaderProgramObject_Sphere, g_FragmentShaderObject_Sphere);
	// pre-link Program object with Vertex Sahder position attribute
	glBindAttribLocation(g_ShaderProgramObject_Sphere, SAM_ATTRIBUTE_POSITION, "vPosition");
	glBindAttribLocation(g_ShaderProgramObject_Sphere, SAM_ATTRIBUTE_NORMAL, "vNormal");
	glBindAttribLocation(g_ShaderProgramObject_Sphere, SAM_ATTRIBUTE_TEXTURE0, "vTexture0_Coord");
	// link Shader 
	glLinkProgram(g_ShaderProgramObject_Sphere);

	GLint iShaderProgramLinkStatus = 0;
	glGetProgramiv(g_ShaderProgramObject_Sphere, GL_LINK_STATUS, &iShaderProgramLinkStatus);
	szInfoLog = NULL;
	iInfoLogLength = 0;
	if (iShaderProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(g_ShaderProgramObject_Sphere, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(g_ShaderProgramObject_Sphere, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile, "Error : Shader Program Link Log : %s \n", szInfoLog);
				free(szInfoLog);
				return INIT_LINK_SHADER_PROGRAM_FAILED;
			}
		}
	}

	// get all uniforms in OpenGL code
	g_Uniform_Model_Matrix_Sphere = glGetUniformLocation(g_ShaderProgramObject_Sphere, "u_model_matrix");
	g_Uniform_View_Matrix_Sphere = glGetUniformLocation(g_ShaderProgramObject_Sphere, "u_view_matrix");
	g_Uniform_Projection_Matrix_Sphere = glGetUniformLocation(g_ShaderProgramObject_Sphere, "u_projection_matrix");
	g_Uniform_LightPosition_Sphere = glGetUniformLocation(g_ShaderProgramObject_Sphere, "u_LightPosition");
	g_Uniform_Earth_Day = glGetUniformLocation(g_ShaderProgramObject_Sphere, "EarthTextureDay");
	g_Uniform_Earth_Night = glGetUniformLocation(g_ShaderProgramObject_Sphere, "EarthTextureNight");
	g_Uniform_Earth_Cloud = glGetUniformLocation(g_ShaderProgramObject_Sphere, "EarthTextureCloud");
	g_Uniform_Earth_Gloss = glGetUniformLocation(g_ShaderProgramObject_Sphere, "EarthTextureGloss");
	
	// **** Verttices, Colors, Shader Attribs, Vbo, Vao Initializations ****
	MakeSphere(1.0f, 32, 32);
	int index = GetIndexCountSphere();
	int vertex = GetVertexCountSphere();

	LoadGLTextures(&g_Texture_Earth_Day, MAKEINTRESOURCE(IDB_EARTH_DAY),0);
	LoadGLTextures(&g_Texture_Earth_Night, MAKEINTRESOURCE(IDB_EARTH_NIGHT),1);
	LoadGLTextures(&g_Texture_Earth_Cloud, MAKEINTRESOURCE(IDB_EARTH_CLOUD),2);
	LoadGLTextures(&g_Texture_Earth_Gloss, MAKEINTRESOURCE(IDB_EARTH_GLOSS),3);

	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glEnable(GL_CULL_FACE);

	glClearColor(0.125f, 0.125f, 0.125f, 1.0f);

	g_PersPectiveProjectionMatrix = vmath::mat4::identity();

	Resize(WIN_WIDTH, WIN_HEIGHT);

	

	return INIT_ALL_OK;
}

int LoadGLTextures(GLuint *texture, TCHAR imageResourceId[], short sOffset)
{
	fprintf_s(g_pFile, "Inside LoadGLTextures....!!!\n");
	HBITMAP hBitmap = NULL;
	BITMAP bmp;
	int iStatus = FALSE;

	glGenTextures(1, texture);
	hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), imageResourceId, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

	if (hBitmap)
	{
		fprintf_s(g_pFile, "EXEC : Inside LoadGLTextures - Image Obtained\n");
		iStatus = TRUE;
		GetObject(hBitmap, sizeof(bmp), &bmp);

		glActiveTexture(GL_TEXTURE0 + sOffset);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, *texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmp.bmWidth, bmp.bmHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, bmp.bmBits);

		glGenerateMipmap(GL_TEXTURE_2D);

		DeleteObject(hBitmap);

	}
	fprintf_s(g_pFile, "Leaving LoadGLTextures....!!!\n");
	return iStatus;
}

void Update(void)
{
	g_RotateSphere = g_RotateSphere + 0.1f;
	if (g_RotateSphere>=360.0f)
	{
		g_RotateSphere = 0.0f;
	}
}

void Render(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	vmath::mat4 modelMatrix = vmath::mat4::identity();
	vmath::mat4 viewMatrix = vmath::mat4::identity();

	glUseProgram(g_ShaderProgramObject_Sphere);
	
	modelMatrix = vmath::translate(0.0f, 0.0f, -3.0f);
	modelMatrix = modelMatrix * vmath::rotate(-90.0f, 1.0f, 0.0f, 0.0f);
	modelMatrix = modelMatrix * vmath::rotate(g_RotateSphere, 0.0f, 0.0f, 1.0f);

	glUniformMatrix4fv(g_Uniform_Model_Matrix_Sphere, 1, GL_FALSE, modelMatrix);
	glUniformMatrix4fv(g_Uniform_View_Matrix_Sphere, 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(g_Uniform_Projection_Matrix_Sphere, 1, GL_FALSE, g_PersPectiveProjectionMatrix);
	
	glUniform3f(g_Uniform_LightPosition_Sphere, 4.0f, 0.0f, 2.0f);

	// Bind Textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_Texture_Earth_Day);
	glUniform1i(g_Uniform_Earth_Day, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_Texture_Earth_Night);
	glUniform1i(g_Uniform_Earth_Night, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, g_Texture_Earth_Cloud);
	glUniform1i(g_Uniform_Earth_Cloud, 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, g_Texture_Earth_Gloss);
	glUniform1i(g_Uniform_Earth_Gloss, 3);
	

	glBindVertexArray(g_VertexArrayObject_Sphere);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Sphere_Element);
	glDrawElements(GL_TRIANGLES, g_iNumElements, GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);

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

	g_PersPectiveProjectionMatrix = vmath::perspective(45.0f, (float)iWidth / (float)iHeight, 0.1f, 100.0f);

	return true;
}


int UnInitialize(void)
{
	void DeallocateSphere(void);


	if (g_bFullScreen == true)
	{
		SetWindowLong(g_hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(g_hwnd, &wpPrev);
		SetWindowPos(g_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
		ShowCursor(TRUE);
		g_bFullScreen = false;
	}

	DeallocateSphere();


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

///Sphere Logic Start

void AllocateSphere(GLint iNumIndices)
{
	void CleanupMeshDataSphere();

	CleanupMeshDataSphere();

	g_iMaxElements = iNumIndices;
	g_iNumElements = 0;
	g_iNumVertices = 0;

	int iNumberOfIndices = iNumIndices / 3;

	g_iElements = (GLushort*)malloc(iNumberOfIndices * 3 * sizeof(g_iElements));
	g_fVerts = (GLfloat*)malloc(iNumberOfIndices * 3 * sizeof(g_fVerts));
	g_fNorms = (GLfloat*)malloc(iNumberOfIndices * 3 * sizeof(g_fNorms));
	g_fTexCoords = (GLfloat*)malloc(iNumberOfIndices * 2 * sizeof(g_fTexCoords));


}

void AddTriangles(GLfloat **single_vertex, GLfloat **single_normal, GLfloat **single_texture)
{
	void NormalizeVector(GLfloat *fVector);
	bool IsFoundIdentical(GLfloat val1, GLfloat val2, GLfloat diff);

	const float diff = 0.00001f;
	GLuint i, j;

	NormalizeVector(single_normal[0]);
	NormalizeVector(single_normal[1]);
	NormalizeVector(single_normal[2]);

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < g_iNumVertices; j++)
		{
			if (IsFoundIdentical(g_fVerts[j * 3], single_vertex[i][0], diff) &&
				IsFoundIdentical(g_fVerts[(j * 3) + 1], single_vertex[i][1], diff) &&
				IsFoundIdentical(g_fVerts[(j * 3) + 2], single_vertex[i][2], diff) &&

				IsFoundIdentical(g_fNorms[j * 3], single_normal[i][0], diff) &&
				IsFoundIdentical(g_fNorms[(j * 3) + 1], single_normal[i][1], diff) &&
				IsFoundIdentical(g_fNorms[(j * 3) + 2], single_normal[i][2], diff) &&

				IsFoundIdentical(g_fTexCoords[j * 2], single_texture[i][0], diff) &&
				IsFoundIdentical(g_fTexCoords[(j * 2) + 1], single_texture[i][1], diff))
			{
				g_iElements[g_iNumElements] = (short)j;
				g_iNumElements++;
				break;
			}
		}


		if ((j == g_iNumVertices) && (g_iNumVertices < g_iMaxElements) && (g_iNumElements < g_iMaxElements))
		{
			g_fVerts[g_iNumVertices * 3] = single_vertex[i][0];
			g_fVerts[(g_iNumVertices * 3) + 1] = single_vertex[i][1];
			g_fVerts[(g_iNumVertices * 3) + 2] = single_vertex[i][2];

			g_fNorms[g_iNumVertices * 3] = single_normal[i][0];
			g_fNorms[(g_iNumVertices * 3) + 1] = single_normal[i][1];
			g_fNorms[(g_iNumVertices * 3) + 2] = single_normal[i][2];

			g_fTexCoords[g_iNumVertices * 2] = single_texture[i][0];
			g_fTexCoords[(g_iNumVertices * 2) + 1] = single_texture[i][1];

			g_iElements[g_iNumElements] = (short)g_iNumVertices;
			g_iNumElements++;
			g_iNumVertices++;
		}
	}
}

void MakeSphere(GLfloat fRadius, GLint iSlices, GLint iStacks)
{
	void AllocateSphere(GLint iNumIndices);
	void PrepareToDraw();
	void ReleaseMemory(GLfloat **Block);

	GLfloat drho = (float)M_PI / (float)iStacks;
	GLfloat dtheta = 2.0f * (float)M_PI / (float)iSlices;
	GLfloat ds = 1.0f / (float)(iSlices);
	GLfloat dt = 1.0f / (float)(iStacks);
	GLfloat t = 1.0f;
	GLfloat s = 0.0f;
	int i = 0, j = 0;

	AllocateSphere(iSlices * iStacks * 6);

	for (int i = 0; i < iStacks; i++)
	{
		float rho = (float)(i * drho);
		float srho = (sinf(rho));
		float crho = (cosf(rho));
		float srhodrho = (sinf(rho + drho));
		float crhodrho = (cosf(rho + drho));

		s = 0.0;

		float **vertex = (float **)malloc(sizeof(float *) * 4);
		for (int idx = 0; idx < 4; idx++) // vertex[4][3]
			vertex[idx] = (float *)malloc(sizeof(float) * 3);

		float **normal = (float **)malloc(sizeof(float *) * 4);
		for (int idx = 0; idx < 4; idx++)// normal[4][3]
			normal[idx] = (float *)malloc(sizeof(float) * 3);

		float **texture = (float **)malloc(sizeof(float *) * 4);
		for (int idx = 0; idx < 4; idx++)// texture[4][2]
			texture[idx] = (float *)malloc(sizeof(float) * 2);


		for (j = 0; j < iSlices; j++)
		{
			float theta = (j == iSlices) ? 0.0f : j * dtheta;
			float stheta = (float)(-sin(theta));
			float ctheta = (float)(cos(theta));

			float x = stheta * srho;
			float y = ctheta * srho;
			float z = crho;

			texture[0][0] = s;
			texture[0][1] = t;
			normal[0][0] = x;
			normal[0][1] = y;
			normal[0][2] = z;
			vertex[0][0] = x * fRadius;
			vertex[0][1] = y * fRadius;
			vertex[0][2] = z * fRadius;

			x = stheta * srhodrho;
			y = ctheta * srhodrho;
			z = crhodrho;

			texture[1][0] = s;
			texture[1][1] = t - dt;
			normal[1][0] = x;
			normal[1][1] = y;
			normal[1][2] = z;
			vertex[1][0] = x * fRadius;
			vertex[1][1] = y * fRadius;
			vertex[1][2] = z * fRadius;

			theta = ((j + 1) == iSlices) ? 0.0f : (j + 1) * dtheta;
			stheta = (float)(-sin(theta));
			ctheta = (float)(cos(theta));

			x = stheta * srho;
			y = ctheta * srho;
			z = crho;

			s += ds;
			texture[2][0] = s;
			texture[2][1] = t;
			normal[2][0] = x;
			normal[2][1] = y;
			normal[2][2] = z;
			vertex[2][0] = x * fRadius;
			vertex[2][1] = y * fRadius;
			vertex[2][2] = z * fRadius;

			x = stheta * srhodrho;
			y = ctheta * srhodrho;
			z = crhodrho;

			texture[3][0] = s;
			texture[3][1] = t - dt;
			normal[3][0] = x;
			normal[3][1] = y;
			normal[3][2] = z;
			vertex[3][0] = x * fRadius;
			vertex[3][1] = y * fRadius;
			vertex[3][2] = z * fRadius;

			AddTriangles(vertex, normal, texture);


			vertex[0][0] = vertex[1][0];
			vertex[0][1] = vertex[1][1];
			vertex[0][2] = vertex[1][2];
			normal[0][0] = normal[1][0];
			normal[0][1] = normal[1][1];
			normal[0][2] = normal[1][2];
			texture[0][0] = texture[1][0];
			texture[0][1] = texture[1][1];

			vertex[1][0] = vertex[3][0];
			vertex[1][1] = vertex[3][1];
			vertex[1][2] = vertex[3][2];
			normal[1][0] = normal[3][0];
			normal[1][1] = normal[3][1];
			normal[1][2] = normal[3][2];
			texture[1][0] = texture[3][0];
			texture[1][1] = texture[3][1];

			AddTriangles(vertex, normal, texture);
		}
		t -= dt;
		ReleaseMemory(vertex);
		ReleaseMemory(normal);
		ReleaseMemory(texture);
	}

	PrepareToDraw();
}

void PrepareToDraw()
{
	void CleanupMeshDataSphere();

	glGenVertexArrays(1, &g_VertexArrayObject_Sphere);
	glBindVertexArray(g_VertexArrayObject_Sphere);


	glGenBuffers(1, &g_VertexBufferObject_Sphere_Position);
	glBindBuffer(GL_ARRAY_BUFFER, g_VertexBufferObject_Sphere_Position);
	glBufferData(GL_ARRAY_BUFFER, (g_iMaxElements * 3 * sizeof(g_fVerts) / 3), g_fVerts, GL_STATIC_DRAW);

	glVertexAttribPointer(SAM_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(SAM_ATTRIBUTE_POSITION);

	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glGenBuffers(1, &g_VertexBufferObject_Sphere_Normal);
	glBindBuffer(GL_ARRAY_BUFFER, g_VertexBufferObject_Sphere_Normal);
	glBufferData(GL_ARRAY_BUFFER, (g_iMaxElements * 3 * sizeof(g_fNorms) / 3), g_fNorms, GL_STATIC_DRAW);

	glVertexAttribPointer(SAM_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(SAM_ATTRIBUTE_NORMAL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glGenBuffers(1, &g_VertexBufferObject_Sphere_Texture);
	glBindBuffer(GL_ARRAY_BUFFER, g_VertexBufferObject_Sphere_Texture);
	glBufferData(GL_ARRAY_BUFFER, (g_iMaxElements * 2 * sizeof(g_fTexCoords) / 3), g_fTexCoords, GL_STATIC_DRAW);

	glVertexAttribPointer(SAM_ATTRIBUTE_TEXTURE0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(SAM_ATTRIBUTE_TEXTURE0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &g_VertexBufferObject_Sphere_Element);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Sphere_Element);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (g_iMaxElements * 3 * sizeof(g_iElements) / 3), g_iElements, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	CleanupMeshDataSphere();
}

void NormalizeVector(GLfloat *fVector)
{

	float fSquaredVectorLength = (fVector[0] * fVector[0]) + (fVector[1] * fVector[1]) + (fVector[2] * fVector[2]);
	float fSsquareRootOfSquaredVectorLength = sqrtf(fSquaredVectorLength);

	fVector[0] = fVector[0] * 1.0f / fSsquareRootOfSquaredVectorLength;
	fVector[1] = fVector[1] * 1.0f / fSsquareRootOfSquaredVectorLength;
	fVector[2] = fVector[2] * 1.0f / fSsquareRootOfSquaredVectorLength;
}

bool IsFoundIdentical(GLfloat val1, GLfloat val2, GLfloat diff)
{
	if (fabs(val1 - val2) < diff)
		return(true);
	else
		return(false);
}


GLuint GetIndexCountSphere()
{
	return(g_iNumElements);
}

GLuint GetVertexCountSphere()
{
	return GLuint(g_iNumElements);
}

void ReleaseMemory(GLfloat **Block)
{
	for (int idx = 0; idx < 4; idx++)
	{
		free(Block[idx]);
		Block[idx] = NULL;
	}
	free(Block);
	Block = NULL;

}


void CleanupMeshDataSphere()
{
	if (g_iElements != NULL)
	{
		free(g_iElements);
		g_iElements = NULL;
	}

	if (g_fVerts != NULL)
	{
		free(g_fVerts);
		g_fVerts = NULL;
	}

	if (g_fNorms != NULL)
	{
		free(g_fNorms);
		g_fNorms = NULL;
	}

	if (g_fTexCoords != NULL)
	{
		free(g_fTexCoords);
		g_fTexCoords = NULL;
	}
}


void DeallocateSphere()
{
	if (g_VertexArrayObject_Sphere)
	{
		glDeleteVertexArrays(1, &g_VertexArrayObject_Sphere);
		g_VertexArrayObject_Sphere = 0;
	}

	if (g_VertexBufferObject_Sphere_Position)
	{
		glDeleteBuffers(1, &g_VertexBufferObject_Sphere_Position);
		g_VertexBufferObject_Sphere_Position = 0;
	}

	if (g_VertexBufferObject_Sphere_Normal)
	{
		glDeleteBuffers(1, &g_VertexBufferObject_Sphere_Normal);
		g_VertexBufferObject_Sphere_Normal = 0;
	}

	if (g_VertexBufferObject_Sphere_Element)
	{
		glDeleteBuffers(1, &g_VertexBufferObject_Sphere_Element);
		g_VertexBufferObject_Sphere_Element = 0;
	}

	glDetachShader(g_ShaderProgramObject_Sphere, g_VertexShaderObject_Sphere);
	glDetachShader(g_ShaderProgramObject_Sphere, g_FragmentShaderObject_Sphere);

	if (g_VertexShaderObject_Sphere)
	{
		glDeleteShader(g_VertexShaderObject_Sphere);
		g_VertexShaderObject_Sphere = 0;
	}

	if (g_FragmentShaderObject_Sphere)
	{
		glDeleteShader(g_FragmentShaderObject_Sphere);
		g_FragmentShaderObject_Sphere = 0;
	}

	if (g_ShaderProgramObject_Sphere)
	{
		glDeleteProgram(g_ShaderProgramObject_Sphere);
		g_ShaderProgramObject_Sphere = 0;
	}

	glUseProgram(0);
}


///Sphere Logic End
