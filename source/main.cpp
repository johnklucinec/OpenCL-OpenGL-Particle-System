#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <array>
#include <chrono>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "cl_utils.h"
#include "gl_utils.h"
#include <imGuIZMOquat.h>
#include <iostream>
#include <string>
#include <vector>

#include "cl.h"
#include "cl_gl.h"

// Platform-specific includes
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#else  // <--- Linux
#include <GL/glx.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Aliases for vectors/colors
inline xyzw XYZW(float, float, float, float = 1.0f);  // x, y, z, w (optional)
inline rgba RGBA(float, float, float, float = 1.0f);  // r, g, b, a (optional)

// ==================================================
// Constants:
// ==================================================

// Window and Rendering Constants
constexpr const char* WINDOW_TITLE = "OpenCL/OpenGL Particle System";
constexpr int         WINDOW_SIZE  = 800;
constexpr float       FOV          = 50.0f;
constexpr float       FAR_LOOK     = 5000.0f;
constexpr float       AXES_LENGTH  = 150.0f;
constexpr glm::vec3   AXES_COLOR   = glm::vec3(1.0f, 0.5f, 0.0f);
constexpr bool        VSYNC        = false;

// Particle System Constants
constexpr int         NUM_PARTICLES   = (1024) * 1024;
constexpr int         LOCAL_SIZE      = 1;
constexpr const char* CL_FILE_NAME    = "particles.cl";
constexpr const char* CL_BINARY_NAME  = "particles.nv";
static constexpr int  WARMUP_FRAMES   = 100;
static constexpr bool PRINTINFO       = false;  // Might be useful
constexpr int         STEPS_PER_FRAME = 1;      // How many times should we talk to the GPU per frame?
constexpr float       BASE_DT         = 0.05f;  // Change particle speed

// ==================================================
// Grouped State Structures:
// ==================================================

struct AppState
{
  bool  showUI      = false;  // Toggle UI
  bool  axesOn      = false;  // ON or OFF
  bool  paused      = true;   // Animation paused state
  bool  perspective = true;   // ORTHO or PERSP
  bool  performance = false;  // Show performance
  float mouseX      = 0.0f;   // Mouse Values
  float mouseY      = 0.0f;   // Mouse Values
};

enum ButtonID
{
  GO,
  PAUSE,
  RESET,
  QUIT
};

struct Camera
{
  float     distance = 800.0f;                          // Distance for zooming
  float     panX     = 0.0f;                            // Camera pan values
  float     panY     = 0.0f;                            // Camera pan values
  float     theta    = 0.0f;                            // Horizontal rotation angle (radians)
  float     phi      = glm::radians(97.0f);             // Vertical rotation angle
  glm::vec3 lookAt   = glm::vec3(0.0f, -100.0f, 0.0f);  // Point camera orbits around
};

struct ComputeConfig
{
  size_t globalWorkSize[3] = {NUM_PARTICLES, 1, 1};
  size_t localWorkSize[3]  = {LOCAL_SIZE, 1, 1};
};

// OpenCL Objects
struct OpenCL
{
  cl_platform_id   platform = nullptr;
  cl_device_id     device   = nullptr;
  cl_context       context  = nullptr;
  cl_command_queue cmdQueue = nullptr;
  cl_program       program  = nullptr;
  cl_kernel        kernel   = nullptr;
};

struct ParticleBuffers
{
  GLuint particleVAO = 0;
  GLuint posGL       = 0;        // OpenGL position buffer (hPobj)
  GLuint colorGL     = 0;        // OpenGL color buffer (hCobj)
  cl_mem posCL       = nullptr;  // OpenCL position buffer (dPobj)
  cl_mem colorCL     = nullptr;  // OpenCL color buffer (dCobj)
  xyzw*  velHost     = nullptr;  // Host velocity (hVel)
  cl_mem velCL       = nullptr;  // OpenCL velocity buffer (dVel)
};

struct PerfMetrics
{
  double elapsedTime              = 0.0;
  float  peakGigaParticlesPerSec  = 0.0f;
  float  totalGigaParticlesPerSec = 0.0f;
  int    frameCount               = 0;
};


