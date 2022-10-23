/*
    Copy right Xiyang Tan, USTC
	2022.5.22
	Final project for computer graphics
	Lake simulation
*/



#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <random>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <vector>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "Camera.h"
#include "Model.h";

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
unsigned int loadCubemap(std::vector<std::string> faces);
void printGLinfo() {
	std::cout << "///////////////// Local Hardware Information ///////////////////" << std::endl;
	
	GLint result;
	glGetIntegerv(GL_MAX_IMAGE_UNITS, &result);
	std::cout << "GL MAX_IMAGE_UNITS: " << result << std::endl;
	
	const GLubyte* vendor = glGetString(GL_VENDOR);
	std::cout << "GL VENDOR: " << vendor << std::endl;

	const GLubyte* renderer = glGetString(GL_RENDERER);
	std::cout << "GL RENDERER: " << renderer << std::endl;

	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "GL Version: " << version << std::endl;

	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	std::cout << "GL version (integer): " << major << "." << minor << std::endl;

	const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	std::cout << "GLSL Version: " << glslVersion << std::endl;

	///////////////////////// 获取最大的work group维数 ////////////////////////////////
	int workGroupSize[3], workGroupInv;
	// maximum globbal work group (total work in a dispatch)
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workGroupSize[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workGroupSize[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workGroupSize[2]);
	std::cout << "Max global (total) work group size x: "
		<< workGroupSize[0] << " y: "
		<< workGroupSize[1] << " z: "
		<< workGroupSize[2] << std::endl;
	// maximum local work group (one shader's slice)
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workGroupSize[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workGroupSize[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workGroupSize[2]);
	std::cout << "Max local (in one shader) work group size x: "
		<< workGroupSize[0] << " y: "
		<< workGroupSize[1] << " z: "
		<< workGroupSize[2] << std::endl;
	// maximum compute shader invocations (x * y * z)
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workGroupInv);
	std::cout << "Max computer shader invocations: " << workGroupInv << std::endl;
	std::cout << "///////////////// Local hardware information ///////////////////" << std::endl;
	/////////////////////////////////////////////////////////////////////////////////
}
//////////////////////  窗口动作处理  ///////////////////////////
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

////////////////////// 读文件的函数 /////////////////////////
std::string get_file_contents(const char* filename){
	std::ifstream in(filename, std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}
std::string vertexCode = get_file_contents("cubic.vert");
std::string fragmentCode = get_file_contents("cubic.frag");
std::string H0kComputeCode = get_file_contents("H0k.comp");
std::string HktComputeCode = get_file_contents("hkt.comp");
std::string ButterflyComputeCode = get_file_contents("ButterflyTex.comp");
std::string CopyComputeCode = get_file_contents("Copy.comp");
std::string PingPongComputeCode = get_file_contents("ButterflyCompute.comp");
std::string DisplacementCode = get_file_contents("Displacement.comp");
std::string SkyboxFragCode = get_file_contents("skybox.frag");
std::string SkyboxVertCode = get_file_contents("skybox.vert");

const char* vertexSource = vertexCode.c_str();
const char* fragmentSource = fragmentCode.c_str();
const char* H0kComputeSource = H0kComputeCode.c_str();
const char* HktComputeSource = HktComputeCode.c_str();
const char* ButterflyComputeSource = ButterflyComputeCode.c_str();
const char* CopyComputeSource = CopyComputeCode.c_str();
const char* PingPongComputeSource = PingPongComputeCode.c_str();
const char* DisplacementComputeSource = DisplacementCode.c_str();
const char* SkyboxVertSource = SkyboxVertCode.c_str();
const char* SkyboxFragSource = SkyboxFragCode.c_str();
////////////////////////////////////////////////////////////////////
 

//////////////////  设置窗口大小  //////////////////////
const unsigned int SCR_WIDTH = 1080;
const unsigned int SCR_HEIGHT = 720;
//////////////////////////////////////////////////////

///////////////// 网格大小 /////////////////////////
const unsigned int FourierGridSize = 256;
///////////////////// FFT 反序数组预计算 /////////////////
int get_computation_layers(int num) {
	int nLayers = 0;
	int len = num;
	if (len == 2) return 1;
	while (true) {
		len = len / 2;
		nLayers++;
		if (len == 2) return nLayers + 1;
		if (len < 1) return -1;
	}
}
auto reverse(int N) {
	std::vector<int> bit_reversed(N, 0);
	int index = 0;
	int r = get_computation_layers(N);
	for (int i = 0; i < N; i++) {
		index = 0;
		for (int m = r - 1; m >= 0; m--) {
			index += (1 && (i & (1 << m))) << (r - m - 1);
		}
		bit_reversed[i] = index;
	}
	return bit_reversed;
}

///////////////////////////////////////////////////////
//////////////// 摄像机 /////////////////
Camera camera(glm::vec3(0.0f, 0.2f, 1.0f));

float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
///////////////////////////////////////
/////////////// 光照 ///////////////////
glm::vec3 lightPos(0.0f, 500.0f, 0.0f);
///////////////////////////////////////
///////////////////////////////////////////////////////


int main(int argc, char** argv)
{
	std::vector<int> temp(FourierGridSize, 0);
	temp = reverse(FourierGridSize);
	int bit_reversed[FourierGridSize];
	std::copy(temp.begin(), temp.end(), bit_reversed);
	//////////////   一些初始化设定    /////////////////
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, argv[0], NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window, exiting..." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad:: load all OpenGL function pointers
	// ----------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD, exiting..." << std::endl;
		return -1;
	}

	printGLinfo();
	////////////////////////////////////////////////////////////////////////

	//////////////////// 读入天空盒数据 ///////////////////////
	std::vector<std::string> faces
	{
		"..\\skybox\\right.jpg",
		"..\\skybox\\left.jpg",
		"..\\skybox\\top.jpg",
		"..\\skybox\\bottom.jpg",
		"..\\skybox\\front.jpg",
		"..\\skybox\\back.jpg"
	};
	//std::vector<std::string> faces
	//{
	//	"..\\skybox\\cm2_right.png",
	//	"..\\skybox\\cm2_left.png",
	//	"..\\skybox\\cm2_top.png",
	//	"..\\skybox\\cm2_bottom.png",
	//	"..\\skybox\\cm2_front.png",
	//	"..\\skybox\\cm2_back.png"
	//};
	//std::vector<std::string> faces
	//{
	//	"..\\skybox\\dot_right.jpg",
	//	"..\\skybox\\dot_left.jpg",
	//	"..\\skybox\\dot_up.jpg",
	//	"..\\skybox\\dot_down.jpg",
	//	"..\\skybox\\dot_front.jpg",
	//	"..\\skybox\\dot_back.jpg"
	//};
	unsigned int cubemap;
	unsigned int cubemapTexture = loadCubemap(faces);
	//////////////////////////////////////////////////////////


	/////////////////// 创建，编译，链接着色器程序 ///////////////////
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	////////////////////////// 天空盒着色器 ////////////////////////////
	////////// 顶点 ////////////
	unsigned int SkyboxVertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(SkyboxVertShader, 1, &SkyboxVertSource, NULL);
	glCompileShader(SkyboxVertShader);
	glGetShaderiv(SkyboxVertShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(SkyboxVertShader, 512, NULL, infoLog);
		std::cout << "ERROR:SHADER::VERT::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	////////// 片元 ////////////
	unsigned int SkyboxFragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(SkyboxFragShader, 1, &SkyboxFragSource, NULL);
	glCompileShader(SkyboxFragShader);
	glGetShaderiv(SkyboxFragShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(SkyboxFragShader, 512, NULL, infoLog);
		std::cout << "ERROR:SHADER::FRAG::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	////////////////////////// 计算着色器 /////////////////////////////
	unsigned int H0kComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(H0kComputeShader, 1, &H0kComputeSource, NULL);
	glCompileShader(H0kComputeShader);
	glGetShaderiv(H0kComputeShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(H0kComputeShader, 512, NULL, infoLog);
		std::cout << "ERROR:SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	unsigned int HktComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(HktComputeShader, 1, &HktComputeSource, NULL);
	glCompileShader(HktComputeShader);
	glGetShaderiv(HktComputeShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(HktComputeShader, 512, NULL, infoLog);
		std::cout << "ERROR:SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	unsigned int ButterflyComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(ButterflyComputeShader, 1, &ButterflyComputeSource, NULL);
	glCompileShader(ButterflyComputeShader);
	glGetShaderiv(ButterflyComputeShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(ButterflyComputeShader, 512, NULL, infoLog);
		std::cout << "ERROR:SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}

	unsigned int PingPongComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(PingPongComputeShader, 1, &PingPongComputeSource, NULL);
	glCompileShader(PingPongComputeShader);
	glGetShaderiv(PingPongComputeShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(PingPongComputeShader, 512, NULL, infoLog);
		std::cout << "ERROR:SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	unsigned int DisplacementComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(DisplacementComputeShader, 1, &DisplacementComputeSource, NULL);
	glCompileShader(DisplacementComputeShader);
	glGetShaderiv(DisplacementComputeShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(DisplacementComputeShader, 512, NULL, infoLog);
		std::cout << "ERROR:SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}

	unsigned int CopyComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(CopyComputeShader, 1, &CopyComputeSource, NULL);
	glCompileShader(CopyComputeShader);
	glGetShaderiv(CopyComputeShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(CopyComputeShader, 512, NULL, infoLog);
		std::cout << "ERROR:SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	/////////////////////////////////////////////////////////////////


	//////////////////// 生成渲染程序传入GPU //////////////////////////
	unsigned int renderProgram = glCreateProgram();
	glAttachShader(renderProgram, vertexShader);
	glAttachShader(renderProgram, fragmentShader);
	glLinkProgram(renderProgram);


	glGetProgramiv(renderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(renderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::RENDER_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	glUseProgram(renderProgram);
	glUniform1i(glGetUniformLocation(renderProgram, "skybox"), 0);

	///////////////// 天空盒 ///////////////////
	unsigned int skyboxProgram = glCreateProgram();
	glAttachShader(skyboxProgram, SkyboxVertShader);
	glAttachShader(skyboxProgram, SkyboxFragShader);
	glLinkProgram(skyboxProgram);

	glGetProgramiv(skyboxProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(skyboxProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::SKYBOX_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	glUseProgram(skyboxProgram);
	glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);
	//////////////////////////////////////////////////////////////////////
  

	//////////////////// 生成计算着色器程序(GPGPU)传入GPU ///////////////////////////////
	//////////////////// H0k ////////////////////
	unsigned int H0kComputeProgram = glCreateProgram();
	glAttachShader(H0kComputeProgram, H0kComputeShader);
	glLinkProgram(H0kComputeProgram);

	glGetProgramiv(H0kComputeProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(H0kComputeProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPUTE_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	///////////////////////////////////////////////

	//////////////////// Hkt //////////////////////
	unsigned int HktComputeProgram = glCreateProgram();
	glAttachShader(HktComputeProgram, HktComputeShader);
	glLinkProgram(HktComputeProgram);

	glGetProgramiv(H0kComputeProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(H0kComputeProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPUTE_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	////////////////////////////////////////////////

	/////////////////// Butterfly ///////////////////
	unsigned int ButterflyComputeProgram = glCreateProgram();
	glAttachShader(ButterflyComputeProgram, ButterflyComputeShader);
	glLinkProgram(ButterflyComputeProgram);

	glGetProgramiv(ButterflyComputeProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(ButterflyComputeProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPUTE_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	////////////////////////////////////////////////

	///////////////// Ping-Pong ////////////////////
	unsigned int PingPongComputeProgram = glCreateProgram();
	glAttachShader(PingPongComputeProgram, PingPongComputeShader);
	glLinkProgram(PingPongComputeProgram);

	glGetProgramiv(PingPongComputeProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(PingPongComputeProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPUTE_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	///////////////////////////////////////////////////

	///////////////// Displacement //////////////////
	unsigned int DisplacementComputeProgram = glCreateProgram();
	glAttachShader(DisplacementComputeProgram, DisplacementComputeShader);
	glLinkProgram(DisplacementComputeProgram);

	glGetProgramiv(DisplacementComputeProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(DisplacementComputeProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPUTE_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	//////////////////////////////////////////////////
	//////////////////// Copy /////////////////////////
	unsigned int CopyComputeProgram = glCreateProgram();
	glAttachShader(CopyComputeProgram, CopyComputeShader);
	glLinkProgram(CopyComputeProgram);

	glGetProgramiv(CopyComputeProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(CopyComputeProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPUTE_PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		exit(1);
	}
	//////////////////////////////////////////////////
	/////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////


	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(H0kComputeShader);
	glDeleteShader(HktComputeShader);
	glDeleteShader(ButterflyComputeShader);
	glDeleteShader(CopyComputeShader);
	glDeleteShader(PingPongComputeShader);
	glDeleteShader(DisplacementComputeShader);


	glEnable(GL_DEPTH_TEST);
	//////////////////////////////////////////////////////////////////////////////////
	/*float cubeVertices[] = {
		// positions          // normals
		-.5f, -.5f, -.5f,  0.0f,  0.0f, -1.0f,
		 .5f, -.5f, -.5f,  0.0f,  0.0f, -1.0f,
		 .5f,  .5f, -.5f,  0.0f,  0.0f, -1.0f,
		 .5f,  .5f, -.5f,  0.0f,  0.0f, -1.0f,
		-.5f,  .5f, -.5f,  0.0f,  0.0f, -1.0f,
		-.5f, -.5f, -.5f,  0.0f,  0.0f, -1.0f,

		-.5f, -.5f,  .5f,  0.0f,  0.0f, 1.0f,
		 .5f, -.5f,  .5f,  0.0f,  0.0f, 1.0f,
		 .5f,  .5f,  .5f,  0.0f,  0.0f, 1.0f,
		 .5f,  .5f,  .5f,  0.0f,  0.0f, 1.0f,
		-.5f,  .5f,  .5f,  0.0f,  0.0f, 1.0f,
		-.5f, -.5f,  .5f,  0.0f,  0.0f, 1.0f,

		-.5f,  .5f,  .5f, -1.0f,  0.0f,  0.0f,
		-.5f,  .5f, -.5f, -1.0f,  0.0f,  0.0f,
		-.5f, -.5f, -.5f, -1.0f,  0.0f,  0.0f,
		-.5f, -.5f, -.5f, -1.0f,  0.0f,  0.0f,
		-.5f, -.5f,  .5f, -1.0f,  0.0f,  0.0f,
		-.5f,  .5f,  .5f, -1.0f,  0.0f,  0.0f,

		 .5f,  .5f,  .5f,  1.0f,  0.0f,  0.0f,
		 .5f,  .5f, -.5f,  1.0f,  0.0f,  0.0f,
		 .5f, -.5f, -.5f,  1.0f,  0.0f,  0.0f,
		 .5f, -.5f, -.5f,  1.0f,  0.0f,  0.0f,
		 .5f, -.5f,  .5f,  1.0f,  0.0f,  0.0f,
		 .5f,  .5f,  .5f,  1.0f,  0.0f,  0.0f,

		-.5f, -.5f, -.5f,  0.0f, -1.0f,  0.0f,
		 .5f, -.5f, -.5f,  0.0f, -1.0f,  0.0f,
		 .5f, -.5f,  .5f,  0.0f, -1.0f,  0.0f,
		 .5f, -.5f,  .5f,  0.0f, -1.0f,  0.0f,
		-.5f, -.5f,  .5f,  0.0f, -1.0f,  0.0f,
		-.5f, -.5f, -.5f,  0.0f, -1.0f,  0.0f,

		-.5f,  .5f, -.5f,  0.0f,  1.0f,  0.0f,
		 .5f,  .5f, -.5f,  0.0f,  1.0f,  0.0f,
		 .5f,  .5f,  .5f,  0.0f,  1.0f,  0.0f,
		 .5f,  .5f,  .5f,  0.0f,  1.0f,  0.0f,
		-.5f,  .5f,  .5f,  0.0f,  1.0f,  0.0f,
		-.5f,  .5f, -.5f,  0.0f,  1.0f,  0.0f
	};*/
	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	// skybox VAO
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// cube VAO
	/*unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	*/
	// load model
	//Model surface("surface.obj");
	Model surface("surface.obj");
	//std::cout << surface.textures_loaded.size() << std::endl;
	
	/*float vertices[] = {
		//// 坐标 ////   //// 纹理坐标 ////
		 1.0f,  1.0f,      1.0f, 1.0f,
		 1.0f, -1.0f,      1.0f, 0.0f,
		-1.0f, -1.0f,      0.0f, 0.0f,
		-1.0f,  1.0f,      0.0f, 1.0f
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,  // first Triangle
		1, 2, 3   // second Triangle
	};
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);
	*/

	////////////////////////// 设置H0k.comp的输出纹理[h0(k)和h0(-k)] ///////////////////////////////
	int H0k_tex_w = FourierGridSize;
	int H0k_tex_h = FourierGridSize;
	//unsigned int tilde0;
	///////////////////////////////// 传入辅助噪声纹理 /////////////////////////////////////////////////
	unsigned int tex0; // contains the noise texture, and texture h0k.
	glGenTextures(1, &tex0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex0);
	 
	int w_noise, h_noise, nrChannels;
	unsigned char* data = stbi_load("noise1.png", &w_noise, &h_noise, &nrChannels, 0);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, w_noise, h_noise, 2);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// 在网站https://aeroson.github.io/rgba-noise-image-generator/里可以生成这个"noise1.png"
	// 生成时设置参数: width不能小于这里的FourierGridSize, height不能小于这里的FourierGridSize
	// IMPORTANT: 首先我们需要关闭G, B, A通道, 然后要把transform [-1, 1] range to: 调成-0.35, 1.35
	// 这样就可以生成我们的noise1.png
	if (data) {
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, w_noise, h_noise, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glBindImageTexture(0, tex0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

	}
	else std::cout << "Failed to load noise texture r0." << std::endl;
	stbi_image_free(data);
	///////////////////////////////////////////////////////////////////////




	///////////////////////////////// 设置hkt.comp的相关纹理 /////////////////////////////////////////
	////////// hkt /////////////
	unsigned int tex1; // tex1 contains 3 layers. The first layer is tilde_hkt, the second is tilde_disp, and the third is tilde_slope. 
	glGenTextures(1, &tex1); 
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex1);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, FourierGridSize, FourierGridSize, 3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindImageTexture(1, tex1, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	///////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////// 设置辅助纹理 ////////////////////////////////////
	unsigned int tex2; // contains butterfly texture, pingpong0 and pingpong1
	glGenTextures(1, &tex2);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex2);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, FourierGridSize, FourierGridSize, 3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindImageTexture(2, tex2, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

	///////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////// 设置choppy waves以及slope的ifft后产生的纹理 /////////////////////////
	unsigned int tex3; // contains tex_disp_x (0), tex_disp_z (1), tex_slope_x (2), tex_slope_z (3)
	glGenTextures(1, &tex3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex3);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, FourierGridSize, FourierGridSize, 4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindImageTexture(3, tex3, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);


	glUseProgram(H0kComputeProgram);
	glDispatchCompute((GLuint)H0k_tex_w, (GLuint)H0k_tex_h, 1);
	glUseProgram(ButterflyComputeProgram);
	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(bit_reversed), bit_reversed, GL_STATIC_READ);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
	glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);
	
	//////////////////////////////////////////////
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	///////////////// 游戏循环 //////////////////////
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = static_cast<float>(glfwGetTime());
		double lastTime = glfwGetTime();
		int nbFrames = 0;

		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0) {
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		// input
		// -----
		processInput(window);

		
		// 设置计算着色器变量在这里 //
		glUseProgram(HktComputeProgram);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);
		GLfloat timeValue = glfwGetTime() + 1000.f; // 设置时间
		int timeLocation = glGetUniformLocation(HktComputeProgram, "time");
		glUniform1f(timeLocation, timeValue);

		glUseProgram(CopyComputeProgram);
		int statusLocation = glGetUniformLocation(CopyComputeProgram, "status");
		glUniform1i(statusLocation, 1);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

		
		int pingpong = 0;
		int direction = 0;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);
	
			pingpong++;
			pingpong = pingpong % 2;
			
		}
		direction = 1;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);
			
			pingpong++;
			pingpong = pingpong % 2;
		}
		glUseProgram(DisplacementComputeProgram);
		int pingpongLocation = glGetUniformLocation(DisplacementComputeProgram, "pingpong");
		glUniform1i(pingpongLocation, pingpong);
		int whichLocation = glGetUniformLocation(DisplacementComputeProgram, "which");
		glUniform1i(whichLocation, 1);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);
		
		//
		glUseProgram(CopyComputeProgram);
		statusLocation = glGetUniformLocation(CopyComputeProgram, "status");
		glUniform1i(statusLocation, 2);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);
		
		pingpong = 0;
		direction = 0;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

			pingpong++;
			pingpong = pingpong % 2;

		}
		direction = 1;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

			pingpong++;
			pingpong = pingpong % 2;
		}
		glUseProgram(DisplacementComputeProgram);
		pingpongLocation = glGetUniformLocation(DisplacementComputeProgram, "pingpong");
		glUniform1i(pingpongLocation, pingpong);
		whichLocation = glGetUniformLocation(DisplacementComputeProgram, "which");
		glUniform1i(whichLocation, 2);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

		//
		glUseProgram(CopyComputeProgram);
		statusLocation = glGetUniformLocation(CopyComputeProgram, "status");
		glUniform1i(statusLocation, 3);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

		pingpong = 0;
		direction = 0;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

			pingpong++;
			pingpong = pingpong % 2;

		}
		direction = 1;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

			pingpong++;
			pingpong = pingpong % 2;
		}
		glUseProgram(DisplacementComputeProgram);
		pingpongLocation = glGetUniformLocation(DisplacementComputeProgram, "pingpong");
		glUniform1i(pingpongLocation, pingpong);
		whichLocation = glGetUniformLocation(DisplacementComputeProgram, "which");
		glUniform1i(whichLocation, 3);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);
		//
		glUseProgram(CopyComputeProgram);
		statusLocation = glGetUniformLocation(CopyComputeProgram, "status");
		glUniform1i(statusLocation, 4);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

		pingpong = 0;
		direction = 0;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

			pingpong++;
			pingpong = pingpong % 2;

		}
		direction = 1;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

			pingpong++;
			pingpong = pingpong % 2;
		}
		glUseProgram(DisplacementComputeProgram);
		pingpongLocation = glGetUniformLocation(DisplacementComputeProgram, "pingpong");
		glUniform1i(pingpongLocation, pingpong);
		whichLocation = glGetUniformLocation(DisplacementComputeProgram, "which");
		glUniform1i(whichLocation, 4);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);
		//
		glUseProgram(CopyComputeProgram);
		statusLocation = glGetUniformLocation(CopyComputeProgram, "status");
		glUniform1i(statusLocation, 5);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

		pingpong = 0;
		direction = 0;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

			pingpong++;
			pingpong = pingpong % 2;

		}
		direction = 1;
		for (int i = 0; i < log2(FourierGridSize); ++i) {
			glUseProgram(PingPongComputeProgram);
			int stageLocation = glGetUniformLocation(PingPongComputeProgram, "stage");
			int pingpongLocation = glGetUniformLocation(PingPongComputeProgram, "pingpong");
			int directionLocation = glGetUniformLocation(PingPongComputeProgram, "direction");
			glUniform1i(stageLocation, i);
			glUniform1i(pingpongLocation, pingpong);
			glUniform1i(directionLocation, direction);
			glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);

			pingpong++;
			pingpong = pingpong % 2;
		}
		glUseProgram(DisplacementComputeProgram);
		pingpongLocation = glGetUniformLocation(DisplacementComputeProgram, "pingpong");
		glUniform1i(pingpongLocation, pingpong);
		whichLocation = glGetUniformLocation(DisplacementComputeProgram, "which");
		glUniform1i(whichLocation, 5);
		glDispatchCompute((GLuint)FourierGridSize, (GLuint)FourierGridSize, 1);




		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		
		/////////////////////////
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		/////////////// 渲染纹理，查看输出 /////////////////
		//glBindVertexArray(VAO); 
		//glUseProgram(renderProgram);
		//////// 设置摄像机 /////////
		glUseProgram(renderProgram);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glUniformMatrix4fv(glGetUniformLocation(renderProgram, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(renderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniform3fv(glGetUniformLocation(renderProgram, "cameraPos"), 1, &camera.Position[0]);
		//////////////////////////
		////////// 光照 ///////////
		// 光源信息
		glUniform3fv(glGetUniformLocation(renderProgram, "light.position"), 1, &lightPos[0]);
		glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
		glm::vec3 diffuseColor = lightColor * glm::vec3(.7f); // decrease the influence
		glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f); // low influence
		glUniform3fv(glGetUniformLocation(renderProgram, "light.ambient"), 1, &ambientColor[0]);
		glUniform3fv(glGetUniformLocation(renderProgram, "light.diffuse"), 1, &diffuseColor[0]);
		glUniform3fv(glGetUniformLocation(renderProgram, "light.specular"), 1, &glm::vec3(1.0f, 1.0f, 1.0f)[0]);
		// 材质信息
		glUniform3fv(glGetUniformLocation(renderProgram, "material.ambient"), 1, &glm::vec3(.12f, 0.53f, 1.f)[0]);
		glUniform3fv(glGetUniformLocation(renderProgram, "material.diffuse"), 1, &glm::vec3(.9f, .9f, .9f)[0]);
		glUniform3fv(glGetUniformLocation(renderProgram, "material.specular"), 1, &glm::vec3(0.25f, 0.25f, 0.25f)[0]);
		glUniform1f(glGetUniformLocation(renderProgram, "material.shininess"), 32.0f);

		//////////////////////////
		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
		model = glm::scale(model, glm::vec3(.1f, .1f, .1f));	// it's a bit too big for our scene, so scale it down
		glUniformMatrix4fv(glGetUniformLocation(renderProgram, "model"), 1, GL_FALSE, &model[0][0]);
		surface.Draw(renderProgram);
		// cubes
		//glBindVertexArray(cubeVAO);
		//glDrawArrays(GL_TRIANGLES, 0, 36);
		//glBindVertexArray(0);
		
		///////////////////////////
		/////////////// 在这里修改要渲染的纹理 //////////////
		//glBindTexture(GL_TEXTURE_2D_ARRAY, tex1);
		//glBindTexture(GL_TEXTURE_2D_ARRAY, tex2);
		//gBindTexture(GL_TEXTURE_2D_ARRAY, tex3);
		///////////////////////////////////////
		/////////////////////// 天空盒 /////////////////////
		glDepthFunc(GL_LEQUAL);
		glUseProgram(skyboxProgram);
		view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
		glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		// skybox cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default

		
		
		///////////////////////////////////////
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		//////////////////////////
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &skyboxVAO);
	//glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &skyboxVBO);
	//glDeleteBuffers(1, &VBO);
	//glDeleteBuffers(1, &EBO);
	glDeleteProgram(renderProgram);
	glDeleteProgram(H0kComputeProgram);
	glDeleteProgram(HktComputeProgram);

	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}
// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}


unsigned int loadCubemap(std::vector<std::string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}