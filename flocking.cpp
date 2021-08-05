#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>

#include<gl\glew.h>
#include<gl\GL.h>
#include<IL\il.h>
#include"vmath.h"


#define WIN_WIDTH	800
#define WIN_HEIGHT	600

/*#define WORKGROUP_SIZE	128
#define NUM_WORKGROUPS	16*/
#define WORKGROUP_SIZE	256
#define NUM_WORKGROUPS  16
#define FLOCK_SIZE		NUM_WORKGROUPS * WORKGROUP_SIZE

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"DevIL.lib")

//using namespace std;

enum InitErrorCodes
{
	INIT_COMPUTE_SHADER_COMPILATION_FAILED = -10,
	INIT_VERTEX_SHADER_COMPILATION_FAILED = -9,
	INIT_FRAGMENT_SHADER_COMPILATION_FAILED,
	INIT_LINK_SHADER_PROGRAM_FAILED,
	INIT_FAIL_GLEW_INIT,
	INIT_FAIL_BRIDGE_CONTEX_SET,
	INIT_FAIL_BRIDGE_CONTEX_CREATION,
	INIT_FAIL_SET_PIXEL_FORMAT,
	INIT_FAIL_NO_PIXEL_FORMAT,
	INIT_FAIL_NO_HDC,
	INIT_ALL_OK,
};

/*
enum attributeBindLocations
{
	SAM_ATTRIBUTE_POSITION = 0,
	SAM_ATTRIBUTE_COLOR,
	SAM_ATTRIBUTE_NORNAL,
	SAM_ATTRIBUTE_TEXTURE0,
	SAM_ATTRIBUTE_BIRD_POS,
	SAM_ATTRIBUTE_BIRD_VEL,
};
*/

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

bool g_bWindowActive = false;
HWND g_hwnd = NULL;
HDC  g_hdc = NULL;
HGLRC g_hrc = NULL;

WINDOWPLACEMENT wpPrev;
DWORD dwStyle;
bool g_bFullScreen = false;

FILE* g_pFile = NULL;

struct flock_member
{
	vmath::vec3 position;
	unsigned int : 32;// padding
	vmath::vec3 velocity;
	unsigned int : 32;// padding
};

// Shaders
//GLuint iVertexShaderObject = 0;
//GLuint iFragmentShaderObject = 0;
GLuint g_ShaderProgramObject_Particle = 0;
GLuint g_ComputeProgramObject_Particle = 0;

// All Vertex Buffers
//GLuint g_VertexArrayObject = 0;
//GLuint g_VertexBufferObject_Position = 0;
//GLuint g_VertexBufferObject_Color = 0;

GLuint      flock_buffer[2];
GLuint      flock_render_vao[2];
GLuint      geometry_buffer;

// Uniforms
GLuint g_Uniform_Model_Matrix = 0;
GLuint g_Uniform_View_Matrix = 0;
GLuint g_Uniform_Projection_Matrix = 0;

GLint g_uniform_NumberofRows = 0;
GLint g_uniform_Offset1 = 0;
GLint g_uniform_Offset2 = 0;
GLint g_uniform_blendFactor = 0;

GLint g_Uniform_CS_closest_allowed_dist = 0;
GLint g_Uniform_CS_rule1_weight = 0;
GLint g_Uniform_CS_rule2_weight = 0;
GLint g_Uniform_CS_rule3_weight = 0;
GLint g_Uniform_CS_rule4_weight = 0;
GLint g_Uniform_CS_goal = 0;
GLint g_Uniform_CS_timeStep = 0;

GLuint  swap_index = 0;
// sampler
GLuint g_uniform_ButterFlySampler = 0;
GLuint g_TextureButterFly = 0;



// Projection
vmath::mat4 g_PersPectiveProjectionMatrix;

GLfloat g_AnimTime = 0.0;
GLfloat g_FlockTime = 0.0;

// Image information to calculate offsets

