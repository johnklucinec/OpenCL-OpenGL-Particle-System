#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include "imgui.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// ==================================================
// Data Types
// ==================================================

using SphereDataType = std::vector<float>;

// Aliases for vectors/colors
struct xyzw
{
  float x, y, z, w;
};

struct rgba
{
  float r, g, b, a;
};

struct SphereConfig
{
  xyzw  position;  // x,y,z
  float radius;    // r
  rgba  color;     // r,g,b
};


// ==================================================
// Function Prototypes:
// ==================================================

namespace GLUtils {

void        InitAxes(GLuint&, GLuint&, int&, float, const glm::vec3&);
void        DrawWireSphere(SphereDataType&, const SphereConfig&);
void        FinalizeSpheres(SphereDataType&, GLuint&, GLuint&, int&);
bool        PanControl(const char*, float xyz[3], float& scale, float size = 200.0f);
std::string ReadShaderFile(const char*);
GLuint      CompileShader(GLenum, const char*);
void        UpdateFPS(GLFWwindow*, const char*);

}  // namespace GLUtils


namespace GLUtils {
// ==================================================
// Initialize Axes Geometry (VBO/VAO):
// ==================================================
inline void InitAxes(GLuint& axesVAO, GLuint& axesVBO, int& axesVertexCount, float axesLength, const glm::vec3& axesColor)
{
  // Axes vertices: position (x, y, z) and color (r, g, b)
  // Drawing X, Y, Z axes as line strips
  std::vector<float> axesData;

  auto addVertex = [&](float x, float y, float z, float r, float g, float b) {
    axesData.push_back(x);
    axesData.push_back(y);
    axesData.push_back(z);
    axesData.push_back(r);
    axesData.push_back(g);
    axesData.push_back(b);
  };

  float length = axesLength;
  float r = axesColor.r, g = axesColor.g, b = axesColor.b;

  // Main axes lines (using GL_LINES, so pairs of vertices)
  // X axis
  addVertex(0.0f, 0.0f, 0.0f, r, g, b);
  addVertex(length, 0.0f, 0.0f, r, g, b);
  // Y axis
  addVertex(0.0f, 0.0f, 0.0f, r, g, b);
  addVertex(0.0f, length, 0.0f, r, g, b);
  // Z axis
  addVertex(0.0f, 0.0f, 0.0f, r, g, b);
  addVertex(0.0f, 0.0f, length, r, g, b);

  // Letter labels
  float fact = 0.10f * length;  // LENFRAC
  float base = 1.10f * length;  // BASEFRAC

  // X label
  static float xx[] = {0.f, 1.f, 0.f, 1.f};
  static float xy[] = {-.5f, .5f, .5f, -.5f};
  addVertex(base + fact * xx[0], fact * xy[0], 0.0f, r, g, b);
  addVertex(base + fact * xx[1], fact * xy[1], 0.0f, r, g, b);
  addVertex(base + fact * xx[2], fact * xy[2], 0.0f, r, g, b);
  addVertex(base + fact * xx[3], fact * xy[3], 0.0f, r, g, b);

  // Y label
  static float yx[] = {0.f, 0.f, -.5f, .5f};
  static float yy[] = {0.f, .6f, 1.f, 1.f};
  addVertex(fact * yx[0], base + fact * yy[0], 0.0f, r, g, b);
  addVertex(fact * yx[1], base + fact * yy[1], 0.0f, r, g, b);
  addVertex(fact * yx[1], base + fact * yy[1], 0.0f, r, g, b);
  addVertex(fact * yx[2], base + fact * yy[2], 0.0f, r, g, b);
  addVertex(fact * yx[1], base + fact * yy[1], 0.0f, r, g, b);
  addVertex(fact * yx[3], base + fact * yy[3], 0.0f, r, g, b);

  // Z label
  static float zx[] = {1.f, 0.f, 1.f, 0.f, .25f, .75f};
  static float zy[] = {.5f, .5f, -.5f, -.5f, 0.f, 0.f};
  addVertex(0.0f, fact * zy[0], base + fact * zx[0], r, g, b);
  addVertex(0.0f, fact * zy[1], base + fact * zx[1], r, g, b);
  addVertex(0.0f, fact * zy[1], base + fact * zx[1], r, g, b);
  addVertex(0.0f, fact * zy[2], base + fact * zx[2], r, g, b);
  addVertex(0.0f, fact * zy[2], base + fact * zx[2], r, g, b);
  addVertex(0.0f, fact * zy[3], base + fact * zx[3], r, g, b);

  axesVertexCount = static_cast<int>(axesData.size() / 6);

  glGenVertexArrays(1, &axesVAO);
  glGenBuffers(1, &axesVBO);

  glBindVertexArray(axesVAO);
  glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
  glBufferData(GL_ARRAY_BUFFER, axesData.size() * sizeof(float), axesData.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

// ==================================================
// Draw Wireframe Sphere (Modern OpenGL replacement for glutWireSphere):
// Call in InitLists() for each sphere. Generates VBO-compatible geometry.
// ==================================================
inline void DrawWireSphere(SphereDataType& sphereData, const SphereConfig& config)
{
  const int slices = 100;
  const int stacks = 100;

  float cx     = config.position.x;
  float cy     = config.position.y;
  float cz     = config.position.z;
  float radius = config.radius;
  float r      = config.color.r;
  float g      = config.color.g;
  float b      = config.color.b;

  // Latitude lines (horizontal circles)
  for(int i = 0; i <= stacks; i++)
  {
    float phi      = static_cast<float>(M_PI) * (-0.5f + static_cast<float>(i) / stacks);
    float y        = radius * std::sin(phi);
    float r_circle = radius * std::cos(phi);

    for(int j = 0; j < slices; j++)
    {
      float theta1 = 2.0f * static_cast<float>(M_PI) * static_cast<float>(j) / slices;
      float theta2 = 2.0f * static_cast<float>(M_PI) * static_cast<float>(j + 1) / slices;

      float x1 = r_circle * std::cos(theta1);
      float z1 = r_circle * std::sin(theta1);
      float x2 = r_circle * std::cos(theta2);
      float z2 = r_circle * std::sin(theta2);

      sphereData.insert(sphereData.end(), {cx + x1, cy + y, cz + z1, r, g, b});
      sphereData.insert(sphereData.end(), {cx + x2, cy + y, cz + z2, r, g, b});
    }
  }

  // Longitude lines (vertical circles)
  for(int j = 0; j < slices; j++)
  {
    float theta     = 2.0f * static_cast<float>(M_PI) * static_cast<float>(j) / slices;
    float cos_theta = std::cos(theta);
    float sin_theta = std::sin(theta);

    for(int i = 0; i < stacks; i++)
    {
      float phi1 = static_cast<float>(M_PI) * (-0.5f + static_cast<float>(i) / stacks);
      float phi2 = static_cast<float>(M_PI) * (-0.5f + static_cast<float>(i + 1) / stacks);

      float x1 = radius * std::cos(phi1) * cos_theta;
      float y1 = radius * std::sin(phi1);
      float z1 = radius * std::cos(phi1) * sin_theta;

      float x2 = radius * std::cos(phi2) * cos_theta;
      float y2 = radius * std::sin(phi2);
      float z2 = radius * std::cos(phi2) * sin_theta;

      sphereData.insert(sphereData.end(), {cx + x1, cy + y1, cz + z1, r, g, b});
      sphereData.insert(sphereData.end(), {cx + x2, cy + y2, cz + z2, r, g, b});
    }
  }
}

// ==================================================
// Finalize Spheres (Upload geometry to GPU):
// Call after all DrawWireSphere() calls.
// ==================================================
inline void FinalizeSpheres(SphereDataType& sphereData, GLuint& sphereVAO, GLuint& sphereVBO, int& sphereVertexCount)
{
  sphereVertexCount = static_cast<int>(sphereData.size() / 6);

  glGenVertexArrays(1, &sphereVAO);
  glGenBuffers(1, &sphereVBO);

  glBindVertexArray(sphereVAO);
  glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
  glBufferData(GL_ARRAY_BUFFER, sphereData.size() * sizeof(float), sphereData.data(), GL_STATIC_DRAW);

  // Position attribute (aVertex)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Color attribute (aColor)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);

  // Clear the temporary data
  sphereData.clear();
  sphereData.shrink_to_fit();
}


// ==================================================
// ImGui Pan/Scale Control Widget:
// ==================================================
inline bool PanControl(const char* label, float xyz[3], float& scale, float size)
{
  ImGui::InvisibleButton(label, ImVec2(size, size));

  ImVec2 p_min  = ImGui::GetItemRectMin();
  ImVec2 p_max  = ImGui::GetItemRectMax();
  ImVec2 center = ImVec2((p_min.x + p_max.x) * 0.5f, (p_min.y + p_max.y) * 0.5f);

  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  bool isHovered   = ImGui::IsItemHovered();
  bool isShiftHeld = ImGui::GetIO().KeyShift;

  // Background color - lighter on hover
  ImU32 bgColor = isHovered ? IM_COL32(66, 150, 249, 105) : IM_COL32(66, 150, 249, 63);
  draw_list->AddRectFilled(p_min, p_max, bgColor);

  // Arrow rendering
  float arrowSize  = size * 0.25f;
  ImU32 arrowColor = IM_COL32(200, 200, 200, 200);

  if(isShiftHeld)
  {
    // Only show up/down arrows for scaling
    draw_list->AddTriangleFilled(ImVec2(center.x, center.y - arrowSize), ImVec2(center.x - 8, center.y - arrowSize + 12),
                                 ImVec2(center.x + 8, center.y - arrowSize + 12), arrowColor);

    draw_list->AddTriangleFilled(ImVec2(center.x, center.y + arrowSize), ImVec2(center.x - 8, center.y + arrowSize - 12),
                                 ImVec2(center.x + 8, center.y + arrowSize - 12), arrowColor);
  }
  else
  {
    // Show all four arrows for panning
    draw_list->AddTriangleFilled(ImVec2(center.x, center.y - arrowSize), ImVec2(center.x - 8, center.y - arrowSize + 12),
                                 ImVec2(center.x + 8, center.y - arrowSize + 12), arrowColor);

    draw_list->AddTriangleFilled(ImVec2(center.x, center.y + arrowSize), ImVec2(center.x - 8, center.y + arrowSize - 12),
                                 ImVec2(center.x + 8, center.y + arrowSize - 12), arrowColor);

    draw_list->AddTriangleFilled(ImVec2(center.x - arrowSize, center.y), ImVec2(center.x - arrowSize + 12, center.y - 8),
                                 ImVec2(center.x - arrowSize + 12, center.y + 8), arrowColor);

    draw_list->AddTriangleFilled(ImVec2(center.x + arrowSize, center.y), ImVec2(center.x + arrowSize - 12, center.y - 8),
                                 ImVec2(center.x + arrowSize - 12, center.y + 8), arrowColor);
  }

  bool changed = false;
  if(ImGui::IsItemActive())
  {
    ImVec2 delta = ImGui::GetIO().MouseDelta;

    if(isShiftHeld)
    {
      // Scale mode - only vertical movement, scales indefinitely
      float scaleFactor = 1.0f + (delta.y * -0.01f);  // Negative so up = larger
      scale *= scaleFactor;
      scale   = std::max(0.01f, scale);  // Prevent negative/zero scale
      changed = true;
    }
    else
    {
      // Pan mode - horizontal and vertical movement
      xyz[0] += delta.x;
      xyz[1] -= delta.y;
      changed = true;
    }
  }

  return changed;
}


// ==================================================
// Read Shader Source From File:
// Searches root and src/ directories for the shader file.
// ==================================================
inline std::string ReadShaderFile(const char* filePath)
{
  constexpr std::array searchPaths = {"", "src/"};  // Search root and src/ for shader files

  for(const auto& basePath : searchPaths)
  {
    std::filesystem::path fullPath = std::string(basePath) + filePath;

    if(std::filesystem::exists(fullPath))
    {
      std::ifstream     file(fullPath);
      std::stringstream buffer;
      buffer << file.rdbuf();
      return buffer.str();
    }
  }

  std::cerr << "Failed to open shader file: " << filePath << std::endl;
  return "";
}


// ==================================================
// Compile Shader and Return ID:
// Logs compilation errors to stderr.
// ==================================================
inline GLuint CompileShader(GLenum type, const char* source)
{
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
  }
  return shader;
}


// ==================================================
// Update FPS Counter in Window Title:
// ==================================================
inline void UpdateFPS(GLFWwindow* window, const char* windowTitle)
{
  static double lastTime   = glfwGetTime();
  static int    frameCount = 0;

  double currentTime = glfwGetTime();
  frameCount++;
  if(currentTime - lastTime >= 1.0)
  {
    std::string title = std::string(windowTitle) + " - FPS: " + std::to_string(frameCount);
    glfwSetWindowTitle(window, title.c_str());
    frameCount = 0;
    lastTime += 1.0;
  }
}

}  // namespace GLUtils