struct RenderState
{
  GLuint axesVAO           = 0;  // VAO for axes
  GLuint axesVBO           = 0;  // VBO for axes
  int    axesVertexCount   = 0;  // Number of vertices for axes
  GLuint sphereVAO         = 0;  // VAO for all spheres
  GLuint sphereVBO         = 0;  // VBO for all spheres
  int    sphereVertexCount = 0;  // Number of vertices for all spheres
  GLuint shaderProgram     = 0;  // Shader program
  GLint  uModelMatrix      = -1;
  GLint  uViewMatrix       = -1;
  GLint  uProjMatrix       = -1;
};

struct Transform
{
  glm::quat            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // w,x,y,z identity
  std::array<float, 3> position = {0.0f, 0.0f, 0.0f};                 // Set by ImGui widgets
  float                scale    = 1.0f;                               // Object scaling factor
};

struct Random
{
  const float XMIN = -100.0f;
  const float XMAX = 100.0f;
  const float YMIN = -100.0f;
  const float YMAX = 100.0f;
  const float ZMIN = -100.0f;
  const float ZMAX = 100.0f;
  const float VMIN = -100.0f;
  const float VMAX = 100.0f;
};

// Global State
AppState        app;
Camera          camera;
ComputeConfig   compute;
GLFWwindow*     window = nullptr;
OpenCL          cl;
ParticleBuffers particle;
PerfMetrics     perf;
Random          r;
RenderState     gfx;
Transform       model;

// Function Prototypes
void Animate();
void Buttons(int);
void Render();
void InitCL();
void InitGraphics();
void InitImGui();
void InitLists();
void InitShaders();
void MainLoop();
void PerformanceOverlay();
void Quit();
void RenderUI();
void Reset();
void ResetParticles();

// Helper Function Prototypes
void  CursorPosition(GLFWwindow*, double, double);
void  Keyboard(GLFWwindow*, int, int, int, int);
void  MouseButton(GLFWwindow*, int, int, int);
float Ranf(float, float);


// ==================================================
// Main Program:
// ==================================================
int main(int argc, char* argv[])
{
  InitGraphics();
  InitLists();
  InitCL();
  InitImGui();
  Reset();
  MainLoop();
  Quit();
  return 0;
}

// Main rendering loop
void MainLoop()
{
  while(!glfwWindowShouldClose(window))
  {
    Animate();
    Render();
  }
}

// Animation updates
void Animate()
{
  if(app.paused)
    return;

  cl_int status;
  glFinish();  // Ensure GL is done with shared buffers

  // OpenCL aquires the buffer
  cl_mem glObjects[2] = {particle.posCL, particle.colorCL};
  status              = clEnqueueAcquireGLObjects(cl.cmdQueue, 2, glObjects, 0, NULL, NULL);
  CLUtils::PrintCLError(status, "clEnqueueAcquireGLObjects: ");

  float dt = BASE_DT / STEPS_PER_FRAME;  // Update dt before launching kernels
  clSetKernelArg(cl.kernel, 3, sizeof(float), &dt);

  auto time0 = std::chrono::steady_clock::now();  // Start timing after acquire is enqueued

  for(int i = 0; i < STEPS_PER_FRAME; i++)
  {
    status = clEnqueueNDRangeKernel(cl.cmdQueue, cl.kernel, 1, NULL, compute.globalWorkSize, compute.localWorkSize, 0, NULL, NULL);
    CLUtils::PrintCLError(status, "clEnqueueNDRangeKernel: ");
  }

  // OpenCL releases the buffer
  status = clEnqueueReleaseGLObjects(cl.cmdQueue, 2, glObjects, 0, NULL, NULL);
  CLUtils::PrintCLError(status, "clEnqueueReleaseGLObjects: ");

  clFinish(cl.cmdQueue);

  auto time1 = std::chrono::steady_clock::now();

  if(app.performance)
    perf.elapsedTime = std::chrono::duration<double>(time1 - time0).count() / STEPS_PER_FRAME;
}


