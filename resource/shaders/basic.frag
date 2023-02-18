#version 460 core 
in vec4 color;
in vec2 tex;

uniform sampler2D diffuse;

out vec4 FragColor;

void main(void) 
{ 
    FragColor = texture(diffuse,tex);
}
