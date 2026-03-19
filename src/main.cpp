#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>

// Drawing a shader
#define numVAOs 1 // Vertex array objects
GLuint renderingProgram;
GLuint vao[numVAOs];

// Read shader from glsl
std::string readShaderSource(const char *filePath) {
  std::string content;
  std::ifstream fileStream(filePath, std::ios::in);
  std::string line = "";

  while (std::getline(fileStream, line)) {
    content.append(line + "\n");
  }
  fileStream.close();
  return content;
}

// Error handling
void printShaderLog(GLuint shader) {
  int len = 0;
  int chWritten = 0;
  char *log;

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

  if (len > 0) {
    log = (char *)malloc(len);
    glGetShaderInfoLog(shader, len, &chWritten, log);
    std::cout << "Shader info log: " << log << std::endl;
    free(log);
  }
}
bool checkOpenGLError() {
  bool foundError = false;
  int glErr = glGetError();

  if (glErr != GL_NO_ERROR)
    foundError = true;
  while (glErr != GL_NO_ERROR) {
    std::cout << "glError: " << glErr << std::endl;
    glErr = glGetError();
  }
  return foundError;
}
void printProgramLog(int prog) {
  int len = 0;
  int chWrittn = 0;
  char *log;

  glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
  if (len > 0) {
    log = (char *)malloc(len);
    glGetProgramInfoLog(prog, len, &chWrittn, log);
    std::cout << "Program info log: " << log << std::endl;
    free(log);
  }
}

// Shader program
GLuint createShaderProgram() {
  GLint vertCompiled;
  GLint fragCompiled;
  GLint linked;

  std::string vertShaderStr = readShaderSource("src/vertShader.glsl");
  std::string fragShaderStr = readShaderSource("src/fragShader.glsl");
  const char *vertShaderSrc = vertShaderStr.c_str();
  const char *fragShaderSrc = fragShaderStr.c_str();

  GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
  GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(vShader, 1, &vertShaderSrc, NULL);
  glShaderSource(fShader, 1, &fragShaderSrc, NULL);
  // Catch errors while compiling shaders
  // Vertex
  glCompileShader(vShader);
  checkOpenGLError();
  glGetShaderiv(vShader, GL_COMPILE_STATUS, &vertCompiled);
  if (vertCompiled != 1) {
    std::cout << "Vertex compilation failed" << std::endl;
    printShaderLog(vShader);
  }
  // Fragment
  glCompileShader(fShader);
  checkOpenGLError();
  glGetShaderiv(fShader, GL_COMPILE_STATUS, &fragCompiled);
  if (fragCompiled != 1) {
    std::cout << "Fragment compilation failed" << std::endl;
    printShaderLog(fShader);
  }

  GLuint vfProgram = glCreateProgram();
  // Catch errors while linking shaders
  glAttachShader(vfProgram, vShader);
  glAttachShader(vfProgram, fShader);

  glLinkProgram(vfProgram);
  checkOpenGLError();
  glGetProgramiv(vfProgram, GL_LINK_STATUS, &linked);
  if (linked != 1) {
    std::cout << "Linking failed: " << std::endl;
    printProgramLog(vfProgram);
  }

  return vfProgram;
}

bool windowOk(GLFWwindow *window) {
  if (!window) {
    std::cerr << "Failed to create window\n";
    glfwTerminate();
    return false;
  }
  return true;
}
void init(GLFWwindow *window) {
  renderingProgram = createShaderProgram();
  glGenVertexArrays(numVAOs, vao);
  glBindVertexArray(vao[0]);
}
void clear(GLFWwindow *window) {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}
void display() {
  glUseProgram(renderingProgram);
  glPointSize(30.0f);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}
void runWindow(GLFWwindow *window) {
  while (!glfwWindowShouldClose(window)) {
    clear(window);
    display();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(800, 600, "Renderer", nullptr, nullptr);
  if (!windowOk(window))
    return -1;

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
    return -1;
  }

  init(window);
  runWindow(window);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