// ==================================================
// Draw the Complete Scene:
// ==================================================
void Render()
{
  // Update FPS and window title
  GLUtils::UpdateFPS(window, WINDOW_TITLE);

  glfwPollEvents();

  // ImGui frame setup
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  RenderUI();
  PerformanceOverlay();
  ImGui::Render();

  // OpenGL rendering setup
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);

  // Create square viewport (centered) to maintain aspect ratio
  GLsizei min = (std::min)(display_w, display_h);  // minimum dimension
  GLint   xl  = (display_w - min) / 2;
  GLint   yb  = (display_h - min) / 2;
  glViewport(xl, yb, min, min);

  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up projection matrix
  float     halfHeight = camera.distance * tan(glm::radians(FOV / 2.0f));
  glm::mat4 projMatrix = app.perspective ? glm::perspective(glm::radians(FOV), 1.0f, 0.1f, FAR_LOOK) :
                                           glm::ortho(-halfHeight, halfHeight, -halfHeight, halfHeight, 0.1f, FAR_LOOK);

  // Calculate camera position using spherical coordinates
  glm::vec3 cameraOffset(camera.distance * sin(camera.phi) * sin(camera.theta),  //
                         camera.distance * cos(camera.phi),                      //
                         camera.distance * sin(camera.phi) * cos(camera.theta)   //
  );

  // Apply panning by offsetting both camera and look-at point
  glm::vec3 panOffset(camera.panX, camera.panY, 0.0f);
  glm::vec3 cameraPos = camera.lookAt + cameraOffset + panOffset;
  glm::vec3 lookAt    = camera.lookAt + panOffset;

  // Set up view matrix
  glm::mat4 viewMatrix = glm::lookAt(cameraPos, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));

  // Use shader program and set uniforms
  glUseProgram(gfx.shaderProgram);
  glUniformMatrix4fv(gfx.uProjMatrix, 1, GL_FALSE, glm::value_ptr(projMatrix));  // Proj Matrix
  glUniformMatrix4fv(gfx.uViewMatrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));  // View Matrix

  // Draw axes if enabled
  if(app.axesOn)
  {
    // Render axes at world origin with no transformations
    glm::mat4 axesModel = glm::mat4(1.0f);                                         // Identity
    glUniformMatrix4fv(gfx.uModelMatrix, 1, GL_FALSE, glm::value_ptr(axesModel));  // Model Matrix

    glLineWidth(3.0f);
    glBindVertexArray(gfx.axesVAO);
    glDrawArrays(GL_LINES, 0, gfx.axesVertexCount);
    glBindVertexArray(0);
    glLineWidth(1.0f);
  }

  // Set up model matrix (apply transformations)
  glm::mat4 modelMatrix = glm::mat4(1.0f);
  modelMatrix = glm::translate(modelMatrix, glm::vec3(model.position[0], model.position[1], -model.position[2]));
  modelMatrix = modelMatrix * glm::mat4_cast(model.rotation);  // Convert quaternion to rotation matrix
  modelMatrix = glm::scale(modelMatrix, glm::vec3(model.scale));
  glUniformMatrix4fv(gfx.uModelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));  // Model Matrix

  // ==================================================
  // Here is where you draw your spheres.
  // You define them, however, in the InitLists( ) function.
  // ==================================================
  glBindVertexArray(gfx.sphereVAO);
  glDrawArrays(GL_LINES, 0, gfx.sphereVertexCount);
  glBindVertexArray(0);

  // ==================================================
  // Here is where you draw the current state of the particles:
  // ==================================================
  glPointSize(2.0f);  // Size of particles
  glBindVertexArray(particle.particleVAO);
  glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
  glBindVertexArray(0);

  // Render ImGui on top
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);
}

// ==================================================
// Initialize the Geometry That Will Not Change:
// ==================================================
void InitLists()
{
  using namespace GLUtils;
  std::vector<float> sphereData;  // Temp storage for building sphere geometry

  // ****************************************
  // Here is where the first sphere is drawn.
  // It is centered at (-100,-800.,0.) with a radius of 600.
  // This had better match the definition of the first sphere
  // in the .cl file
  // ****************************************
  SphereConfig sphere1 = {
      XYZW(-100.0f, -800.0f, 0.0f),  // position (xyz)
      600.0f,                        // radius
      RGBA(0.9f, 0.9f, 0.0f)         // color (rgb)
  };

  DrawWireSphere(sphereData, sphere1);

  // ****************************************
  // Here is where the second sphere is drawn.
  // This had better match the definition of the second sphere
  // in the .cl file
  // ****************************************

  // do this for yourself...

  // Upload all sphere geometry to GPU
  FinalizeSpheres(sphereData, gfx.sphereVAO, gfx.sphereVBO, gfx.sphereVertexCount);

  // Initialize axes
  InitAxes(gfx.axesVAO, gfx.axesVBO, gfx.axesVertexCount, AXES_LENGTH, AXES_COLOR);
}