int iNumberOfRows = 5;// change it later on

int iTextureIndex = 0;
// size of texture atlas
int g_iImgRows = 0;
int g_iImgCols = 0;
// x and y offset => local at Render()

//GLfloat g_fTimeValue = 0.0f;

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
	case INIT_COMPUTE_SHADER_COMPILATION_FAILED:
		fprintf_s(g_pFile, "Failed to Compile compute Shader \n");
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
	int LoadPNGTexture(GLuint * texture, const char* filename, int* imgRows, int* imgCols);

	int iPixelIndex = 0;
	PIXELFORMATDESCRIPTOR pfd;

	// Shader Programs
	GLuint iVertexShaderObject = 0;
	GLuint iFragmentShaderObject = 0;

	GLuint iCoumuteShaderObject = 0;

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
	/*Compute Shader Program*/
	iCoumuteShaderObject = glCreateShader(GL_COMPUTE_SHADER);
	const GLchar* coupteSource =
		"#version 430 core"	\
		"\n"	\
		"layout (local_size_x=256)in;\n"	\
		"uniform float closest_allowed_dist = 50.0;\n"	\
		"uniform float rule1_weight = 0.18;\n"	\
		"uniform float rule2_weight = 0.05;\n"	\
		"uniform float rule3_weight = 0.17;\n"	\
		"uniform float rule4_weight = 0.02;\n"	\
		"uniform vec3 goal = vec3(0.0);\n"	\
		"uniform float timestep = 0.4;\n"	\
		"struct flock_member"	\
		"{"	\
		"	vec3 position;"	\
		"	vec3 velocity;"	\
		"};\n"	\
		"layout(std430,binding=0) readonly buffer members_in"	\
		"{"	\
		"	flock_member member[];"	\
		"}input_data;\n"	\
		"layout (std430, binding = 1) buffer members_out"	\
		"{"	\
		"	flock_member member[];"	\
		"}output_data;"	\
		"shared flock_member shared_member[gl_WorkGroupSize.x];"	\
		"vec3 rule1(vec3 my_position, vec3 my_velocity, vec3 their_position, vec3 their_velocity)"	\
		"{"	\
		"	vec3 d = my_position - their_position;\n"	\
		"	if (dot(d, d) < closest_allowed_dist)"	\
		"	{"	\
		"		return d;\n"	\
		"	}"	\
		"	return vec3(0.0);\n"	\
		"}\n"	\
		"vec3 rule2(vec3 my_position, vec3 my_velocity, vec3 their_position, vec3 their_velocity)"	\
		"{"	\
		"	vec3 d = their_position - my_position;"	\
		"	vec3 dv = their_velocity - my_velocity;"	\
		"	return dv / (dot(d, d) + 10.0);"	\
		"}\n"	\
		"void main ()"	\
		"{"	\
		"	uint i, j;\n"	\
		"	int global_id = int(gl_GlobalInvocationID.x);\n"	\
		"	int local_id  = int(gl_LocalInvocationID.x);\n"	\
		"	flock_member me = input_data.member[global_id];\n"	\
		"	flock_member new_me;\n"	\
		"	vec3 accelleration = vec3(0.0);"	\
		"	vec3 flock_center = vec3(0.0);"	\
		"	for (i = 0; i < gl_NumWorkGroups.x; i++)"	\
		"	{\n"	\
		"		flock_member them=input_data.member[i*gl_WorkGroupSize.x+local_id];\n"	\
		"		shared_member[local_id] = them;\n"	\
		"		memoryBarrierShared();"	\
		"		barrier();"	\
		"		for (j = 0; j < gl_WorkGroupSize.x; j++)\n"	\
		"		{"	\
		"			them = shared_member[j];\n"	\
		"			flock_center += them.position;\n"	\
		"			if(i*gl_WorkGroupSize.x+j!=global_id)\n"	\
		"			{"	\
		"				accelleration += rule1(me.position,me.velocity,them.position,them.velocity) * rule1_weight;\n"	\
		"				accelleration += rule2(me.position,me.velocity,them.position,them.velocity) * rule2_weight;\n"	\
		"			}"	\
		"		}"	\
		"		barrier();\n"	\
		"	}\n"	\
		"	flock_center /= float(gl_NumWorkGroups.x * gl_WorkGroupSize.x);\n"	\
		"	new_me.position = me.position + me.velocity * timestep;\n"	\
		"	accelleration+=normalize(goal-me.position)*rule3_weight;\n"	\
		"	accelleration+=normalize(flock_center-me.position)*rule4_weight;\n"	\
		"	new_me.velocity=me.velocity+accelleration*timestep;\n"	\
		"	if (length(new_me.velocity) > 10.0)"	\
		"	{"	\
		"		new_me.velocity=normalize(new_me.velocity)*10.0;"	\
		"	}"	\
		"	new_me.velocity=mix(me.velocity,new_me.velocity,0.4);"	\
		"	output_data.member[global_id] = new_me;\n"	\
		"}";

	glShaderSource(iCoumuteShaderObject, 1, (const GLchar**)&coupteSource, NULL);
	glCompileShader(iCoumuteShaderObject);
	GLint iInfoLogLength = 0;
	GLint iCompileStatus = 0;
	char* chErrorMessage = NULL;
	glGetShaderiv(iCoumuteShaderObject, GL_COMPILE_STATUS, &iCompileStatus);
	if (iCompileStatus == GL_FALSE)
	{
		glGetShaderiv(iCoumuteShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			chErrorMessage = (char*)malloc(iInfoLogLength);
			if (chErrorMessage != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(iCoumuteShaderObject, GL_INFO_LOG_LENGTH, &written, chErrorMessage);
				fprintf_s(g_pFile, "COMPUTE Shader Error : %s \n", chErrorMessage);
				free(chErrorMessage); chErrorMessage = NULL;
				return INIT_COMPUTE_SHADER_COMPILATION_FAILED;
			}
		}
	}

	g_ComputeProgramObject_Particle = glCreateProgram();

	glAttachShader(g_ComputeProgramObject_Particle, iCoumuteShaderObject);

	//Link Shader
	glLinkProgram(g_ComputeProgramObject_Particle);

	GLint iLinkStatus = 0;
	iInfoLogLength = 0;
	chErrorMessage = NULL;
	glGetProgramiv(g_ComputeProgramObject_Particle, GL_LINK_STATUS, &iLinkStatus);

	if (iLinkStatus == GL_FALSE)
	{
		glGetProgramiv(g_ComputeProgramObject_Particle, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			GLsizei written;
			chErrorMessage = (char*)malloc(iInfoLogLength);
			if (chErrorMessage != NULL)
			{
				glGetProgramInfoLog(g_ComputeProgramObject_Particle, GL_INFO_LOG_LENGTH, &written, chErrorMessage);
				fprintf_s(g_pFile, "Error In COMPUTE ShaderProgram: %s \n", chErrorMessage);
				free(chErrorMessage); chErrorMessage = NULL;
				return INIT_LINK_SHADER_PROGRAM_FAILED;
			}
		}
	}

	g_Uniform_CS_closest_allowed_dist = glGetUniformLocation(g_ComputeProgramObject_Particle, "closest_allowed_dist");
	g_Uniform_CS_rule1_weight = glGetUniformLocation(g_ComputeProgramObject_Particle, "rule1_weight");
	g_Uniform_CS_rule2_weight = glGetUniformLocation(g_ComputeProgramObject_Particle, "rule2_weight");
	g_Uniform_CS_rule3_weight = glGetUniformLocation(g_ComputeProgramObject_Particle, "rule3_weight");
	g_Uniform_CS_rule4_weight = glGetUniformLocation(g_ComputeProgramObject_Particle, "rule4_weight");
	g_Uniform_CS_goal = glGetUniformLocation(g_ComputeProgramObject_Particle, "goal");
	g_Uniform_CS_timeStep = glGetUniformLocation(g_ComputeProgramObject_Particle, "timestep");
	/*Compute Shader Program*/

	/*Vertex Shader Start*/
	iVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* vertexShaderSourceCode = "#version 450 core"	\
		"\n" \
		"layout (location = 0)in vec3 vPosition;\n" \
		"layout (location = 1)in vec3 vNormal;\n" \
		"layout (location = 2)in vec2 vTexCoord;\n" \

		"layout (location = 3)in vec3 vBirdPos;\n" \
		"layout (location = 4)in vec3 vBirdVel;\n" \

		"out vec3 out_Color;\n" \
		"out vec2 out_TexCoord;\n" \
		"uniform mat4 u_model_matrix;\n" \
		"uniform mat4 u_view_matrix;\n" \
		"uniform mat4 u_projection_matrix;\n" \
		"mat4 make_lookat(vec3 forward, vec3 up)"	\
		"{\n"	\
		"	vec3 side = cross(forward, up);"	\
		"	vec3 u_frame = cross(side, forward);"	\
		"	return mat4(vec4(side, 0.0),vec4(u_frame, 0.0),vec4(forward, 0.0),vec4(0.0, 0.0, 0.0, 1.0));"	\
		"}\n"	\
		"vec3 choose_color(float f)"	\
		"{\n"	\
		"	float R = sin(f * 6.2831853);\n"	\
		"	float G = sin((f + 0.3333) * 6.2831853);\n"	\
		"	float B = sin((f + 0.6666) * 6.2831853);\n"	\
		"	return vec3(R, G, B) * 0.25 + vec3(0.75);\n"	\
		"}\n"	\
		"\n"	\
		"void main(void)" \
		"{\n" \
		"	mat4 lookAt=make_lookat(normalize(vBirdVel),vec3(0.0, 1.0, 0.0));\n"	\
		"	vec4 obj_coord=lookAt*vec4(vPosition.xyz,1.0);"	\
		"	vec3 N = mat3(lookAt)*vNormal;"	\
		"	vec3 C = choose_color(fract(float(gl_InstanceID / float(1237.0))));"	\
		"	gl_Position =  u_projection_matrix * u_view_matrix * u_model_matrix*(obj_coord+vec4(vBirdPos,0.0));\n" \
		"	out_Color = mix(C , C* 0.2, smoothstep(0.0, 0.8, abs(N).z));\n"	\
		"	out_TexCoord = vec2(vTexCoord.x,-vTexCoord.y);\n"	\
		"}";

	glShaderSource(iVertexShaderObject, 1, (const GLchar**)&vertexShaderSourceCode, NULL);

	// Compile it
	glCompileShader(iVertexShaderObject);
	iInfoLogLength = 0;
	GLint iShaderCompileStatus = 0;
	GLchar* szInfoLog = NULL;
	glGetShaderiv(iVertexShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(iVertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(iVertexShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf_s(g_pFile, "ERROR : Vertex Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				return INIT_VERTEX_SHADER_COMPILATION_FAILED;
				//DestroyWindow(g_hwnd);
				//exit(EXIT_FAILURE);
			}
		}
	}

	/*Vertex Shader End*/

	/*Fragment Shader Start*/
	iFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar* fragmentShaderSourceCode = "#version 450 core"	\
		"\n"	\
		"in vec3 out_Color;"	\
		"in vec2 out_TexCoord;\n" \
		"out vec4 FragColor;"	\
		"uniform sampler2D s_texture;"	\
		"uniform float numberOfRows;\n"	\
		"uniform vec2 offset1;\n"	\
		"uniform vec2 offset2;\n"	\
		"uniform float blendFactor;"	\
		"void main(void)"	\
		"{"	\
		"	vec4 texColor1;\n"	\
		"	vec4 texColor2;\n"	\
		"	vec4 finColor;\n"		\
		"	vec2 newTexcoords1 = (vec2(out_TexCoord.x,out_TexCoord.y)/numberOfRows) + offset1;\n"	\
		"	vec2 newTexcoords2 = (vec2(out_TexCoord.x,out_TexCoord.y)/numberOfRows) + offset2;\n"	\
		"	texColor1 = texture(s_texture,newTexcoords1);"	\
		"	texColor2 = texture(s_texture,newTexcoords2);"	\
		"	finColor = mix(texColor1,texColor2,blendFactor);"	\
		"	if( finColor.a <= 0.3)"	\
		"	{\n"	\
		"		discard;\n"	\
		"	}\n"	\
		"	FragColor = vec4(finColor.r,finColor.g,finColor.b,1.0);"	\
		"}";

	glShaderSource(iFragmentShaderObject, 1, (const GLchar**)&fragmentShaderSourceCode, NULL);
	glCompileShader(iFragmentShaderObject);
	iInfoLogLength = 0;
	iShaderCompileStatus = 0;
	szInfoLog = NULL;
	glGetShaderiv(iFragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(iFragmentShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(iFragmentShaderObject, GL_INFO_LOG_LENGTH, &written, szInfoLog);
				fprintf(g_pFile, "ERROR: Fragment Shader Compilation Log : %s \n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				return INIT_FRAGMENT_SHADER_COMPILATION_FAILED;
				//DestroyWindow(g_hwnd);
				//exit(EXIT_FAILURE);
			}
		}
	}
	/*Fragment Shader End*/

	/* Shader Program Start */
	g_ShaderProgramObject_Particle = glCreateProgram();
	glAttachShader(g_ShaderProgramObject_Particle, iVertexShaderObject);
	glAttachShader(g_ShaderProgramObject_Particle, iFragmentShaderObject);
	//glBindAttribLocation(g_ShaderProgramObject_Particle, SAM_ATTRIBUTE_POSITION, "vPosition");
	//glBindAttribLocation(g_ShaderProgramObject_Particle, SAM_ATTRIBUTE_COLOR, "vColor");
	glLinkProgram(g_ShaderProgramObject_Particle);

	GLint iShaderLinkStatus = 0;
	iInfoLogLength = 0;
	glGetProgramiv(g_ShaderProgramObject_Particle, GL_LINK_STATUS, &iShaderLinkStatus);
	if (iShaderLinkStatus == GL_FALSE)
	{
		glGetProgramiv(g_ShaderProgramObject_Particle, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (GLchar*)malloc(iInfoLogLength * sizeof(GLchar));
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(g_ShaderProgramObject_Particle, GL_INFO_LOG_LENGTH, &written, szInfoLog);
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
	g_Uniform_Model_Matrix = glGetUniformLocation(g_ShaderProgramObject_Particle, "u_model_matrix");
	g_Uniform_Projection_Matrix = glGetUniformLocation(g_ShaderProgramObject_Particle, "u_projection_matrix");
	g_Uniform_View_Matrix = glGetUniformLocation(g_ShaderProgramObject_Particle, "u_view_matrix");

	g_uniform_ButterFlySampler = glGetUniformLocation(g_ShaderProgramObject_Particle, "s_texture");
	g_uniform_NumberofRows = glGetUniformLocation(g_ShaderProgramObject_Particle, "numberOfRows");
	g_uniform_Offset1 = glGetUniformLocation(g_ShaderProgramObject_Particle, "offset1");
	g_uniform_Offset2 = glGetUniformLocation(g_ShaderProgramObject_Particle, "offset2");
	g_uniform_blendFactor = glGetUniformLocation(g_ShaderProgramObject_Particle, "blendFactor");
	/*Setup Uniforms End*/
	/// Sam : all Shader Code End

	/*const vmath::vec3 geometry[] =
	{
		// Positions
		vmath::vec3(-5.0f, 1.0f, 0.0f),
		vmath::vec3(-1.0f, 1.5f, 0.0f),
		vmath::vec3(-1.0f, 1.5f, 7.0f),
		vmath::vec3(0.0f, 0.0f, 0.0f),
		vmath::vec3(0.0f, 0.0f, 10.0f),
		vmath::vec3(1.0f, 1.5f, 0.0f),
		vmath::vec3(1.0f, 1.5f, 7.0f),
		vmath::vec3(5.0f, 1.0f, 0.0f),

		// Normals
		vmath::vec3(0.0f),
		vmath::vec3(0.0f),
		vmath::vec3(0.107f, -0.859f, 0.00f),
		vmath::vec3(0.832f, 0.554f, 0.00f),
		vmath::vec3(-0.59f, -0.395f, 0.00f),
		vmath::vec3(-0.832f, 0.554f, 0.00f),
		vmath::vec3(0.295f, -0.196f, 0.00f),
		vmath::vec3(0.124f, 0.992f, 0.00f),
	};*/

	static const vmath::vec3 geometry[] =
	{
		// vertices
		vmath::vec3(-10.0f,-10.0f,0.0f),
		vmath::vec3(-10.0f,10.0f,0.0f),
		vmath::vec3(10.0f,-10.0f,0.0f),

		vmath::vec3(10.0f,10.0f,0.0f),

		// Normals
		vmath::vec3(0.0f,1.0f,0.0f),
		vmath::vec3(0.0f,1.0f,0.0f),
		vmath::vec3(0.0f,1.0f,0.0f),

		vmath::vec3(0.0f,1.0f,0.0f),

		// Texcoords
		vmath::vec3(0.0f,0.0f,0.0f),
		vmath::vec3(0.0f,1.0f,0.0f),
		vmath::vec3(1.0f,0.0f,0.0f),

		vmath::vec3(1.0f,1.0f,0.0f),
	};


	static const GLfloat geoMetryVNT[] =
	{
		// vertices			// Normal			// Texcoord
		-10.0f,-10.0f,0.0f,	0.0f,1.0f,0.0f,		0.0f,0.0f,
		10.0f,-10.0f,0.0f,	0.0f,1.0f,0.0f,		1.0f,0.0f,
		-10.0f,10.0f,0.0f,	0.0f,1.0f,0.0f,		0.0f,1.0f,

		10.0f,10.0f,0.0f,	0.0f,1.0f,0.0f,		1.0f,1.0f,

	};

	glGenBuffers(2, flock_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, flock_buffer[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, FLOCK_SIZE * sizeof(flock_member), NULL, GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, flock_buffer[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, FLOCK_SIZE * sizeof(flock_member), NULL, GL_DYNAMIC_COPY);

	int i;

	glGenBuffers(1, &geometry_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(geoMetryVNT), geoMetryVNT, GL_STATIC_DRAW);

	glGenVertexArrays(2, flock_render_vao);

	for (i = 0; i < 2; i++)
	{
		glBindVertexArray(flock_render_vao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, geometry_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

		glBindBuffer(GL_ARRAY_BUFFER, flock_buffer[i]);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(flock_member), NULL);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(flock_member), (void*)sizeof(vmath::vec4));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
	}

	glBindBuffer(GL_ARRAY_BUFFER, flock_buffer[0]);
	flock_member* ptr = reinterpret_cast<flock_member*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, FLOCK_SIZE * sizeof(flock_member), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

	for (i = 0; i < FLOCK_SIZE; i++)
	{
		ptr[i].position = (vmath::vec3::random() - vmath::vec3(0.5f)) * 300.0f;
		ptr[i].velocity = (vmath::vec3::random() - vmath::vec3(0.5f));
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//glEnable(GL_CULL_FACE);

	ilInit();
	if (LoadPNGTexture(&g_TextureButterFly, "atlas.png", &g_iImgRows, &g_iImgCols))
	{
		MessageBox(NULL, TEXT("Image Loaded"), TEXT("msg"), MB_OK | MB_ICONINFORMATION);
	}

	glClearColor(0.125f, 0.125f, 0.125f, 1.0f);

	g_PersPectiveProjectionMatrix = vmath::mat4::identity();

	Resize(WIN_WIDTH, WIN_HEIGHT);

	return INIT_ALL_OK;
}


int LoadPNGTexture(GLuint* texture, const char* filename, int* imgRows, int* imgCols)
{

	ILuint ImageName;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);
	if (!ilLoadImage((ILstring)filename))
	{
		return -1;
	}

	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_FORMAT), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), ilGetInteger(IL_IMAGE_TYPE), ilGetData());
	glGenerateMipmap(GL_TEXTURE_2D);

	*imgRows = ilGetInteger(IL_IMAGE_HEIGHT);
	*imgCols = ilGetInteger(IL_IMAGE_WIDTH);

	ilDeleteImages(1, &ImageName);

	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}

