#include<Windows.h>
#include<stdio.h>

#include<gl\glew.h>
#include<gl\GL.h>

#include"vmath.h" // vermillion math library

#pragma comment (lib,"glew32.lib")
#pragma comment (lib,"opengl32.lib")

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

#define VERTEX_COUNT 4
//#define SIZE 0.8f
#define SIZE 0.451f


using namespace std;

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

enum
{
	SAM_ATTRIBUTE_VERTEX = 0,
	SAM_ATTRIBUTE_COLOR,
	SAM_ATTRIBUTE_NORMAL,
	SAM_ATTRIBUTE_TEXTURE0,
	SAM_ATTRIBUTE_INDICES,
};

FILE *gp_File = NULL;

HWND g_hwnd = NULL;
HDC  g_hdc = NULL;
HGLRC g_hrc = NULL;

DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(wpPrev) };

bool g_bActiveWindow = false;
bool g_bEscapePressed = false;
bool g_bFullscreen = false;

GLuint g_VertexShaderObject;
GLuint g_FragmentShaderObject;
GLuint g_ShaderProgramObject;

GLuint g_VertexArrayObject1;
GLuint g_VertexBufferObject;
GLuint g_VertexBufferObject_Indices;

GLuint g_Uniform_ModelMatrix;
GLuint g_Uniform_ViewMatrix;
GLuint g_Uniform_ProjectionMatrix;
GLuint g_Uniform_InputColor;

vmath::mat4 g_PerspectiveProjectionMatrix;

float g_PresentWidth = WIN_WIDTH;
float g_PresentHeight = WIN_HEIGHT;
float g_PresentAspectRatio = g_PresentWidth / g_PresentHeight;

GLfloat g_MeshVertices[2 * VERTEX_COUNT * VERTEX_COUNT];
GLuint g_shIndices[6 * (VERTEX_COUNT - 1)*(VERTEX_COUNT - 1)];
GLuint g_shIndices_shape3[16];
GLuint g_shIndices_shape2[54];
GLuint g_shIndices_shape4[26];
GLuint g_shIndices_shape5[18];
GLuint g_shIndices_shape6[24];
GLuint g_shIndices_shape6_b1[8];
GLuint g_shIndices_shape6_b2[8];
GLuint g_shIndices_shape6_b3[8];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	void Initialize(void);
	void Display(void);
	void Update(void);
	void UnInitialize(void);

	WNDCLASSEX wndclass;
	MSG msg;
	HWND hwnd = NULL;
	TCHAR szClassName[] = TEXT("SSM_OpenGLProgramablePipeline");
	bool bDone = false;

	wndclass.cbSize = sizeof(wndclass);
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.hInstance = hInstance;
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szClassName;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wndclass.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(hInstance, IDC_ARROW);
	
	if (!RegisterClassEx(&wndclass))
	{
		MessageBox(NULL, TEXT("could not Register Class"), TEXT("Failure..!!"), MB_OK);
		exit(EXIT_FAILURE);
	}
	
	hwnd = CreateWindowEx(WS_EX_APPWINDOW, szClassName, TEXT("SSM : OGL_Primitives"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, WIN_WIDTH, WIN_HEIGHT, (HWND)NULL, (HMENU)NULL,
		hInstance, (LPVOID)NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, TEXT("could not Create Window"), TEXT("Failure..!!"), MB_OK);
		exit(EXIT_FAILURE);
	}
	
	g_hwnd = hwnd;

	ShowWindow(hwnd, iCmdShow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	if (fopen_s(&gp_File, "SAM_OGL_Primitives_proglog.txt", "w+") !=0)
	{
		MessageBox(NULL, TEXT("could not Open File"), TEXT("Failure..!!"), MB_OK);
		exit(EXIT_FAILURE);
	}
	else
	{
		fprintf_s(gp_File, "File opened Succesfully....!!!\n");
	}
	Initialize();
	
	while (bDone == false)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.wParam == WM_QUIT)
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
			if (g_bActiveWindow == true)
			{
				if (g_bEscapePressed == true)
				{
					bDone = true;
				}
				Update();
				Display();
			}

		}
	}

	UnInitialize();

	return (int)msg.wParam;

}


LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{

	void Resize(int iWidth, int iHeight);
	void ToggleFullScreen(void);
	void UnInitialize(void);

	static WORD xMouse = NULL;
	static WORD yMouse = NULL;

	switch (iMsg)
	{
	case WM_ACTIVATE:
		if (HIWORD(wParam) == 0)
			g_bActiveWindow = true;
		else
			g_bActiveWindow = false;
		break;
	case WM_KEYDOWN:
		switch (LOWORD(wParam))
		{
		case VK_ESCAPE:
			g_bEscapePressed = true;
			break;
		case 0x46:
			if (g_bFullscreen == false)
			{
				ToggleFullScreen();
				g_bFullscreen = true;
			}
			else
			{
				ToggleFullScreen();
				g_bFullscreen = false;
			}
			break;
		default:
			break;
		}
		break;
	case WM_SIZE:
		g_PresentWidth = LOWORD(lParam);
		g_PresentHeight = HIWORD(lParam);
		g_PresentAspectRatio = g_PresentWidth / g_PresentHeight;
		Resize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_CLOSE:
		UnInitialize();
		break;
	case WM_DESTROY:
		PostQuitMessage(EXIT_SUCCESS);
		break;
	default:
		break;
	}

	return(DefWindowProc(hwnd, iMsg, wParam, lParam));
}