// ==================================================
// Initialize the OpenCL Stuff:
// ==================================================
void InitCL()
{
  // Show us platform and device information
  if(PRINTINFO)
    CLUtils::PrintOpenclInfo();

  // 1.  Select OpenCL device and initialize Platform and Device globals:
  // ==================================================

  CLUtils::SelectOpenclDevice(&cl.platform, &cl.device);

  // Exit early if OpenCL kernel file cannot be opened
  if(std::ifstream fp{CL_FILE_NAME}; !fp)
  {
    fprintf(stderr, "Cannot open OpenCL source file '%s'\n", CL_FILE_NAME);
    return;
  }


  // 2.  Allocate the host memory buffers:
  // ==================================================

  // OpenCL function return value (check against CL_SUCCESS)
  cl_int status;

  // Since this is an opengl interoperability program,
  // check if the OpenGL sharing extension is supported.
  // (we need the Device in order to ask, so can't do it any sooner than here)
  bool hasGLSharing = CLUtils::IsCLExtensionSupported("cl_khr_gl_sharing", cl.device)        // Windows
                      || CLUtils::IsCLExtensionSupported("cl_APPLE_gl_sharing", cl.device);  // MacOS

  if(!hasGLSharing)
  {
    fprintf(stderr, "OpenGL sharing extension is not supported -- sorry.\n");
    return;  // No point going on if it isn't
  }
  fprintf(stderr, "OpenGL sharing extension is supported.\n");


  // 3.  Create an OpenCL context based on the OpenGL context:
  // ==================================================

#if defined(_WIN32)
  // Windows: Use WGL context
  cl_context_properties props[] = {CL_GL_CONTEXT_KHR,
                                   (cl_context_properties)wglGetCurrentContext(),
                                   CL_WGL_HDC_KHR,
                                   (cl_context_properties)wglGetCurrentDC(),
                                   CL_CONTEXT_PLATFORM,
                                   (cl_context_properties)cl.platform,
                                   0};
#elif defined(__APPLE__)
  // macOS: Use CGL sharegroup
  CGLContextObj    cglContext    = CGLGetCurrentContext();
  CGLShareGroupObj cglShareGroup = CGLGetShareGroup(cglContext);
  cl_context_properties props[] = {CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)cglShareGroup, 0};
#else
  // Linux: Use GLX context
  cl_context_properties props[] = {CL_GL_CONTEXT_KHR,
                                   (cl_context_properties)glXGetCurrentContext(),
                                   CL_GLX_DISPLAY_KHR,
                                   (cl_context_properties)glXGetCurrentDisplay(),
                                   CL_CONTEXT_PLATFORM,
                                   (cl_context_properties)cl.platform,
                                   0};
