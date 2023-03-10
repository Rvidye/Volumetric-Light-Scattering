#pragma once

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoords;
	vec3 Tangent;
	vec3 Bitangent;
	ivec4 m_BoneID;
	vec4 m_Weight;

	Vertex()
	{
		Position = vec3(0.0f, 0.0f, 0.0f);
		Normal = vec3(0.0f, 0.0f, 0.0f);
		TexCoords = vec2(0.0f, 0.0f);
		Tangent = vec3(0.0f, 0.0f, 0.0f);
		Bitangent = vec3(0.0f, 0.0f, 0.0f);

		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			m_BoneID[i] = -1;
			m_Weight[i] = 0.0f;
		}
	}

	void SetVertexBoneData(int boneID, float weight)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			if (this->m_BoneID[i] < 0)
			{
				this->m_BoneID[i] = boneID;
				this->m_Weight[i] = weight;
				break;
			}
		}
	}
};

struct NTexture
{
	unsigned int id;
	std::string type;
	std::string path;
};

struct NMaterial
{
	std::string material_name;
	vec3 value;
	std::string type;
};

struct BoneInfo
{
	int id;
	mat4 offset;
};

class Mesh {

public:

	Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<NTexture>& textures, std::vector<NMaterial>& mats)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
		this->materials = mats;
		setupMesh();
	}

	~Mesh()
	{
	}

	void MeshCleanup()
	{
		if (this->ebo)
		{
			glDeleteBuffers(1, &this->ebo);
			this->ebo = NULL;
		}

		if (this->vbo)
		{
			glDeleteBuffers(1, &this->vbo);
			this->vbo;
		}

		if (this->vao)
		{
			glDeleteVertexArrays(1,&this->vao);
			this->vao = NULL;
		}

		if (this->vertices.size() > 0)
		{
			this->vertices.clear();
			this->vertices.shrink_to_fit();
		}

		if (this->indices.size() > 0)
		{
			this->indices.clear();
			this->indices.shrink_to_fit();
		}

		if (this->textures.size() > 0)
		{
			for (size_t i = 0; i < this->textures.size(); i++)
			{
				glDeleteTextures(1, &this->textures[i].id);
				this->textures[i].id = NULL;
			}

			this->textures.clear();
			this->textures.shrink_to_fit();
		}

		if (this->materials.size() > 0)
		{
			this->materials.clear();
			this->materials.shrink_to_fit();
		}
	}

	void Draw(GLuint program)
	{
		// Bind appropriate Textures
		unsigned int diffuseNr = 1;
		unsigned int specularNr = 1;
		unsigned int normalNr = 1;
		unsigned int heightNr = 1;

		for (unsigned int i = 0; i < this->textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);

			std::string name = this->textures[i].type;
			std::string number;

			if (name == "texture_diffuse")
				number = std::to_string(diffuseNr++);
			else if (name == "texture_specular")
				number = std::to_string(specularNr++);
			else if (name == "texture_normal")
				number = std::to_string(normalNr++);
			else if (name == "texture_height")
				number = std::to_string(heightNr++);

			glUniform1i(glGetUniformLocation(program, (name + number).c_str()), i);
			glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
		}

		unsigned int diffuse_mr = 1;

		for (size_t i = 0; i < this->materials.size(); i++)
		{
			std::string type = materials[i].type;
			if ((type == "material_ambient_animModel") || (type == "material_diffuse_animModel") || (type == "material_specular_animModel"))
			{
				glUniform3fv(glGetUniformLocation(program, type.c_str()), 1, materials[i].value);
			}
		}

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(this->indices.size()), GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void DrawInstanced(GLuint program, GLuint numOfInstance)
	{

	}

	void printVertData()
	{
		FILE* opFile;
		fopen_s(&opFile, "VData.txt", "a");
		fprintf(opFile, "Vertices Data Size = %zd\n\n",vertices.size());
		for (Vertex v : this->vertices)
		{
			fprintf(opFile, "Position Data : %.1f %.1f %.1f\n", v.Position[0], v.Position[1], v.Position[2]);
			fprintf(opFile, "Normal Data : %.1f %.1f %.1f\n", v.Normal[0], v.Normal[1], v.Normal[2]);
			fprintf(opFile, "TexCoordData : %.1f %.1f\n", v.TexCoords[0], v.TexCoords[1]);
			fprintf(opFile, "Tangent Data : %.1f %.1f %.1f\n", v.Tangent[0], v.Tangent[1], v.Tangent[2]);
			fprintf(opFile, "Bitangent Data : %.1f %.1f %.1f\n", v.Bitangent[0], v.Bitangent[1], v.Bitangent[2]);
			fprintf(opFile, "Bone ID : %d %d %d %d\n", v.m_BoneID[0], v.m_BoneID[1], v.m_BoneID[2], v.m_BoneID[3]);
			fprintf(opFile, "Weights Data : %.1f %.1f %.1f %.1f\n", v.m_Weight[0], v.m_Weight[1], v.m_Weight[2],v.m_Weight[3]);
			fprintf(opFile, "\n");
		}
		fprintf(opFile, "\n");
		fclose(opFile);
	}

	void setupMesh()
	{
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
		glEnableVertexAttribArray(3);

		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
		glEnableVertexAttribArray(4);

		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneID));
		glEnableVertexAttribArray(5);

		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weight));
		glEnableVertexAttribArray(6);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

private:
	GLuint vao, vbo, ebo;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<NTexture> textures;
	std::vector<NMaterial> materials;
};
