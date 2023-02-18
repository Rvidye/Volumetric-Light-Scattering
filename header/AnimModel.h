#pragma once
class Model {
	public:

	Model(std::string const& path)
	{
		fopen_s(&alog, "AssimpLog.txt", "w");
		loadModel(path);
		fclose(alog);
	}

	~Model() {}

	void ModelCleanup()
	{
		if (meshes.size() > 0)
		{
			for (size_t i = 0; i < meshes.size(); i++)
			{
				meshes[i].MeshCleanup();
			}
		}
	}

	void Draw(unsigned int& program)
	{
		for (unsigned int i = 0; i < meshes.size(); i++)
		{
			meshes[i].Draw(program);
		}
	}

	void printMeshVertex()
	{
		fopen_s(&alog, "AssimpLog.txt", "a");
		fprintf(alog, "Number of Meshes : %zd Textures Loaded : %zd\n",this->meshes.size(),this->textures_loaded.size());

		/*
		for (unsigned int i = 0; i < meshes.size(); i++)
		{
			fprintf(alog, "Mesh : %d, Vertrices : %zd , Indices : %zd\n",i,this->meshes[i].vertices.size(),this->meshes[i].vertices.size());
		}
		*/

		fclose(alog);
		
		for (unsigned int i = 0; i < meshes.size(); i++)
		{
			meshes[i].printVertData();
		}
	}

	std::map<std::string, BoneInfo>& GetBoneInfoMap() { return m_BoneInfoMap; }

	int& GetBoneCount() { return m_BoneCounter; }

private:

	std::vector<NTexture> textures_loaded;
	std::vector<Mesh> meshes;
	std::string directory;

	std::map<std::string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;

	FILE* alog;

	void loadModel(std::string const& path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			fprintf(alog,"Error Loading Model\n");
			fclose(alog);
			return;
		}

		this->directory = path.substr(0, path.find_last_of("\\"));
		fprintf(alog, "Directory :: %s\n",this->directory.c_str());
		processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}

		/*
		for (unsigned int i = 0; i < scene->mNumMaterials; i++)
		{
			aiMaterial* mat = scene->mMaterials[i];
			fprintf(alog, "Material %d :: %s\n", i,mat->GetName().C_Str());
		}
		*/
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<NTexture> textures;
		std::vector<NMaterial> mats;

		fprintf(alog,"Mesh Data : %s, Num Vertices : %d, indices %d Bones : %d.\n",mesh->mName.C_Str(),mesh->mNumVertices,(mesh->mNumFaces*3),mesh->mNumBones);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			vec3 vec;

			vec[0] = mesh->mVertices[i].x;
			vec[1] = mesh->mVertices[i].y;
			vec[2] = mesh->mVertices[i].z;

			vertex.Position = vec;

			if (mesh->HasNormals())
			{
				vec[0] = mesh->mNormals[i].x;
				vec[1] = mesh->mNormals[i].y;
				vec[2] = mesh->mNormals[i].z;
				vertex.Normal = vec;
			}

			if (mesh->mTextureCoords[0])
			{
				vec2 uv;

				uv[0] = mesh->mTextureCoords[0][i].x;
				uv[1] = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = uv;

				// Tangent
				vec[0] = mesh->mTangents[i].x;
				vec[1] = mesh->mTangents[i].y;
				vec[2] = mesh->mTangents[i].z;
				vertex.Tangent = vec;

				vec[0] = mesh->mBitangents[i].x;
				vec[1] = mesh->mBitangents[i].y;
				vec[2] = mesh->mBitangents[i].z;
				vertex.Bitangent = vec;
			}
			else
				vertex.TexCoords = vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			//diffuse map
			std::vector<NTexture> diffuseMap = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMap.begin(), diffuseMap.end());

			//specular map
			std::vector<NTexture> specularMap = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMap.begin(), specularMap.end());

			//normal map
			std::vector<NTexture> normalMap = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
			textures.insert(textures.end(), normalMap.begin(), normalMap.end());

			// emissive
			std::vector<NTexture> emissiveMap = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive");
			textures.insert(textures.end(), emissiveMap.begin(), emissiveMap.end());

			std::vector<NMaterial> MaterialAmbient = loadMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, "material_ambient_animModel");
			mats.insert(mats.end(),MaterialAmbient.begin(), MaterialAmbient.end());

			std::vector<NMaterial> MaterialDiffuse = loadMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, "material_diffuse_animModel");
			mats.insert(mats.end(),MaterialDiffuse.begin(), MaterialDiffuse.end());

			std::vector<NMaterial> MaterialSpecular = loadMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, "material_specular_animModel");
			mats.insert(mats.end(),MaterialSpecular.begin(), MaterialSpecular.end());
		}


		// Parse Bone Data
		if (mesh->HasBones())
		{
			for (int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
			{
				int boneID = -1;
				std::string bone_name(mesh->mBones[boneIndex]->mName.data);

				if (m_BoneInfoMap.find(bone_name) == m_BoneInfoMap.end())
				{
					BoneInfo newBoneInfo;
					newBoneInfo.id = m_BoneCounter;
					newBoneInfo.offset = ConvertMatrixToVmathFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
					m_BoneInfoMap[bone_name] = newBoneInfo;
					boneID = m_BoneCounter;
					m_BoneCounter++;
				}
				else
				{
					boneID = m_BoneInfoMap[bone_name].id;
				}

				for (int weightIndex = 0; weightIndex < mesh->mBones[boneIndex]->mNumWeights; weightIndex++)
				{
					int vertexId = mesh->mBones[boneIndex]->mWeights[weightIndex].mVertexId;
					float weight = mesh->mBones[boneIndex]->mWeights[weightIndex].mWeight;
					vertices[vertexId].SetVertexBoneData(boneID, weight);
				}
			}
		}
		return Mesh(vertices, indices, textures,mats);
	}

	std::vector<NTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typestring)
	{
		std::vector<NTexture> textures;

		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);

			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}

			if (!skip)
			{
				NTexture texture;
				texture.id = TextureFromFile(str.C_Str(), this->directory);
				texture.type = typestring;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture);
			}
		}
		return textures;
	}

	std::vector<NMaterial> loadMaterialColor(aiMaterial* mat, const char* type, int one, int two, std::string typeName)
	{
		std::vector<NMaterial> materials;

		aiColor3D color;

		mat->Get(type, one, two, color);

		NMaterial matInfo;
		matInfo.type = typeName;
		matInfo.value[0] = color[0];
		matInfo.value[1] = color[1];
		matInfo.value[2] = color[2];

		fprintf(alog,"Material %s : value = %.1f, %.1f, %.1f\n",matInfo.type.c_str(), matInfo.value[0], matInfo.value[1], matInfo.value[2]);

		materials.push_back(matInfo);
		return materials;
	}

	unsigned int TextureFromFile(const char* path, const std::string& directory)
	{
		stbi_set_flip_vertically_on_load(true);
		std::string filename = std::string(path);
		filename = directory + '\\' + filename;
		fprintf(alog, "Texture path :: %s\n", filename.c_str());
		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			GLenum format;
			if (nrComponents == 1)
				format = GL_RED;
			else if (nrComponents == 3)
				format = GL_RGB;
			else if (nrComponents == 4)
				format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		stbi_image_free(data);
		return textureID;
	}
};