#endif

  cl.context = clCreateContext(props, 1, &cl.device, nullptr, nullptr, &status);
  CLUtils::PrintCLError(status, "clCreateContext: ");


  // 4.  Create an OpenCL command queue:
  // ==================================================

  cl.cmdQueue = clCreateCommandQueue(cl.context, cl.device, 0, &status);
  if(status != CL_SUCCESS)
    fprintf(stderr, "clCreateCommandQueue failed\n");

  // Allocate host-side velocity array (not shared with OpenGL, stays in CPU memory)
  delete[] particle.velHost;
  particle.velHost = new xyzw[NUM_PARTICLES];

  // Create position VBO - will be shared with OpenCL
  glGenBuffers(1, &particle.posGL);
  glBindBuffer(GL_ARRAY_BUFFER, particle.posGL);
  glBufferData(GL_ARRAY_BUFFER, 4 * NUM_PARTICLES * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

  // Create color VBO - will be shared with OpenCL
  glGenBuffers(1, &particle.colorGL);
  glBindBuffer(GL_ARRAY_BUFFER, particle.colorGL);
  glBufferData(GL_ARRAY_BUFFER, 4 * NUM_PARTICLES * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind the buffer

  // Create and configure particle VAO
  glGenVertexArrays(1, &particle.particleVAO);
  glBindVertexArray(particle.particleVAO);

  // Bind position buffer and set attribute pointer
  glBindBuffer(GL_ARRAY_BUFFER, particle.posGL);
  glEnableVertexAttribArray(0);  // attribute location 0 (aVertex)
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Bind color buffer and set attribute pointer
  glBindBuffer(GL_ARRAY_BUFFER, particle.colorGL);
  glEnableVertexAttribArray(1);  // attribute location 1 (aColor)
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Unbind
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);


  // 5a.  Create the OpenCL version of the OpenGL buffers:
  // ==================================================

  particle.posCL = clCreateFromGLBuffer(cl.context, CL_MEM_READ_WRITE, particle.posGL, &status);
  CLUtils::PrintCLError(status, "clCreateFromGLBuffer (1)");

  particle.colorCL = clCreateFromGLBuffer(cl.context, CL_MEM_READ_WRITE, particle.colorGL, &status);
  CLUtils::PrintCLError(status, "clCreateFromGLBuffer (2)");


  // 5b. Create the opencl version of the velocity array:
  // ==================================================

  particle.velCL = clCreateBuffer(cl.context, CL_MEM_READ_WRITE, 4 * sizeof(float) * NUM_PARTICLES, nullptr, &status);
  CLUtils::PrintCLError(status, "clCreateBuffer: ");

  ResetParticles();  // Fill those arrays and buffers

  // 6.  Enqueue the command to write the data from the host buffers to the Device buffers:
  // ==================================================

  cl_event writeEvent;
  status = clEnqueueWriteBuffer(cl.cmdQueue, particle.velCL, CL_FALSE, 0, 4 * sizeof(float) * NUM_PARTICLES,
                                particle.velHost, 0, nullptr, &writeEvent);
  CLUtils::PrintCLError(status, "clEnqueueWriteBuffer: ");

  // 7.  Read the Kernel code from a file:
  // ==================================================

  std::ifstream fp{CL_FILE_NAME, std::ios::binary | std::ios::ate};
  size_t        fileSize = static_cast<size_t>(fp.tellg());
  fp.seekg(0, std::ios::beg);

  char* clProgramText = new char[fileSize + 1];  // leave room for '\0'
  fp.read(clProgramText, fileSize);
  clProgramText[fileSize] = '\0';
  fp.close();

  // Create the text for the Kernel Program:
  const char* strings[] = {clProgramText};
  cl.program            = clCreateProgramWithSource(cl.context, 1, strings, nullptr, &status);
  if(status != CL_SUCCESS)
    fprintf(stderr, "clCreateProgramWithSource failed\n");
  delete[] clProgramText;


  // 8.  Compile and link the Kernel code:
  // ==================================================

  const char* options = "-cl-fast-relaxed-math";
  status              = clBuildProgram(cl.program, 1, &cl.device, options, nullptr, nullptr);
  if(status != CL_SUCCESS)
  {
    size_t size;
    clGetProgramBuildInfo(cl.program, cl.device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &size);
    cl_char* log = new cl_char[size];
    clGetProgramBuildInfo(cl.program, cl.device, CL_PROGRAM_BUILD_LOG, size, log, nullptr);
    fprintf(stderr, "clBuildProgram failed:\n%s\n", log);
    delete[] log;
  }


  // 9.  Create the Kernel object:
  // ==================================================

  cl.kernel = clCreateKernel(cl.program, "Particle", &status);
  CLUtils::PrintCLError(status, "clCreateKernel failed: ");


  // 10. Setup the arguments to the Kernel object:
  // ==================================================

  status = clSetKernelArg(cl.kernel, 0, sizeof(cl_mem), &particle.posCL);
  CLUtils::PrintCLError(status, "clSetKernelArg (1): ");

  status = clSetKernelArg(cl.kernel, 1, sizeof(cl_mem), &particle.velCL);
  CLUtils::PrintCLError(status, "clSetKernelArg (2): ");

  status = clSetKernelArg(cl.kernel, 2, sizeof(cl_mem), &particle.colorCL);
  CLUtils::PrintCLError(status, "clSetKernelArg (3): ");

  float dt = BASE_DT / STEPS_PER_FRAME;
  status   = clSetKernelArg(cl.kernel, 3, sizeof(float), &dt);
  CLUtils::PrintCLError(status, "clSetKernelArg (4): ");

  // Wait for velocity buffer write to complete
  clWaitForEvents(1, &writeEvent);
  clReleaseEvent(writeEvent);
}


