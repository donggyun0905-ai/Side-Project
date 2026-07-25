#include "shader_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>

static std::string ReadFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "[ERROR] Shader file not found: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint LoadShader(const char* vertexPath, const char* fragmentPath) {
    std::string vertCode = ReadFile(vertexPath);
    std::string fragCode = ReadFile(fragmentPath);

    const char* vSrc = vertCode.c_str();
    const char* fSrc = fragCode.c_str();

    // ---------------------------
    // Vertex Shader
    // ---------------------------
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vSrc, NULL);
    glCompileShader(vShader);

    GLint success;
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(vShader, 1024, NULL, log);
        std::cout << "\n[VERTEX SHADER COMPILE ERROR]\n" << log << std::endl;
    }

    // ---------------------------
    // Fragment Shader
    // ---------------------------
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fSrc, NULL);
    glCompileShader(fShader);

    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(fShader, 1024, NULL, log);
        std::cout << "\n[FRAGMENT SHADER COMPILE ERROR]\n" << log << std::endl;
    }

    // ---------------------------
    // Program Link
    // ---------------------------
    GLuint program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, 1024, NULL, log);
        std::cout << "\n[PROGRAM LINK ERROR]\n" << log << std::endl;
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return program;
}
