#include "utils.h"

std::string Utils::readShaderFromFile(const char *filePath) {
  std::string content;
  std::ifstream filestream(filePath, std::ios::in);
  std::string line = "";

  while (std::getline(filestream, line)) {
    content.append(line + "\n");
  }
  filestream.close();
  return content;
}
GLuint Utils::prepareShader(int shaderType, const char *shaderPath) {
  GLint shaderCompiled;
  std::string shaderStr = readShaderFromFile(shaderPath);
  const char *shaderSrc = shaderStr.c_str();
  GLuint shaderRef = glCreateShader(shaderType);
  glShaderSource(shaderRef, 1, &shaderSrc, NULL);
  glCompileShader(shaderRef);
  glGetShaderiv(shaderRef, GL_COMPILE_STATUS, &shaderCompiled);

  if (shaderCompiled != 1) {
    if (shaderType == 35633)
      std::cout << "Vertex ";
    if (shaderType == 36488)
      std::cout << "Tess Control ";
    if (shaderType == 36487)
      std::cout << "Tess Eval ";
    if (shaderType == 36313)
      std::cout << "Geometry ";
    if (shaderType == 35632)
      std::cout << "Fragment ";
    std::cout << "shader compilation error." << std::endl;
  }
  return shaderRef;
}
GLuint Utils::finalizeShaderProgram(GLuint sprogram) {
  GLint linked;
  glLinkProgram(sprogram);
  glGetProgramiv(sprogram, GL_LINK_STATUS, &linked);

  if (linked != 1) {
    std::cout << "Linking failed" << std::endl;
  }
  return sprogram;
}
GLuint Utils::createShaderProgram(const char *vp, const char *fp) {
  GLuint vShader = prepareShader(GL_VERTEX_SHADER, vp);
  GLuint fShader = prepareShader(GL_FRAGMENT_SHADER, fp);

  GLuint vfProgram = glCreateProgram();
  glAttachShader(vfProgram, vShader);
  glAttachShader(vfProgram, fShader);
  finalizeShaderProgram(vfProgram);
  return vfProgram;
}
