#version 460 core

out vec2 texCoord;

void main(void)
{
    vec2 vPos[] = vec2[](
        vec2(1.0f,1.0f),
        vec2(-1.0f,1.0f),
        vec2(1.0f,-1.0f),
        vec2(-1.0f,-1.0f)
    );

    gl_Position = vec4(vPos[gl_VertexID],0.0f,1.0);
    texCoord = clamp(vPos[gl_VertexID],0.0,1.0);
}