void Initialize(void)
{
	fprintf_s(gp_File, "EXEC : Inside Initialize - Start\n");
	void Resize(int iWidth, int iHeight);
	void UnInitialize(void);

	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex;

	SecureZeroMemory(&pfd, sizeof(pfd));

	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cDepthBits = 32;

	g_hdc = GetDC(g_hwnd);

	iPixelFormatIndex = ChoosePixelFormat(g_hdc, &pfd);
	if (iPixelFormatIndex == 0)
	{
		ReleaseDC(g_hwnd, g_hdc);
		g_hdc = NULL;
	}

	if (SetPixelFormat(g_hdc, iPixelFormatIndex, &pfd) == false)
	{
		ReleaseDC(g_hwnd, g_hdc);
		g_hdc = NULL;
	}

	g_hrc = wglCreateContext(g_hdc);
	if (g_hrc == NULL)
	{
		ReleaseDC(g_hwnd, g_hdc);
		g_hdc = NULL;
	}

	if (wglMakeCurrent(g_hdc, g_hrc) == false)
	{
		wglDeleteContext(g_hrc);
		g_hrc = NULL;
		ReleaseDC(g_hwnd, g_hdc);
		g_hdc = NULL;
	}

	// Sam : Position For GLEW initialization Code.
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK)
	{
		wglDeleteContext(g_hrc);
		g_hrc = NULL;
		ReleaseDC(g_hwnd, g_hdc);
		g_hdc = NULL;
	}

	// Shader Information Start
	fprintf_s(gp_File, "SHADER_INFO : OpenGl Version is : %s \n", glGetString(GL_VERSION));
	fprintf_s(gp_File, "SHADER_INFO : GLSL Version is : %s \n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Shader Information End
	// Sam :  All Shaders Code Start

	//***** Vertex Shader *****
	//Create Shader
	g_VertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	// give source code to shader
	const GLchar *vertexShaderSourceCode = 
		"#version 450 core"	\
		"\n" \
		"in vec4 vPosition;"	\
		"uniform mat4 u_model_matrix;" \
		"uniform mat4 u_view_matrix;" \
		"uniform mat4 u_projection_matrix;" \
		"void main (void)"	\
		"{"	\
		"	gl_Position = (u_projection_matrix * u_view_matrix * u_model_matrix) * vec4(vPosition.x,vPosition.y,0.0,1.0);"	\
		"}";
	glShaderSource(g_VertexShaderObject, 1, (const GLchar**)&vertexShaderSourceCode, NULL);

	// Compile Source Code
	glCompileShader(g_VertexShaderObject);
	GLint iInfoLogLength = 0;
	GLint iShaderCompileStatus = 0;
	char *szInfoLog = NULL;
	glGetShaderiv(g_VertexShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(g_VertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(g_VertexShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(gp_File, "Error : Vertex Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				UnInitialize();
				exit(EXIT_FAILURE);
			}

		}

	}

	//***** Fragment Shader *****
	g_FragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

	const GLchar *fragmentShaderSourceCode = "#version 450 core"	\
		"\n"	\
		"out vec4 FragColor;"	\
		"uniform vec4 inputColor;"	\
		"void main (void)" \
		"{"	\
		"	FragColor = vec4(inputColor.r,inputColor.g,inputColor.b,1.0);"	\
		"}";
	glShaderSource(g_FragmentShaderObject, 1, (const GLchar **)&fragmentShaderSourceCode, NULL);

	// Compile Source Code
	glCompileShader(g_FragmentShaderObject);
	iInfoLogLength = 0;
	iShaderCompileStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(g_FragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(g_FragmentShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(g_FragmentShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(gp_File, "Error : Fragment Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				UnInitialize();
				exit(EXIT_FAILURE);
			}
		}
	}

	//***** Shader Program *****
	// Create
	g_ShaderProgramObject = glCreateProgram();
	// Attach Vertex Shader
	glAttachShader(g_ShaderProgramObject, g_VertexShaderObject);
	// Attach Fragment Shader
	glAttachShader(g_ShaderProgramObject, g_FragmentShaderObject);
	// pre-link Program object with Vertex Sahder position attribute
	glBindAttribLocation(g_ShaderProgramObject, SAM_ATTRIBUTE_VERTEX, "vPosition");
	// link Shader 
	glLinkProgram(g_ShaderProgramObject);

	GLint iShaderProgramLinkStatus = 0;
	glGetProgramiv(g_ShaderProgramObject, GL_LINK_STATUS, &iShaderProgramLinkStatus);
	szInfoLog = NULL;
	iInfoLogLength = 0;
	if (iShaderProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(g_ShaderProgramObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char*)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(g_ShaderProgramObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(gp_File, "Error : Shader Program Link Log : %s \n", szInfoLog);
				free(szInfoLog);
				UnInitialize();
				exit(EXIT_FAILURE);
			}
		}
	}

	
	g_Uniform_ModelMatrix = glGetUniformLocation(g_ShaderProgramObject, "u_model_matrix");
	g_Uniform_ViewMatrix = glGetUniformLocation(g_ShaderProgramObject, "u_view_matrix");
	g_Uniform_ProjectionMatrix = glGetUniformLocation(g_ShaderProgramObject, "u_projection_matrix");
	g_Uniform_InputColor = glGetUniformLocation(g_ShaderProgramObject, "inputColor");

	// **** Verttices, Colors, Shader Attribs, Vbo, Vao Initializations ****

	int iIndex = 0;
	for (int i = 0; i < VERTEX_COUNT; i++)
	{
		for (int j = 0; j < VERTEX_COUNT; j++)
		{
			g_MeshVertices[iIndex++] = j * SIZE;
			g_MeshVertices[iIndex++] = -i  * SIZE;
			//g_MeshVertices[iIndex++] = 0.0f;
		}
	}

	iIndex = 0;
	int iFaceCount = 0;

	for (int i = 0, j = 0; j< (6 * (VERTEX_COUNT - 1)*(VERTEX_COUNT - 1)); i++, j += 6)
	{
		/*int topRight = VERTEX_COUNT + i + 1; // bottomRight
		int bottomleft = i; // topLeft
		int bottomRight = i + 1; // topRight
		int topLeft = VERTEX_COUNT + i; // bottomLeft */

		int bottomRight = VERTEX_COUNT + i + 1; // bottomRight
		int topLeft = i; // topLeft
		int topRight = i + 1; // topRight
		int bottomleft = VERTEX_COUNT + i; // bottomLeft

		g_shIndices[j] = topRight;
		g_shIndices[j + 1] = bottomleft;
		g_shIndices[j + 2] = bottomRight;

		g_shIndices[j + 3] = topRight;
		g_shIndices[j + 4] = topLeft;
		g_shIndices[j + 5] = bottomleft;

		iFaceCount += 2;
		if ((iFaceCount / ((VERTEX_COUNT - 1) * 2)) == 1)
		{
			i += 1;
			iFaceCount = 0;
		}
	}
	
	
	GLuint shIndices_Shape2[] = {1,0,0,4,4,1,1,5,5,2,2,1,2,6,6,3,3,2,5,4,4,8,8,5,6,5,5,9,9,6,7,6,6,10,10,7,9,8,8,12,12,9,10,9,9,13,13,10,11,10,10,14,14,11};

	
	memcpy_s(&g_shIndices_shape2, sizeof(g_shIndices_shape2), shIndices_Shape2, sizeof(shIndices_Shape2));

	GLuint shIndices_Shape3[] = {0,12,12,15,15,3,3,0,1,13,2,14,4,7,8,11};

	memcpy_s(&g_shIndices_shape3, sizeof(g_shIndices_shape3), shIndices_Shape3, sizeof(shIndices_Shape3));

	GLuint shIndices_Shape4[] = { 0,12,12,15,15,3,3,0,1,13,2,14,4,7,8,11,4,1,8,2,12,3,13,7,14,11 };
	memcpy_s(&g_shIndices_shape4, sizeof(g_shIndices_shape4), shIndices_Shape4, sizeof(shIndices_Shape4));
	GLuint shIndices_Shape5[] = { 0,12,12,15,15,3,3,0,0,13,0,14,0,15,0,11,0,7 };
	memcpy_s(&g_shIndices_shape5, sizeof(g_shIndices_shape5), shIndices_Shape5, sizeof(shIndices_Shape5));
	GLuint shIndices_Shape6[] = { 0,12,12,15,15,3,3,0,1,13,2,14,4,7,8,11 }; 
	memcpy_s(&g_shIndices_shape6, sizeof(g_shIndices_shape6), shIndices_Shape6, sizeof(shIndices_Shape6));
	GLuint shIndices_Shape6_b1[] = { 1,0, 0,12, 12,13, 13,1 };
	//g_shIndices_shape6_b1
	memcpy_s(&g_shIndices_shape6_b1, sizeof(g_shIndices_shape6_b1), shIndices_Shape6_b1, sizeof(shIndices_Shape6_b1));
	GLuint shIndices_Shape6_b2[] = { 2,1, 1,13, 13,14, 14,2 };
	memcpy_s(&g_shIndices_shape6_b2, sizeof(g_shIndices_shape6_b2), shIndices_Shape6_b2, sizeof(shIndices_Shape6_b2));
	GLuint shIndices_Shape6_b3[] = { 3,2, 2,14, 14,15, 15,3 };
	memcpy_s(&g_shIndices_shape6_b3, sizeof(g_shIndices_shape6_b3), shIndices_Shape6_b3, sizeof(shIndices_Shape6_b3));
	// 0 to 15 basic shape

	// vao creation and binding
	glGenBuffers(1, &g_VertexArrayObject1);
	glBindVertexArray(g_VertexArrayObject1);

	//vbo creation and binding
	glGenBuffers(1, &g_VertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, g_VertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_MeshVertices), g_MeshVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(SAM_ATTRIBUTE_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(SAM_ATTRIBUTE_VERTEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &g_VertexBufferObject_Indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices), g_shIndices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NULL);

	glBindVertexArray(0);
	// Sam :  All Shaders Code End
	glShadeModel(GL_SMOOTH);

	//glPointSize(2.5f);

	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	g_PerspectiveProjectionMatrix = vmath::mat4::identity();

	Resize(WIN_WIDTH, WIN_HEIGHT);

	fprintf_s(gp_File, "EXEC : Inside Initialize - End \n");
}

void ToggleFullScreen(void)
{
	fprintf_s(gp_File, "EXEC : Inside Toggle Fullscreen - Start\n");
	MONITORINFO mi = { sizeof(mi) };

	if (g_bFullscreen == false)
	{
		dwStyle = GetWindowLong(g_hwnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			if (GetWindowPlacement(g_hwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(g_hwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(g_hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(g_hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, (mi.rcMonitor.right - mi.rcMonitor.left), (mi.rcMonitor.bottom - mi.rcMonitor.top), SWP_NOZORDER | SWP_FRAMECHANGED);
			}
			ShowCursor(FALSE);
		}
	}
	else
	{
		SetWindowLong(g_hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(g_hwnd, &wpPrev);
		SetWindowPos(g_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER);
		ShowCursor(TRUE);
	}
	fprintf_s(gp_File, "EXEC : Inside Toggle Fullscreen - End \n");
}

void Resize(int iWidth, int iHeight)
{
	//fprintf_s(gp_File, "EXEC : Inside Resize - Start \n " );
	if (iHeight == 0)
		iHeight = 1;

	glViewport(0, 0, (GLsizei)iWidth, (GLsizei)iHeight);

	//glFrustum(left,right,bottom,top,Near-ZAxis,Far-ZAxis)
	g_PerspectiveProjectionMatrix = vmath::perspective(45.0f, (float)iWidth / (float)iHeight, 0.1f, 100.0f);

	//fprintf_s(gp_File, "EXEC : Inside Resize - End \n");
}

void Display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_ShaderProgramObject);

	vmath::mat4 modelMatrix = vmath::mat4::identity();
	vmath::mat4 viewMatrix = vmath::mat4::identity();
	vmath::mat4 scaleMatrix = vmath::mat4::identity();
	vmath::mat4 rotationMatrix = vmath::mat4::identity();


	modelMatrix = vmath::translate(-2.0f, 0.2f, -5.0f);
	
	glUniformMatrix4fv(g_Uniform_ModelMatrix, 1, GL_FALSE, modelMatrix);
	glUniformMatrix4fv(g_Uniform_ViewMatrix, 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(g_Uniform_ProjectionMatrix, 1, GL_FALSE, g_PerspectiveProjectionMatrix);

	glUniform4f(g_Uniform_InputColor, 1.0f, 1.0f, 1.0f, 1.0f);

	
	// Shape 1
	glViewport(0, g_PresentHeight / 2.0f, g_PresentWidth/1.5f, g_PresentHeight/1.5f );
	glBindVertexArray(g_VertexArrayObject1);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices), g_shIndices, GL_DYNAMIC_DRAW);
	glPointSize(2.5f);
	glDrawElements(GL_POINTS, sizeof(g_shIndices), GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NULL);
	
	glBindVertexArray(0);

	// Shape 2 
	glViewport(g_PresentWidth / 3.0f, g_PresentHeight / 2.0f, g_PresentWidth / 1.5f, g_PresentHeight / 1.5f);
	glBindVertexArray(g_VertexArrayObject1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices_shape2), g_shIndices_shape2, GL_DYNAMIC_DRAW);
	glDrawElements(GL_LINES, sizeof(GLuint)*54, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	
	glBindVertexArray(0);

	// Shape  3
	glViewport(g_PresentWidth * (2.0f / 3.0f), g_PresentHeight / 2.0f, g_PresentWidth / 1.5f, g_PresentHeight / 1.5f);
	glBindVertexArray(g_VertexArrayObject1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices_shape3), g_shIndices_shape3, GL_DYNAMIC_DRAW);
	glDrawElements(GL_LINES, sizeof(g_shIndices_shape3), GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glBindVertexArray(0);

	// Shape 4
	glViewport(0, 0, g_PresentWidth / 1.5f, g_PresentHeight / 1.5f);
	glBindVertexArray(g_VertexArrayObject1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices_shape4), g_shIndices_shape4, GL_DYNAMIC_DRAW);
	glDrawElements(GL_LINES, sizeof(g_shIndices_shape4), GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glBindVertexArray(0);

	// Shape 5
	glViewport(g_PresentWidth / 3.0f, 0, g_PresentWidth / 1.5f, g_PresentHeight / 1.5f);
	glBindVertexArray(g_VertexArrayObject1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices_shape5), g_shIndices_shape5, GL_DYNAMIC_DRAW);
	glDrawElements(GL_LINES, sizeof(g_shIndices_shape5), GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glBindVertexArray(0);

	// Shape 6
	glViewport(g_PresentWidth * (2.0f / 3.0f), 0, g_PresentWidth / 1.5f, g_PresentHeight / 1.5f);

	glUniform4f(g_Uniform_InputColor, 1.0f, 0.0f, 0.0f, 1.0f);
	glBindVertexArray(g_VertexArrayObject1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices_shape6_b1), g_shIndices_shape6_b1, GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLE_FAN, sizeof(g_shIndices_shape6_b1), GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glBindVertexArray(0);

	glUniform4f(g_Uniform_InputColor, 0.0f, 1.0f, 0.0f, 1.0f);
	glBindVertexArray(g_VertexArrayObject1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices_shape6_b2), g_shIndices_shape6_b2, GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLE_FAN, sizeof(g_shIndices_shape6_b2), GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glBindVertexArray(0);

	glUniform4f(g_Uniform_InputColor, 0.0f, 0.0f, 1.0f, 1.0f);
	glBindVertexArray(g_VertexArrayObject1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices_shape6_b3), g_shIndices_shape6_b3, GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLE_FAN, sizeof(g_shIndices_shape6_b3), GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glBindVertexArray(0);
	
	glUniform4f(g_Uniform_InputColor, 1.0f, 1.0f, 1.0f, 1.0f);
	glBindVertexArray(g_VertexArrayObject1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_VertexBufferObject_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_shIndices_shape6), g_shIndices_shape6, GL_DYNAMIC_DRAW);
	glDrawElements(GL_LINES, sizeof(g_shIndices_shape6), GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glBindVertexArray(0);


	glUseProgram(0);

	SwapBuffers(g_hdc);
}

void Update(void)
{

}

void UnInitialize(void)
{
	fprintf_s(gp_File, "EXEC : Inside UnInitilize - Start\n");
	if (g_bFullscreen == true)
	{
		SetWindowLong(g_hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(g_hwnd, &wpPrev);
		SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER);
		ShowCursor(TRUE);
	}

	if (g_VertexArrayObject1)
	{
		glDeleteVertexArrays(1, &g_VertexArrayObject1);
		g_VertexArrayObject1 = 0;
	}

	if (g_VertexBufferObject_Indices)
	{
		glDeleteBuffers(1, &g_VertexBufferObject_Indices);
		g_VertexBufferObject_Indices = 0;
	}

	if (g_VertexBufferObject)
	{
		glDeleteBuffers(1, &g_VertexBufferObject);
		g_VertexBufferObject = 0;
	}

	glDetachShader(g_ShaderProgramObject, g_VertexShaderObject);
	glDetachShader(g_ShaderProgramObject, g_FragmentShaderObject);

	if (g_VertexShaderObject)
	{
		glDeleteShader(g_VertexShaderObject);
		g_VertexShaderObject = 0;
	}

	if (g_FragmentShaderObject)
	{
		glDeleteShader(g_FragmentShaderObject);
		g_FragmentShaderObject = 0;
	}

	if (g_ShaderProgramObject)
	{
		glDeleteProgram(g_ShaderProgramObject);
		g_ShaderProgramObject = 0;
	}

	glUseProgram(0);

	wglMakeCurrent(NULL, NULL);

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

	if (gp_File)
	{
		fprintf_s(gp_File, "EXEC : Inside UnInitilize - End \n");
		fprintf_s(gp_File, "Log file is closed....!!");
		fclose(gp_File);
		gp_File = NULL;
	}
}
