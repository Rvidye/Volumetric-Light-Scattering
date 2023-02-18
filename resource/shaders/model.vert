#version 460 core

in vec3 a_position;
in vec3 a_normal;
in vec2 a_texcoord;
in vec3 a_tangent;
in vec3 a_bitangent;
in ivec4 a_boneIds;
in vec4 a_weights;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

out vec2 out_texcoord;
out vec3 Normal;
out vec3 Pos;

void main(void)
{
	gl_Position = u_Projection * u_View * u_Model * vec4(a_position,1.0f);
	out_texcoord = a_texcoord;
	Normal = mat3(transpose(inverse(u_Model))) * a_normal;
	Pos = vec3(u_Model * vec4(a_position,1.0f));
}
