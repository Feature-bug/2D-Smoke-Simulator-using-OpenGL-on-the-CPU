#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct{
    //Program ID
    unsigned int ID;
}Shader;

//Constructor reads and builds the shader
Shader shader_create(const char* vertexPath, const char* fragmentPath){
    Shader shader;
    char* vertexCode = NULL;
    char* fragmentCode = NULL;
    FILE* vShaderFile;
    FILE* fShaderFile;

    //Open File
    vShaderFile = fopen(vertexPath, "r");
    fShaderFile = fopen(fragmentPath, "r");

    //Check if file opens successfully
    if(vShaderFile == NULL || fShaderFile == NULL){
        fprintf(stderr, "ERROR:SHADER:FILE_NOT_SUCCESSFULLY_READ\n");
        if(vShaderFile){
            fclose(vShaderFile);
        }
        if(fShaderFile){
            fclose(fShaderFile);
        }
        shader.ID = 0;
        return shader;
    }
    //Read vertex shader file
    fseek(vShaderFile, 0, SEEK_END);
    long vSize = ftell(vShaderFile);
    fseek(vShaderFile, 0, SEEK_SET);
    vertexCode = malloc(vSize + 1);
    fread(vertexCode, 1, vSize, vShaderFile);
    vertexCode[vSize] = '\0';
    //Read fragment shader file
    fseek(fShaderFile, 0, SEEK_END);
    long fSize =    ftell(fShaderFile);
    fseek(fShaderFile, 0, SEEK_SET);
    fragmentCode = malloc(fSize + 1);
    fread(fragmentCode, 1, fSize, fShaderFile);
    fragmentCode[fSize] = '\0';

    //Close files
    fclose(vShaderFile);
    fclose(fShaderFile);

    //Compile Shader
    unsigned int vertex, fragment;
    int success;
    char infolog[512];

    //Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    const char* vCode = vertexCode;
    glShaderSource(vertex, 1, &vCode, NULL);
    glCompileShader(vertex);
    //Print Compile erroe if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(vertex, 512, NULL, infolog);
        fprintf(stderr, "ERROR:SHADER:VERTEX:COMPILATION_FAILED\n%s\n", infolog);
    }
    //Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fCode = fragmentCode;
    glShaderSource(fragment, 1, &fCode, NULL);
    glCompileShader(fragment);
    //Print Compile error if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(fragment, 512, NULL, infolog);
        fprintf(stderr, "ERROR:SHADER:FRAGMENT:COMPILATION_FAILED\n%s\n", infolog);
    }

    //Shader program
    shader.ID = glCreateProgram();
    glAttachShader(shader.ID, vertex);
    glAttachShader(shader.ID, fragment);
    glLinkProgram(shader.ID);
    //Delete shader as they are linked into program and no longer needed
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    free(vertexCode);
    free(fragmentCode);

    return shader;
}

//use/activate shader
void shader_use(Shader* shader){
        glUseProgram(shader->ID);
}
//Utility uniform function
void setBool(Shader* shader, const char* name, bool value){
    glUniform1i(glGetUniformLocation(shader->ID, name), (int)value);
}
void setInt(Shader* shader, const char* name, int value){
    glUniform1i(glGetUniformLocation(shader->ID, name), value);
}
void setFloat(Shader* shader, const char* name, float value){
    glUniform1f(glGetUniformLocation(shader->ID, name), value);
}

void delete_shader(Shader* shader){
    glDeleteProgram(shader->ID);
}

#endif
