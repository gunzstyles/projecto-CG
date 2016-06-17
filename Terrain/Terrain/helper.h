#include"model_obj.h"
#include<GL\glew.h>
#include<GL\glut.h>


ModelOBJ            g_model;
float               g_maxAnisotrophy;

GLuint              g_blinnPhongShader;
GLuint              g_normalMappingShader;


typedef std::map<std::string, GLuint> ModelTextures;
ModelTextures       g_modelTextures;



GLuint LoadTexture(const char *pszFilename)
{
	GLuint id = 0;
	Bitmap bitmap;

	if (bitmap.loadPicture(pszFilename))
	{
		// The Bitmap class loads images and orients them top-down.
		// OpenGL expects bitmap images to be oriented bottom-up.
		bitmap.flipVertical();

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (g_maxAnisotrophy > 1.0f)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
				g_maxAnisotrophy);
		}

		gluBuild2DMipmaps(GL_TEXTURE_2D, 4, bitmap.width, bitmap.height,
			GL_BGRA_EXT, GL_UNSIGNED_BYTE, bitmap.getPixels());
	}

	return id;
}

GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path)
{

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}



	GLint Result = GL_FALSE;
	int InfoLogLength;



	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}



void LoadModel(const char *pszFilename)
{
	// Import the OBJ file and normalize to unit length.


	if (!g_model.import(pszFilename))
	{
		printf("erorr in model loading..check file path..!!");
	}

	g_model.normalize();


	//matrial loading <<---------------------------------IMP
	// Load any associated textures.
	// Note the path where the textures are assumed to be located.

	const ModelOBJ::Material *pMaterial = 0;
	GLuint textureId = 0;
	std::string::size_type offset = 0;
	std::string filename;

	for (int i = 0; i < g_model.getNumberOfMaterials(); ++i)
	{
		pMaterial = &g_model.getMaterial(i);

		// Look for and load any diffuse color map textures.

		if (pMaterial->colorMapFilename.empty())
			continue;

		// Try load the texture using the path in the .MTL file.
		textureId = LoadTexture(pMaterial->colorMapFilename.c_str());

		if (!textureId)
		{
			offset = pMaterial->colorMapFilename.find_last_of('\\');

			if (offset != std::string::npos)
				filename = pMaterial->colorMapFilename.substr(++offset);
			else
				filename = pMaterial->colorMapFilename;

			// Try loading the texture from the same directory as the OBJ file.
			textureId = LoadTexture((g_model.getPath() + filename).c_str());
		}

		if (textureId)
			g_modelTextures[pMaterial->colorMapFilename] = textureId;

		// Look for and load any normal map textures.

		if (pMaterial->bumpMapFilename.empty())
			continue;

		// Try load the texture using the path in the .MTL file.
		textureId = LoadTexture(pMaterial->bumpMapFilename.c_str());

		if (!textureId)
		{
			offset = pMaterial->bumpMapFilename.find_last_of('\\');

			if (offset != std::string::npos)
				filename = pMaterial->bumpMapFilename.substr(++offset);
			else
				filename = pMaterial->bumpMapFilename;

			// Try loading the texture from the same directory as the OBJ file.
			textureId = LoadTexture((g_model.getPath() + filename).c_str());
		}

		if (textureId)
			g_modelTextures[pMaterial->bumpMapFilename] = textureId;
	}

}

void InitApp(char* filname)
{

	glewInit();





	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	std::string infoLog;

	g_blinnPhongShader = LoadShaders("blinn_phong.vert", "blinn_phong.frag");
	g_normalMappingShader = LoadShaders("normal_mapping.vert", "normal_mapping.frag");

	if (g_blinnPhongShader == NULL)
		printf("erorr in normal map shader \n");



	if (g_normalMappingShader == NULL)
		printf("erorr in normal map shader \n");


	LoadModel(filname);


	///Data maybe helpfull to know..
	printf("number of meshes = %d \n", g_model.getNumberOfMeshes());
	printf("number of vertices = %d \n", g_model.getNumberOfVertices());

	printf("has texcoors = %d \n", g_model.hasTextureCoords());
	printf("has pos = %d \n", g_model.hasPositions());
	printf("has normal = %d \n", g_model.hasNormals());
}



