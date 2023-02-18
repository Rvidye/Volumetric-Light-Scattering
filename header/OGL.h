#pragma once
#define BAT_ICON 2001

FILE* gpFile = NULL;

// Debug Camera

struct
{
    int lastMouseX;
    int lastMouseY;
    GLfloat cameraYaw;
    GLfloat cameraPitch;
    vec3 cameraFront;
    vec3 cameraPosition;
    vec3 cameraUp;
}DebugCam;

void Log(const char* type, const char* msg)
{
    fopen_s(&gpFile, "Log.txt", "a");
    fprintf(gpFile, "%s :: %s\n", type, msg);
    fclose(gpFile);
}

void checkError(GLuint program, bool isShader)
{
    void uninitialize(void);
    char str[1024];
    int status = 0;
    int infoLogLength = 0;
    char* log = NULL;

    isShader ? glGetShaderiv(program, GL_COMPILE_STATUS, &status) : glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
    {
        isShader ? glGetShaderiv(program, GL_INFO_LOG_LENGTH, &infoLogLength) : glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            log = (char*)malloc(infoLogLength);
            if (log != NULL)
            {
                GLsizei written;
                isShader ? glGetShaderInfoLog(program, infoLogLength, &written, log) : glGetProgramInfoLog(program, infoLogLength, &written, log);
                isShader ? sprintf(str, "Shader Error Log : %s", log) : sprintf(str, "Program Linking Log : %s", log);
                Log("ERROR", str);
                free(log);
                uninitialize();
            }
        }
    }
}

unsigned int loadShader(const char* filePath, GLenum TYPE)
{
    char str[1024];
    sprintf(str, "Loading %s Shader !!!\n", filePath);
    Log("LOG", str);
    unsigned int id = 0;
     // read code from file
    std::string code;
    std::ifstream shaderfile(filePath, std::ios::in);
    GLchar const* src = NULL;

    if (shaderfile.is_open())
    {
        std::stringstream sstr;
        sstr << shaderfile.rdbuf();
        code = sstr.str();
        src = code.c_str();
        shaderfile.close();
    }
    else
    {
        //std::cout<<"Something is wrong!!!\n";
        Log("ERROR", "Something Is Wrong !!!\n");
        return 999;
    }

    // Create Shader Object 
    id = glCreateShader(TYPE);

    // link shader src to shader object
    glShaderSource(id, 1, (const GLchar**)&src, NULL);

    // Compile Shader
    glCompileShader(id);

    // Error checking
    checkError(id, true);

    return id;
}

bool loadTexture(const char* fileName, GLuint* texID)
{
    char str[1024];
    sprintf(str, "Loading %s Texture !!!\n", fileName);
    Log("LOG", str);
    stbi_set_flip_vertically_on_load(false);
    int width, height, nchannels;
    unsigned char* bits = stbi_load(fileName, &width, &height, &nchannels, 0);
    if (bits == NULL)
    {
        Log("ERROR", "stbi_load return null !!!");
        return false;
    }

    GLenum img_format = GL_RGBA, internal_format = GL_RGBA;

    // bmp img

    if (nchannels == STBI_grey)
    {
        img_format = GL_RED;
        internal_format = GL_RED;
    }

    if (nchannels == STBI_rgb)
    {
        fprintf(gpFile, "Internal Format RGB\n");
        img_format = GL_RGB;
        internal_format = GL_RGB;
    }

    // png img
    if (nchannels == STBI_rgb_alpha)
    {
        fprintf(gpFile, "Internal Format RGBA\n");
        img_format = GL_RGBA;
        internal_format = GL_RGBA;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, texID);
    glBindTexture(GL_TEXTURE_2D, *texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, img_format, width, height, 0, internal_format, GL_UNSIGNED_BYTE, bits);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(bits);

    return TRUE;
}