// ==================================================
// Initialize ImGui context and backends
// ==================================================
void InitImGui()
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io    = ImGui::GetIO();
  io.IniFilename = nullptr;  // Don't create an INI file
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 410");
}


// ==================================================
// Initialize GLFW Window and OpenGL Context:
// ==================================================
void InitGraphics()
{
  std::cout << "Starting OpenGL Application..." << std::endl;

  if(!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    exit(-1);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, WINDOW_TITLE, NULL, NULL);
  if(!window)
  {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    exit(-1);
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(VSYNC);  // Enable vsync

  // Load OpenGL functions using GLAD
  if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    glfwTerminate();
    exit(-1);
  }

  glfwSetMouseButtonCallback(window, MouseButton);   // Capture Mouse Input
  glfwSetCursorPosCallback(window, CursorPosition);  // Capture Mouse Movement
  glfwSetKeyCallback(window, Keyboard);              // Capture Keyboard Input

  // Initialize shaders after OpenGL context is created
  InitShaders();

  // Bind attribute locations (must match shader)
  glBindAttribLocation(gfx.shaderProgram, 0, "aVertex");
  glBindAttribLocation(gfx.shaderProgram, 1, "aColor");

  glEnable(GL_DEPTH_TEST);
}


// ==================================================
// Initialize Shaders:
// ==================================================
void InitShaders()
{
  std::string vertexSource   = GLUtils::ReadShaderFile("vertex.vert");
  std::string fragmentSource = GLUtils::ReadShaderFile("fragment.frag");

  if(vertexSource.empty() || fragmentSource.empty())
  {
    std::cerr << "Failed to load shader files!" << std::endl;
    return;
  }

  GLuint vertexShader   = GLUtils::CompileShader(GL_VERTEX_SHADER, vertexSource.c_str());
  GLuint fragmentShader = GLUtils::CompileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str());

  gfx.shaderProgram = glCreateProgram();
  glAttachShader(gfx.shaderProgram, vertexShader);
  glAttachShader(gfx.shaderProgram, fragmentShader);
  glLinkProgram(gfx.shaderProgram);

  GLint success;
  glGetProgramiv(gfx.shaderProgram, GL_LINK_STATUS, &success);
  if(!success)
  {
    char infoLog[512];
    glGetProgramInfoLog(gfx.shaderProgram, 512, nullptr, infoLog);
    std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Get uniform locations
  gfx.uModelMatrix = glGetUniformLocation(gfx.shaderProgram, "uModelMatrix");
  gfx.uViewMatrix  = glGetUniformLocation(gfx.shaderProgram, "uViewMatrix");
  gfx.uProjMatrix  = glGetUniformLocation(gfx.shaderProgram, "uProjMatrix");
}


// ==================================================
// UI for the Settings:
// ==================================================
void RenderUI()
{
  if(!app.showUI)
    return;

  ImGui::SetNextWindowPos(ImVec2(app.mouseX, app.mouseY), ImGuiCond_Appearing);
  ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

  ImGui::Text("Settings");
  ImGui::Separator();

  ImGui::Checkbox("Axes", &app.axesOn);
  ImGui::Checkbox("Perspective", &app.perspective);
  ImGui::Checkbox("Show Performance", &app.performance);

  if(ImGui::CollapsingHeader("Object Transformation"))
  {
    ImGui::BeginGroup();
    ImGui::Text("Pan and Scale (Shift)");
    GLUtils::PanControl("##pan", model.position.data(), model.scale, 150.0f);
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Rotation");
    ImGui::gizmo3D("##gizmo", model.rotation, 150.0f);
    ImGui::EndGroup();

    ImGui::Text("Scale: %.2f", model.scale);
  }

  ImGui::Separator();

  if(ImGui::Button("Go"))
    Buttons(GO);
  ImGui::SameLine();
  if(ImGui::Button(app.paused ? "Resume" : "Pause"))
    Buttons(PAUSE);
  ImGui::SameLine();
  if(ImGui::Button("Reset"))
    Buttons(RESET);
  ImGui::SameLine();
  if(ImGui::Button("Quit"))
    Buttons(QUIT);

  ImGui::End();
}