//////////////////Drawing..


void DrawModelUsingProgrammablePipeline()
{
	const ModelOBJ::Mesh *pMesh = 0;
	const ModelOBJ::Material *pMaterial = 0;
	const ModelOBJ::Vertex *pVertices = 0;
	ModelTextures::const_iterator iter;
	GLuint texture = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (int i = 0; i < g_model.getNumberOfMeshes(); ++i)
	{
		pMesh = &g_model.getMesh(i);
		pMaterial = pMesh->pMaterial;
		pVertices = g_model.getVertexBuffer();

		/*glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, pMaterial->ambient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, pMaterial->diffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, pMaterial->specular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, pMaterial->shininess * 128.0f);*/

		if (pMaterial->bumpMapFilename.empty())
		{
			// Per fragment Blinn-Phong code path.

			glUseProgram(g_blinnPhongShader);

			// Bind the color map texture.

			iter = g_modelTextures.find(pMaterial->colorMapFilename);

			if (iter != g_modelTextures.end())
				texture = iter->second;

			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture);

			// Update shader parameters.

			glUniform1i(glGetUniformLocation(
				g_blinnPhongShader, "colorMap"), 0);
			glUniform1f(glGetUniformLocation(
				g_blinnPhongShader, "materialAlpha"), pMaterial->alpha);
		}
		else
		{
			// Normal mapping code path.

			glUseProgram(g_normalMappingShader);

			// Bind the normal map texture.

			iter = g_modelTextures.find(pMaterial->bumpMapFilename);

			if (iter != g_modelTextures.end())
			{
				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, iter->second);
			}

			// Bind the color map texture.


			iter = g_modelTextures.find(pMaterial->colorMapFilename);

			if (iter != g_modelTextures.end())
				texture = iter->second;

			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture);

			// Update shader parameters.

			glUniform1i(glGetUniformLocation(
				g_normalMappingShader, "colorMap"), 0);
			glUniform1i(glGetUniformLocation(
				g_normalMappingShader, "normalMap"), 1);
			glUniform1f(glGetUniformLocation(
				g_normalMappingShader, "materialAlpha"), pMaterial->alpha);
		}

		// Render mesh.

		if (g_model.hasPositions())
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, g_model.getVertexSize(),
				g_model.getVertexBuffer()->position);
		}

		if (g_model.hasTextureCoords())
		{
			glClientActiveTexture(GL_TEXTURE0);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, g_model.getVertexSize(),
				g_model.getVertexBuffer()->texCoord);
		}

		if (g_model.hasNormals())
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, g_model.getVertexSize(),
				g_model.getVertexBuffer()->normal);
		}

		if (g_model.hasTangents())
		{
			glClientActiveTexture(GL_TEXTURE1);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(4, GL_FLOAT, g_model.getVertexSize(),
				g_model.getVertexBuffer()->tangent);
		}

		glDrawElements(GL_TRIANGLES, pMesh->triangleCount * 3, GL_UNSIGNED_INT,
			g_model.getIndexBuffer() + pMesh->startIndex);

		if (g_model.hasTangents())
		{
			glClientActiveTexture(GL_TEXTURE1);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		if (g_model.hasNormals())
			glDisableClientState(GL_NORMAL_ARRAY);

		if (g_model.hasTextureCoords())
		{
			glClientActiveTexture(GL_TEXTURE0);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		if (g_model.hasPositions())
			glDisableClientState(GL_VERTEX_ARRAY);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
	glDisable(GL_BLEND);
}

void DrawFrame()
{
	DrawModelUsingProgrammablePipeline();
}


void UnloadModel()
{

	ModelTextures::iterator i = g_modelTextures.begin();

	while (i != g_modelTextures.end())
	{
		glDeleteTextures(1, &i->second);
		++i;
	}

	g_modelTextures.clear();
	g_model.destroy();


}