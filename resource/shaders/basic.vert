#version 460 core 

layout(location = 0)in vec4 vPos;
layout(location = 1)in vec4 vCol;
layout(location = 2)in vec2 vTex;
layout(location = 3)in vec2 vNormal;

uniform mat4 pMat;
uniform mat4 vMat;
uniform mat4 mMat;

out vec4 color;
out vec2 tex;

void main(void) 
{ 
    gl_Position = pMat * vMat * mMat * vPos; 
    color = vCol;
    tex = vTex;
}