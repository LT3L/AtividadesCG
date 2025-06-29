/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 13/08/2024
 *
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <cstring>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

#include <cmath>

// Função loadOBJ exatamente igual ao AtividadeVivencial1.cpp
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
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
GLuint loadTexture(string filePath, int &width, int &height);
GLuint createSimpleVAO(const std::vector<vec3>& vertices, const std::vector<vec2>& uvs, const std::vector<vec3>& normals);

void drawGeometry(GLuint shaderID, GLuint VAO, vec3 position, vec3 dimensions, float angle, int nVertices, vec3 color= vec3(1.0,0.0,0.0), vec3 axis = (vec3(0.0, 0.0, 1.0)));

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 2880, HEIGHT = 1800;

bool light1Enabled = true;
bool light2Enabled = true;
bool light3Enabled = true;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;

uniform mat4 projection;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;
void main()
{
   	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
	fragPos = model * vec4(position.x, position.y, position.z, 1.0);
	texCoord = texc;
	vNormal = normal;
	vColor = vec4(color,1.0);
})";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform vec3 lightPos3;
uniform vec3 camPos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;
out vec4 color;
in vec4 fragPos;
in vec3 vNormal;
in vec4 vColor;
uniform bool light1Enabled;
uniform bool light2Enabled;
uniform bool light3Enabled;
void main()
{

	vec3 lightColor = vec3(1.0,1.0,1.0);
	//vec4 objectColor = texture(texBuff,texCoord);
	vec4 objectColor = vColor;

	//Coeficiente de luz ambiente
	vec3 ambient = ka * lightColor;

	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);

	//Coeficiente de reflexão difusa e especular
	vec3 N = normalize(vNormal);
	vec3 V = normalize(camPos - vec3(fragPos));

	// Luz Principal
	if (light1Enabled) {
		vec3 L1 = normalize(lightPos1 - vec3(fragPos));
		float diff1 = max(dot(N, L1),0.0);
		diffuse += kd * diff1 * 1.0 * vec3(1.0, 1.0, 1.0); // como é principal ficou x 1
		
		vec3 R1 = normalize(reflect(-L1,N));
		float spec1 = max(dot(R1,V),0.0);
		spec1 = pow(spec1,q);
		specular += ks * spec1 * 1.0 * vec3(1.0, 1.0, 1.0);
	}

	// Luz de Preenchimento
	if (light2Enabled) {
		vec3 L2 = normalize(lightPos2 - vec3(fragPos));
		float diff2 = max(dot(N, L2),0.0);
		diffuse += kd * diff2 * 0.4 * vec3(1.0, 1.0, 1.0); // x 0.4
		
		vec3 R2 = normalize(reflect(-L2,N));
		float spec2 = max(dot(R2,V),0.0);
		spec2 = pow(spec2,q);
		specular += ks * spec2 * 0.4 * vec3(1.0, 1.0, 1.0);
	}

	// Luz de Fundo
	if (light3Enabled) {
		vec3 L3 = normalize(lightPos3 - vec3(fragPos));
		float diff3 = max(dot(N, L3),0.0);
		diffuse += kd * diff3 * 0.65 * vec3(1.0, 1.0, 1.0); // x 0.65
		
		vec3 R3 = normalize(reflect(-L3,N));
		float spec3 = max(dot(R3,V),0.0);
		spec3 = pow(spec3,q);
		specular += ks * spec3 * 0.65 * vec3(1.0, 1.0, 1.0);
	}

	vec3 result = (ambient + diffuse) * vec3(objectColor) + specular;
	color = vec4(result,1.0);

})";

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Muita atenção aqui: alguns ambientes não aceitam essas configurações
	// Você deve adaptar para a versão do OpenGL suportada por sua placa
	// Sugestão: comente essas linhas de código para desobrir a versão e
	// depois atualize (por exemplo: 4.5 com 4 e 5)
	/*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);*/

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "PRessione 1 a 3 para mudar as luzes", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Carregando modelo OBJ
	std::vector<vec3> vertices;
	std::vector<vec2> uvs;
	std::vector<vec3> normals;
	
	loadOBJ("../assets/Modelos3D/Suzanne.obj", vertices, uvs, normals);
	
	int nVertices = vertices.size();
	GLuint VAO = createSimpleVAO(vertices, uvs, normals);

	// Carregando uma textura e armazenando seu id
	int imgWidth, imgHeight;
	GLuint texID = loadTexture("../assets/tex/pixelWall.png",imgWidth,imgHeight);

	float ka = 0.1, kd =0.5, ks = 0.5, q = 10.0;
	
	// Luz Principal
	vec3 lightPos = vec3(1.5, 2.0, -1.5);
	
	// Luz de Preenchimento
	vec3 lightPos2 = vec3(-1.0, 0.4, -0.6);
	
	// Back Light (Luz de Fundo) - atrás do objeto, cria contorno
	vec3 lightPos3 = vec3(0.0, 0.8, 2);
	
	vec3 camPos = vec3(0.0,0.0,-20.0);


	glUseProgram(shaderID);

	// Enviar a informação de qual variável armazenará o buffer da textura
	glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

	glUniform1f(glGetUniformLocation(shaderID, "ka"), ka);
	glUniform1f(glGetUniformLocation(shaderID, "kd"), kd);
	glUniform1f(glGetUniformLocation(shaderID, "ks"), ks);
	glUniform1f(glGetUniformLocation(shaderID, "q"), q);
	glUniform3f(glGetUniformLocation(shaderID, "lightPos1"), lightPos.x,lightPos.y,lightPos.z);
	glUniform3f(glGetUniformLocation(shaderID, "lightPos2"), lightPos2.x,lightPos2.y,lightPos2.z);
	glUniform3f(glGetUniformLocation(shaderID, "lightPos3"), lightPos3.x,lightPos3.y,lightPos3.z);
	glUniform3f(glGetUniformLocation(shaderID, "camPos"), camPos.x,camPos.y,camPos.z);
	glUniform1i(glGetUniformLocation(shaderID, "light1Enabled"), light1Enabled);
	glUniform1i(glGetUniformLocation(shaderID, "light2Enabled"), light2Enabled);
	glUniform1i(glGetUniformLocation(shaderID, "light3Enabled"), light3Enabled);

	//Ativando o primeiro buffer de textura da OpenGL
	glActiveTexture(GL_TEXTURE0);
	

	// Matriz de projeção paralela ortográfica
	// mat4 projection = ortho(-10.0, 10.0, -10.0, 10.0, -1.0, 1.0);
	mat4 projection = ortho(-1.0, 1.0, -1.0, 1.0, -3.0, 3.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	// Matriz de modelo: transformações na geometria (objeto)
	mat4 model = mat4(1); // matriz identidade
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Atualiza os uniformes das luzes no shader
		glUniform1i(glGetUniformLocation(shaderID, "light1Enabled"), light1Enabled);
		glUniform1i(glGetUniformLocation(shaderID, "light2Enabled"), light2Enabled);
		glUniform1i(glGetUniformLocation(shaderID, "light3Enabled"), light3Enabled);

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT);

		glBindVertexArray(VAO); // Conectando ao buffer de geometria
		glBindTexture(GL_TEXTURE_2D, texID); //conectando com o buffer de textura que será usado no draw

		// Primeiro Triângulo
		drawGeometry(shaderID, VAO, vec3(0, 0, 0), vec3(0.3, 0.3, 0.3), 0.0, nVertices);

		glBindVertexArray(0); // Desconectando o buffer de geometria

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	
	// Controle das luzes
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_1) {
			light1Enabled = !light1Enabled;
			std::cout << "Luz Principal " << (light1Enabled ? "ligada" : "desligada") << std::endl;
		}
		else if (key == GLFW_KEY_2) {
			light2Enabled = !light2Enabled;
			std::cout << "Luz de Preenchimento: " << (light2Enabled ? "ligada" : "desligada") << std::endl;
		}
		else if (key == GLFW_KEY_3) {
			light3Enabled = !light3Enabled;
			std::cout << "Luz de Fundo: " << (light3Enabled ? "ligada" : "desligada") << std::endl;
		}
	}
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
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
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
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
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