void PerformanceOverlay()
{
  if(!app.performance)
    return;

  const float DISTANCE     = 10.0f;
  ImGuiIO&    io           = ImGui::GetIO();
  ImVec2      window_pos   = ImVec2(io.DisplaySize.x - DISTANCE, DISTANCE);
  ImVec2      window_pivot = ImVec2(1.0f, 0.0f);

  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pivot);
  ImGui::SetNextWindowBgAlpha(0.35f);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
                                  | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  if(ImGui::Begin("Performance", nullptr, window_flags))
  {
    float gigaParticlesPerSec = (perf.elapsedTime > 0.0f) ? (float)NUM_PARTICLES / perf.elapsedTime / 1000000000.0f : 0.0f;

    if(perf.elapsedTime > 0.0f && perf.frameCount >= 0 && !app.paused)
    {
      // Track peak
      if(gigaParticlesPerSec > perf.peakGigaParticlesPerSec)
        perf.peakGigaParticlesPerSec = gigaParticlesPerSec;

      // Track average after warmup
      if(perf.frameCount >= WARMUP_FRAMES)
        perf.totalGigaParticlesPerSec += gigaParticlesPerSec;

      perf.frameCount++;
    }

    float avgGigaParticlesPerSec =
        (perf.frameCount > WARMUP_FRAMES) ? perf.totalGigaParticlesPerSec / (perf.frameCount - WARMUP_FRAMES) : 0.0f;

    ImGui::Text("Current: %.3f GigaParticles/Sec", gigaParticlesPerSec);
    ImGui::Text("Peak:    %.3f GigaParticles/Sec", perf.peakGigaParticlesPerSec);
    ImGui::Text("Average: %.3f GigaParticles/Sec", avgGigaParticlesPerSec);
  }
  ImGui::End();
}


// ==================================================
// Reset all Transformation Values:
// ==================================================
void Reset()
{
  app.axesOn      = false;
  app.paused      = true;
  app.perspective = true;

  model  = {};  // Reset to default values
  camera = {};  // Reset to default values
  perf   = {};  // Reset to default values

  ResetParticles();
}