void Update(void)
{
	g_AnimTime = g_AnimTime + 0.04f;
	if (g_AnimTime > 1.0f)
	{
		g_AnimTime = 0.0f;
	}


	g_FlockTime = GetTickCount() / 1000.0f;
}

void Render(void)
{
	int stageCount = iNumberOfRows * iNumberOfRows;
	float atlasProgression = g_AnimTime * stageCount;
	int index1 = (int)floor(atlasProgression);
	int index2 = index1 < stageCount - 1 ? index1 + 1 : index1;

	double integer;
	float blendFactor = (float)modf(atlasProgression, &integer);

	/* v2Offset1,v2Offset2 and fBlendfactor mist be updated per frame */
	// column = (iTextureIndex%iNumberOfRows)
	// xOffset = column / totalNumberOfColumn;
	float xOffset = (float)(index1 % iNumberOfRows) / iNumberOfRows;
	// row = floor(iTextureIndex/iNumberOfRows)
	// yOffset = row / totalNumberOfrows
	float yOffset = (float)floor(index1 / iNumberOfRows) / iNumberOfRows;
	vmath::vec2 v2Offset1 = vmath::vec2(xOffset, yOffset);

	xOffset = (float)(index2 % iNumberOfRows) / iNumberOfRows;
	yOffset = (float)floor(index2 / iNumberOfRows) / iNumberOfRows;
	vmath::vec2 v2Offset2 = vmath::vec2(xOffset, yOffset);


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(g_ComputeProgramObject_Particle);
	vmath::vec3 goal = vmath::vec3(sinf(g_FlockTime * 0.34f),
		cosf(g_FlockTime * 0.29f),
		sinf(g_FlockTime * 0.12f) * cosf(g_FlockTime * 0.5f));

	//goal = goal * vmath::vec3(35.0f, 25.0f, 60.0f);
	goal = goal * vmath::vec3(50.0f, 0.0f, 50.0f);
	glUniform3fv(g_Uniform_CS_goal, 1, goal);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, flock_buffer[swap_index]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, flock_buffer[swap_index ^ 1]);
	glDispatchCompute(NUM_WORKGROUPS, 1, 1);
	glUseProgram(0);
	///swap_index
	vmath::mat4 modelMatrix = vmath::mat4::identity();
	vmath::mat4 viewMatrix = vmath::mat4::identity();

	viewMatrix = vmath::lookat(vmath::vec3(0.0f, 0.0f, -400.0f),
		vmath::vec3(0.0f, 0.0f, 0.0f),
		vmath::vec3(0.0f, 1.0f, 0.0f));

	glUseProgram(g_ShaderProgramObject_Particle);

	glUniformMatrix4fv(g_Uniform_Model_Matrix, 1, GL_FALSE, modelMatrix);
	glUniformMatrix4fv(g_Uniform_View_Matrix, 1, GL_FALSE, viewMatrix);
	glUniformMatrix4fv(g_Uniform_Projection_Matrix, 1, GL_FALSE, g_PersPectiveProjectionMatrix);
	glUniform1f(g_uniform_NumberofRows, iNumberOfRows);
	glUniform2fv(g_uniform_Offset1, 1, v2Offset1);
	glUniform2fv(g_uniform_Offset2, 1, v2Offset2);
	glUniform1f(g_uniform_blendFactor, blendFactor);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_TextureButterFly);
	glUniform1i(g_uniform_ButterFlySampler, 0);
	//glUniform1f(g_Uniform_CS_timeStep, g_AnimTime);

	glBindVertexArray(flock_render_vao[swap_index]);

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, FLOCK_SIZE);

	glUseProgram(0);

	SwapBuffers(g_hdc);

	swap_index ^= 1;
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

	g_PersPectiveProjectionMatrix = vmath::perspective(60.0f, (float)iWidth / (float)iHeight, 0.1f, 3000.0f);

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

	if (g_TextureButterFly)
	{
		glDeleteTextures(1, &g_TextureButterFly);
		g_TextureButterFly = 0;
	}

	if (geometry_buffer)
	{
		glDeleteBuffers(1, &geometry_buffer);
		geometry_buffer = 0;
	}

	if (flock_buffer[0])
	{
		glDeleteBuffers(2, flock_buffer);
	}

	if (flock_render_vao[0])
	{
		glDeleteVertexArrays(2, flock_render_vao);
	}

	glUseProgram(0);

	if (g_ComputeProgramObject_Particle)
	{
		GLsizei iShaderCount;
		GLsizei iShaderNumber;

		glUseProgram(g_ComputeProgramObject_Particle);
		glGetProgramiv(g_ComputeProgramObject_Particle, GL_ATTACHED_SHADERS, &iShaderCount);
		GLuint* pShaders = (GLuint*)calloc(iShaderCount, sizeof(GLuint));

		if (pShaders)
		{
			glGetAttachedShaders(g_ComputeProgramObject_Particle, iShaderCount, &iShaderCount, pShaders);
			for (iShaderNumber = 0; iShaderNumber < iShaderCount; iShaderNumber++)
			{
				glDetachShader(g_ComputeProgramObject_Particle, pShaders[iShaderNumber]);
				glDeleteShader(pShaders[iShaderNumber]);
				pShaders[iShaderNumber] = 0;
			}
			free(pShaders);
			pShaders = NULL;
		}
		glUseProgram(0);
		glDeleteProgram(g_ComputeProgramObject_Particle);
		g_ComputeProgramObject_Particle = NULL;
	}

	if (g_ShaderProgramObject_Particle)
	{
		GLsizei iShaderCount;
		GLsizei iShaderNumber;

		glUseProgram(g_ShaderProgramObject_Particle);
		glGetProgramiv(g_ShaderProgramObject_Particle, GL_ATTACHED_SHADERS, &iShaderCount);
		GLuint* pShaders = (GLuint*)calloc(iShaderCount, sizeof(GLuint));

		if (pShaders)
		{
			glGetAttachedShaders(g_ShaderProgramObject_Particle, iShaderCount, &iShaderCount, pShaders);
			for (iShaderNumber = 0; iShaderNumber < iShaderCount; iShaderNumber++)
			{
				glDetachShader(g_ShaderProgramObject_Particle, pShaders[iShaderNumber]);
				glDeleteShader(pShaders[iShaderNumber]);
				pShaders[iShaderNumber] = 0;
			}
			free(pShaders);
			pShaders = NULL;
		}
		glUseProgram(0);
		glDeleteProgram(g_ShaderProgramObject_Particle);
		g_ShaderProgramObject_Particle = NULL;
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

