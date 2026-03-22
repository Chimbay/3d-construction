#ifndef UTILS_H
#define UTILS_H
#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <fstream>

class Utils {
public:
  static std::string readShaderFromFile(const char *);
  static GLuint prepareShader(int, const char *);
  static GLuint finalizeShaderProgram(GLuint);
  static GLuint createShaderProgram(const char *, const char *);
};

#endif