void ResetParticles()
{
  glBindBuffer(GL_ARRAY_BUFFER, particle.posGL);
  xyzw* points = (xyzw*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  for(int i = 0; i < NUM_PARTICLES; i++)
  {
    points[i].x = Ranf(r.XMIN, r.XMAX);
    points[i].y = Ranf(r.YMIN, r.YMAX);
    points[i].z = Ranf(r.ZMIN, r.ZMAX);
    points[i].w = 1.0;
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);


  glBindBuffer(GL_ARRAY_BUFFER, particle.colorGL);
  rgba* colors = (rgba*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  for(int i = 0; i < NUM_PARTICLES; i++)
  {
    colors[i].r = Ranf(0.3f, 1.0);
    colors[i].g = Ranf(0.3f, 1.0);
    colors[i].b = Ranf(0.3f, 1.0);
    colors[i].a = 1.0;
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);


  for(int i = 0; i < NUM_PARTICLES; i++)
  {
    particle.velHost[i].x = Ranf(r.VMIN, r.VMAX);
    particle.velHost[i].y = Ranf(0.0f, r.VMAX);
    particle.velHost[i].z = Ranf(r.VMIN, r.VMAX);
  }

  // Reset on OpenCL side
  cl_int status = clEnqueueWriteBuffer(cl.cmdQueue, particle.velCL, CL_TRUE,    //
                                       0, 4 * sizeof(float) * NUM_PARTICLES,    //
                                       particle.velHost, 0, nullptr, nullptr);  //
  CLUtils::PrintCLError(status, "clEnqueueWriteBuffer (ResetParticles): ");
}

// ==================================================
// Cleanup and Shutdown (gracefully):
// ==================================================
void Quit()
{
  // Clean up VBOs and VAOs
  if(gfx.axesVAO)
    glDeleteVertexArrays(1, &gfx.axesVAO);
  if(gfx.axesVBO)
    glDeleteBuffers(1, &gfx.axesVBO);
  if(gfx.sphereVAO)
    glDeleteVertexArrays(1, &gfx.sphereVAO);
  if(gfx.sphereVBO)
    glDeleteBuffers(1, &gfx.sphereVBO);
  if(particle.particleVAO)
    glDeleteVertexArrays(1, &particle.particleVAO);
  if(gfx.shaderProgram)
    glDeleteProgram(gfx.shaderProgram);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  std::cout << "Application closed." << std::endl;
}


// ==================================================
// Mouse and Keyboard Callbacks:
// ==================================================

// Mouse button callback - handles click and release actions
void MouseButton(GLFWwindow* win, int button, int action, int mods)
{
  // Check if ImGui wants to capture mouse input
  if(ImGuiIO& io = ImGui::GetIO(); io.WantCaptureMouse)
    return;

  // Toggle UI on right-click
  if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
  {
    app.showUI = !app.showUI;
  }
}

// Cursor position callback - handles click and drag actions
void CursorPosition(GLFWwindow* window, double xpos, double ypos)
{
  // Calculate delta
  const auto dx = static_cast<float>(xpos) - app.mouseX;
  const auto dy = static_cast<float>(ypos) - app.mouseY;

  // Update stored position
  app.mouseX = static_cast<float>(xpos);
  app.mouseY = static_cast<float>(ypos);

  if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
    return;

  if(ImGui::GetIO().WantCaptureMouse)
    return;

  // Check for modifier keys
  auto       isKeyPressed = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
  const bool shiftKey     = isKeyPressed(GLFW_KEY_LEFT_SHIFT) || isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
  const bool altKey       = isKeyPressed(GLFW_KEY_LEFT_ALT) || isKeyPressed(GLFW_KEY_RIGHT_ALT);

  if(shiftKey)
  {
    // ZOOM: Shift + Drag
    constexpr float zoomSpeed = 2.0f;
    camera.distance += dy * zoomSpeed;
    camera.distance = glm::clamp(camera.distance, 100.0f, FAR_LOOK / 1.67f);  // Limit zoom range
  }
  else if(altKey)
  {
    // PAN: Alt + Drag
    const float panSpeed = 0.0005f * camera.distance;
    camera.panX -= dx * panSpeed;
    camera.panY += dy * panSpeed;
  }
  else
  {
    // ROTATE: Normal Drag
    constexpr float rotSpeed = 0.005f;
    camera.theta -= dx * rotSpeed;  // Horizontal rotation
    camera.phi -= dy * rotSpeed;    // Vertical rotation

    // Clamp vertical angle to prevent flipping
    camera.phi = glm::clamp(camera.phi, 0.1f, glm::radians(179.9f));
  }
}

// Keyboard callback - handles key press events
void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  // Only respond to key press events (ignore release and repeat)
  if(action != GLFW_PRESS)
    return;

  switch(key)
  {
    case GLFW_KEY_O:
      app.perspective = false;
      break;

    case GLFW_KEY_P:
      app.perspective = true;
      break;

    case GLFW_KEY_A:
      app.axesOn = !app.axesOn;
      break;

    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
      Buttons(QUIT);
      break;

    case GLFW_KEY_R:
      Buttons(RESET);
      break;

    case GLFW_KEY_SPACE:
      Buttons(PAUSE);
      break;

    // Ignore modifier keys (no action needed)
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_RIGHT_ALT:
    case GLFW_KEY_LEFT_SUPER:
    case GLFW_KEY_RIGHT_SUPER:
      break;

    // Log unhandled keys
    default:
      std::cerr << "Unhandled key: ";
      if(key >= 0x20 && key <= 0x7E)
        std::cerr << static_cast<char>(key) << " ";
      std::cerr << "(0x" << std::hex << key << std::dec << ")\n";
      break;
  }
}

// Button action handler
void Buttons(int id)
{
  switch(id)
  {
    case GO:
      app.paused = false;
      break;

    case PAUSE:
      app.paused = !app.paused;
      break;

    case RESET:
      Reset();
      break;

    case QUIT:
      glfwSetWindowShouldClose(window, true);
      break;

    default:
      std::cerr << "Don't know what to do with Button ID " << id << '\n';
      break;
  }
}


float Ranf(float low, float high)
{
  float r = (float)rand() / (float)RAND_MAX;
  return low + r * (high - low);
}

inline xyzw XYZW(float x, float y, float z, float w)
{
  return xyzw{x, y, z, w};
}

inline rgba RGBA(float r, float g, float b, float a)
{
  return rgba{r, g, b, a};
}