void drawGeometry(GLuint shaderID, GLuint VAO, vec3 position, vec3 dimensions, float angle, int nVertices, vec3 color, vec3 axis)
{
	// Matriz de modelo: transformações na geometria (objeto)
	mat4 model = mat4(1); // matriz identidade
	// Translação
	model = translate(model, position);
	// Rotação
	model = rotate(model, radians(angle), axis);
	// Escala
	model = scale(model, dimensions);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	//glUniform4f(glGetUniformLocation(shaderID, "inputColor"), color.r, color.g, color.b, 1.0f); // enviando cor para variável uniform inputColor
																								//  Chamada de desenho - drawcall
																								//  Poligono Preenchido - GL_TRIANGLES
	glDrawArrays(GL_TRIANGLES, 0, nVertices);
}

GLuint createSimpleVAO(const std::vector<vec3>& vertices, const std::vector<vec2>& uvs, const std::vector<vec3>& normals) {
    // Buffer simples - apenas posição, cor fixa, normal básica e UV padrão
    std::vector<GLfloat> vBuffer;
    vec3 color = vec3(1.0f, 0.5f, 0.0f); // Laranja
    
    for (size_t i = 0; i < vertices.size(); i++) {
        // Posição
        vBuffer.push_back(vertices[i].x);
        vBuffer.push_back(vertices[i].y);
        vBuffer.push_back(vertices[i].z);
        
        // Cor fixa
        vBuffer.push_back(color.r);
        vBuffer.push_back(color.g);
        vBuffer.push_back(color.b);
        
        // Normal básica (apontando pra frente)
        vBuffer.push_back(normals[i].x);
        vBuffer.push_back(normals[i].y);
        vBuffer.push_back(normals[i].z);
        
        // UV padrão
        vBuffer.push_back(uvs[i].x);
        vBuffer.push_back(uvs[i].y);
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    // Configurar atributos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    return VAO;
}