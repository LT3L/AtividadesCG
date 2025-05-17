/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <sstream>  // Add this for istringstream

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>

#include <glm/glm.hpp>

/*
Referências para o loader OBJ e lógica de carregamento de modelos:
- https://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/
- https://github.com/opengl-tutorials/ogl/blob/master/common/objloader.cpp
*/

// Very, VERY simple OBJ loader.
// Here is a short list of features a real function would provide : 
// - Binary files. Reading a model should be just a few memcpy's away, not parsing a file at runtime. In short : OBJ is not very great.
// - Animations & bones (includes bones weights)
// - Multiple UVs
// - All attributes should be optional, not "forced"
// - More stable. Change a line in the OBJ file and it crashes.
// - More secure. Change another line and you can inject code.
// - Loading from memory, stream, etc

bool loadOBJ(
	const char * path, 
	std::vector<glm::vec3> & out_vertices, 
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
){
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices; 
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if( file == NULL ){
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while( 1 ){

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader
		
		if ( strcmp( lineHeader, "v" ) == 0 ){
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
			temp_vertices.push_back(vertex);
		}else if ( strcmp( lineHeader, "vt" ) == 0 ){
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y );
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}else if ( strcmp( lineHeader, "vn" ) == 0 ){
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
			temp_normals.push_back(normal);
		}else if ( strcmp( lineHeader, "f" ) == 0 ){
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
			if (matches != 9){
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				fclose(file);
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices    .push_back(uvIndex[0]);
			uvIndices    .push_back(uvIndex[1]);
			uvIndices    .push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}else{
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for( unsigned int i=0; i<vertexIndices.size(); i++ ){

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];
		
		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
		glm::vec2 uv = temp_uvs[ uvIndex-1 ];
		glm::vec3 normal = temp_normals[ normalIndex-1 ];
		
		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);
	}
	fclose(file);
	return true;
}


// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1920, HEIGHT = 1080;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = "#version 450\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = projection * view * model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

//Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 450\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

// Struct to represent a 3D model
struct Model {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    GLuint VAO;
};

// Global variables for model management
std::vector<Model> models;
int selectedModelIndex = 0;
bool isTranslating = false;
bool isScaling = false;
bool isRotating = false;

enum Mode { TRANSLATE, ROTATE, SCALE };
Mode mode = ROTATE;

// Função para criar VAO/VBO a partir dos vértices
GLuint createVAOFromVertices(const std::vector<glm::vec3>& vertices) {
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

// Função principal
int main()
{
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "[CRT] modo [WASD] para mover e [~TAB] objeto - Lucas Tres de Lima!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Começa no modo de rotação
	isTranslating = false;
	isScaling = false;
	isRotating = true;
	mode = ROTATE;

	// Registra o callback do teclado
	glfwSetKeyCallback(window, key_callback);

	// Inicializa o GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Erro ao inicializar GLAD" << std::endl;
		return -1;
	}

	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compila os shaders
	GLuint shaderID = setupShader();

	// Carrega os modelos
	Model model1, model2;
	loadOBJ("assets/Modelos3D/Suzanne.obj", model1.vertices, model1.uvs, model1.normals);
	loadOBJ("assets/Modelos3D/SuzanneSubdiv1.obj", model2.vertices, model2.uvs, model2.normals);

	model1.position = glm::vec3(-2.0f, 0.0f, 0.0f);
	model2.position = glm::vec3(2.0f, 0.0f, 0.0f);
	model1.scale = glm::vec3(0.5f);
	model2.scale = glm::vec3(0.5f);

	model1.VAO = createVAOFromVertices(model1.vertices);
	model2.VAO = createVAOFromVertices(model2.vertices);

	models.push_back(model1);
	models.push_back(model2);

	glUseProgram(shaderID);
	glEnable(GL_DEPTH_TEST);

	// Loop principal
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLineWidth(10);
		glPointSize(20);

		float angle = (GLfloat)glfwGetTime();

		// Matriz de visão
		glm::mat4 view = glm::lookAt(
			glm::vec3(0.0f, 0.0f, 5.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)
		);

		// Matriz de projeção
		glm::mat4 projection = glm::perspective(
			glm::radians(45.0f),
			(float)WIDTH / (float)HEIGHT,
			0.1f,
			100.0f
		);

		// Passa as matrizes para o shader
		GLint viewLoc = glGetUniformLocation(shaderID, "view");
		GLint projLoc = glGetUniformLocation(shaderID, "projection");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		// Desenha cada modelo
		for (size_t i = 0; i < models.size(); i++) {
			Model& model = models[i];
			glm::mat4 modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, model.position);
			modelMatrix = glm::rotate(modelMatrix, glm::radians(model.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			modelMatrix = glm::rotate(modelMatrix, glm::radians(model.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = glm::rotate(modelMatrix, glm::radians(model.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			modelMatrix = glm::scale(modelMatrix, model.scale);

			GLint modelLoc = glGetUniformLocation(shaderID, "model");
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

			glBindVertexArray(model.VAO);
			glDrawArrays(GL_TRIANGLES, 0, model.vertices.size());
			if (i == selectedModelIndex) {
				glDrawArrays(GL_POINTS, 0, model.vertices.size());
			}
		}

		glfwSwapBuffers(window);
	}

	// Libera recursos
	for (Model& model : models) {
		glDeleteVertexArrays(1, &model.VAO);
	}
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		selectedModelIndex = (selectedModelIndex + 1) % models.size();
		std::cout << "Selected model: " << selectedModelIndex << std::endl;
	}

	// Modo de transformação
	if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_T:
				isTranslating = true;
				isScaling = false;
				isRotating = false;
				mode = TRANSLATE;
				std::cout << "Translation mode" << std::endl;
				break;
			case GLFW_KEY_C:
				isTranslating = false;
				isScaling = true;
				isRotating = false;
				mode = SCALE;
				std::cout << "Scaling mode" << std::endl;
				break;
			case GLFW_KEY_R:
				isTranslating = false;
				isScaling = false;
				isRotating = true;
				mode = ROTATE;
				std::cout << "Rotation mode" << std::endl;
				break;
		}
	}

	// Handle transformations for selected model
	if (models.size() > 0 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		Model& selectedModel = models[selectedModelIndex];
		float step = 0.1f;
		float rotationStep = 5.0f;

		if (isTranslating) {
			if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
				selectedModel.position.x -= step;
				std::cout << "Moving left" << std::endl;
			}
			if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
				selectedModel.position.x += step;
				std::cout << "Moving right" << std::endl;
			}
			if (key == GLFW_KEY_UP || key == GLFW_KEY_W) {
				selectedModel.position.y += step;
				std::cout << "Moving up" << std::endl;
			}
			if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) {
				selectedModel.position.y -= step;
				std::cout << "Moving down" << std::endl;
			}
		}
		else if (isScaling) {
			if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
				selectedModel.scale.x -= step;
				std::cout << "Scaling X down" << std::endl;
			}
			if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
				selectedModel.scale.x += step;
				std::cout << "Scaling X up" << std::endl;
			}
			if (key == GLFW_KEY_UP || key == GLFW_KEY_W) {
				selectedModel.scale.y += step;
				std::cout << "Scaling Y up" << std::endl;
			}
			if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) {
				selectedModel.scale.y -= step;
				std::cout << "Scaling Y down" << std::endl;
			}
		}
		else if (isRotating) {
			if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
				selectedModel.rotation.y -= rotationStep;
				std::cout << "Rotating Y left" << std::endl;
			}
			if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
				selectedModel.rotation.y += rotationStep;
				std::cout << "Rotating Y right" << std::endl;
			}
			if (key == GLFW_KEY_UP || key == GLFW_KEY_W) {
				selectedModel.rotation.x += rotationStep;
				std::cout << "Rotating X up" << std::endl;
			}
			if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) {
				selectedModel.rotation.x -= rotationStep;
				std::cout << "Rotating X down" << std::endl;
			}
		}
	}
}

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

