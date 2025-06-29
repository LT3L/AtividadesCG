/* Hello Triangle - codigo adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Grafico - Unisinos
 * Versao inicial: 7/4/2017
 * Ultima atualizacao em 13/08/2024
 *
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>
#include <regex>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Struct para representar propriedades do material
struct Material {
    std::string name = "default";
    glm::vec3 ka = glm::vec3(0.2f, 0.2f, 0.2f);  // Coeficiente ambiente
    glm::vec3 kd = glm::vec3(0.8f, 0.8f, 0.8f);  // Coeficiente difuso
    glm::vec3 ks = glm::vec3(0.5f, 0.5f, 0.5f);  // Coeficiente especular
    float shininess = 32.0f;                      // Brilho especular (Ns no .mtl)
    std::string diffuseTexture = "";              // Caminho da textura difusa (map_Kd)
    GLuint textureID = 0;                         // ID da textura carregada
};

// Struct para representar uma parte de um modelo com material especifico
struct ModelPart {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    Material material;
    GLuint VAO;
    int nVertices;
    std::string materialName;
};

// Struct para representar um modelo 3D (pode ter múltiplas partes com materiais diferentes)
struct Model {
    std::vector<ModelPart> parts;  // Lista de partes com materiais diferentes
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    std::string name;              // Nome do modelo para debug
};

// Struct para representar a câmera
struct Camera {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float speed = 200.0f;  // Velocidade muito aumentada para navegação rápida
} camera;

// Enums para tipos de animacao
enum AnimationType {
    ANIM_NONE,
    ANIM_BEZIER,
    ANIM_BEZIER_TRACK,  // Nova animação para seguir pista
    ANIM_ORBIT,
    ANIM_LINEAR
};

// Struct para curva Bézier cúbica
struct BezierCurve {
    glm::vec3 p0, p1, p2, p3;  // Pontos de controle
    float duration;            // Duracao em segundos
    bool loop;                 // Se deve repetir
};

// Struct para track com múltiplas curvas de Bézier
struct BezierTrack {
    std::vector<BezierCurve> curves;  // Sequência de curvas
    float totalDuration;              // Duração total do percurso
    bool loop;                        // Se deve repetir o percurso
};

// Struct para animacao orbital
struct OrbitAnimation {
    glm::vec3 center;          // Centro da órbita
    float radius;              // Raio da órbita
    float speed;               // Velocidade em graus/segundo
    glm::vec3 axis;            // Eixo de rotacao (padrao Y)
};

// Struct para animacao de um objeto
struct ObjectAnimation {
    AnimationType type = ANIM_NONE;
    BezierCurve bezier;
    BezierTrack bezierTrack;    // Nova animação de pista
    OrbitAnimation orbit;
    glm::vec3 originalPosition; // Posicao original do objeto
};

// Variaveis globais para gerenciamento de modelos
std::vector<Model> models;
std::vector<ObjectAnimation> objectAnimations; // Animacões correspondentes aos modelos
int selectedModelIndex = 0;
bool isTranslating = false;
bool isScaling = false;
bool isRotating = true; // Comeca no modo rotacao

// Controle de modo: false = controla objetos, true = controla câmera
bool cameraControlMode = false;

// Sistema de animacao
bool animationEnabled = false;
float animationTime = 0.0f;

enum Mode { TRANSLATE, ROTATE, SCALE };
Mode currentMode = ROTATE;

// Função para criar VAO de uma parte de modelo (definida antes para ser usada)
GLuint createModelPartVAO(const ModelPart& part) {
    // Cria buffer com dados da parte do modelo
    std::vector<GLfloat> vBuffer;
    
    for (size_t i = 0; i < part.vertices.size(); i++) {
        // Posicao
        vBuffer.push_back(part.vertices[i].x);
        vBuffer.push_back(part.vertices[i].y);
        vBuffer.push_back(part.vertices[i].z);
        
        // Cor baseada no material
        vBuffer.push_back(part.material.kd.r);
        vBuffer.push_back(part.material.kd.g);
        vBuffer.push_back(part.material.kd.b);
        
        // Normal
        vBuffer.push_back(part.normals[i].x);
        vBuffer.push_back(part.normals[i].y);
        vBuffer.push_back(part.normals[i].z);
        
        // UV
        vBuffer.push_back(part.uvs[i].x);
        vBuffer.push_back(part.uvs[i].y);
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

// Loader OBJ que suporta multiplos materiais
bool loadOBJWithMaterials(const char * path, Model& model, const std::vector<Material>& materials) {
	
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;
	
	// Map para agrupar faces por material
	std::map<std::string, std::vector<unsigned int>> materialVertexIndices;
	std::map<std::string, std::vector<unsigned int>> materialUvIndices;
	std::map<std::string, std::vector<unsigned int>> materialNormalIndices;
	
	std::string currentMaterial = "default";
	
	FILE * file = fopen(path, "r");
	if (file == NULL) {
		return false;
	}
	
	while (true) {
		char lineHeader[128];
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF) break;
		
		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Inverter coordenada V
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "usemtl") == 0) {
			fscanf(file, "%s", lineHeader);
			currentMaterial = std::string(lineHeader);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			// Tentar ler como triângulo primeiro
			unsigned int v1, v2, v3, v4 = 0;
			unsigned int vt1, vt2, vt3, vt4 = 0;
			unsigned int vn1, vn2, vn3, vn4 = 0;
			
			// Ler a linha restante
			char faceLine[1000];
			fgets(faceLine, 1000, file);
			
			// Tentar parse como quad primeiro
			int matches = sscanf(faceLine, "%u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u", 
				&v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3, &v4, &vt4, &vn4);
			
			if (matches == 12) {
				// É um quad - converter para 2 triângulos
				// Primeiro triângulo: v1, v2, v3
				materialVertexIndices[currentMaterial].push_back(v1);
				materialVertexIndices[currentMaterial].push_back(v2);
				materialVertexIndices[currentMaterial].push_back(v3);
				materialUvIndices[currentMaterial].push_back(vt1);
				materialUvIndices[currentMaterial].push_back(vt2);
				materialUvIndices[currentMaterial].push_back(vt3);
				materialNormalIndices[currentMaterial].push_back(vn1);
				materialNormalIndices[currentMaterial].push_back(vn2);
				materialNormalIndices[currentMaterial].push_back(vn3);
				
				// Segundo triângulo: v1, v3, v4
				materialVertexIndices[currentMaterial].push_back(v1);
				materialVertexIndices[currentMaterial].push_back(v3);
				materialVertexIndices[currentMaterial].push_back(v4);
				materialUvIndices[currentMaterial].push_back(vt1);
				materialUvIndices[currentMaterial].push_back(vt3);
				materialUvIndices[currentMaterial].push_back(vt4);
				materialNormalIndices[currentMaterial].push_back(vn1);
				materialNormalIndices[currentMaterial].push_back(vn3);
				materialNormalIndices[currentMaterial].push_back(vn4);
			} else {
				// Tentar como triângulo
				matches = sscanf(faceLine, "%u/%u/%u %u/%u/%u %u/%u/%u", 
					&v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				
				if (matches == 9) {
					// É um triângulo
					materialVertexIndices[currentMaterial].push_back(v1);
					materialVertexIndices[currentMaterial].push_back(v2);
					materialVertexIndices[currentMaterial].push_back(v3);
					materialUvIndices[currentMaterial].push_back(vt1);
					materialUvIndices[currentMaterial].push_back(vt2);
					materialUvIndices[currentMaterial].push_back(vt3);
					materialNormalIndices[currentMaterial].push_back(vn1);
					materialNormalIndices[currentMaterial].push_back(vn2);
					materialNormalIndices[currentMaterial].push_back(vn3);
				}
			}
		}
		else {
			// Pular linha
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}
	}
	
	fclose(file);
	
	// Criar uma parte do modelo para cada material
	for (auto& pair : materialVertexIndices) {
		std::string matName = pair.first;
		std::vector<unsigned int>& vertexIndices = pair.second;
		std::vector<unsigned int>& uvIndices = materialUvIndices[matName];
		std::vector<unsigned int>& normalIndices = materialNormalIndices[matName];
		
		ModelPart part;
		part.materialName = matName;
		
		// Verificar se ha faces suficientes para este material
		if (vertexIndices.size() % 3 != 0) {
			continue;
		}
		
		// Converter indices para vertices
		for (size_t i = 0; i < vertexIndices.size(); i++) {
			if (vertexIndices[i] > temp_vertices.size() || 
			    uvIndices[i] > temp_uvs.size() || 
			    normalIndices[i] > temp_normals.size()) {
				continue;
			}
			
			part.vertices.push_back(temp_vertices[vertexIndices[i] - 1]);
			part.uvs.push_back(temp_uvs[uvIndices[i] - 1]);
			
			// Garantir que todas as normais estao normalizadas
			glm::vec3 normal = temp_normals[normalIndices[i] - 1];
			if (length(normal) > 0.0001f) {
				normal = normalize(normal);
			} else {
				normal = glm::vec3(0.0f, 1.0f, 0.0f);
			}
			part.normals.push_back(normal);
		}
		
		part.nVertices = part.vertices.size();
		
		// Encontrar material correspondente
		bool found = false;
		for (const auto& mat : materials) {
			if (mat.name == matName) {
				part.material = mat;
				found = true;
				break;
			}
		}
		
		if (!found) {
			// Criar material padrao
			part.material.name = matName;
			part.material.kd = glm::vec3(0.7f, 0.7f, 0.7f);
		}
		
		// Ajustar propriedades para materiais especificos
		if (matName == "Decals") {
			part.material.ka = glm::vec3(0.8f, 0.8f, 0.8f);
			part.material.kd = glm::vec3(1.0f, 1.0f, 1.0f);
			part.material.ks = glm::vec3(0.1f, 0.1f, 0.1f);
			part.material.shininess = 1.0f;
		} else if (matName == "Asphalt") {
			part.material.ka = glm::vec3(0.1f, 0.1f, 0.1f);
			part.material.ks = glm::vec3(0.2f, 0.2f, 0.2f);
			part.material.shininess = 4.0f;
		}
		
		// Criar VAO para esta parte
		part.VAO = createModelPartVAO(part);
		
		model.parts.push_back(part);
	}
	
	return model.parts.size() > 0;
}

// Funcao loadOBJ original mantida para compatibilidade
bool loadOBJ(
	const char * path, 
	std::vector<glm::vec3> & out_vertices, 
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
){

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices; 
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	FILE * file = fopen(path, "r");
	if( file == NULL ){
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

// Protótipo da funcao de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Struct para configuracao de cena
struct SceneConfig {
    struct CameraConfig {
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
        float fov = 45.0f;
    } camera;
    
    struct LightConfig {
        glm::vec3 position;
        float intensity;
        bool enabled = true;
    };
    std::vector<LightConfig> lights;
    
    struct ObjectConfig {
        std::string filename;
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        std::string materialType;
        ObjectAnimation animation; // Animacao específica do objeto
    };
    std::vector<ObjectConfig> objects;
};

// Protótipos das funcões
int setupShader();
GLuint loadTexture(string filePath, int &width, int &height);
GLuint createSimpleVAO(const std::vector<vec3>& vertices, const std::vector<vec2>& uvs, const std::vector<vec3>& normals);
GLuint createModelVAO(const Model& model);
GLuint createModelPartVAO(const ModelPart& part);
bool loadOBJWithMaterials(const char * path, Model& model, const std::vector<Material>& materials);

void drawGeometry(GLuint shaderID, GLuint VAO, vec3 position, vec3 dimensions, float angle, int nVertices, vec3 color= vec3(1.0,0.0,0.0), vec3 axis = (vec3(0.0, 0.0, 1.0)));
void drawModel(GLuint shaderID, const Model& model);
void updateCameraMatrix(GLuint shaderID);
void processCameraMovement(int key, float deltaTime);
bool loadSceneConfig(const std::string& filename, SceneConfig& config);
Material createMaterial(const std::string& materialType);
bool loadMTL(const std::string& filename, std::vector<Material>& materials);
Material findMaterialByName(const std::vector<Material>& materials, const std::string& name);
std::string getMTLFilename(const std::string& objFilename);
void updateAnimations(float deltaTime);
void applyAnimationsToModels();
vec3 calculateLightIntensity(int lightIndex, float time);
glm::vec3 calculateBezierPosition(const BezierCurve& curve, float t);
glm::vec3 calculateBezierTrackPosition(const BezierTrack& track, float t, glm::vec3& direction);
glm::vec3 calculateOrbitPosition(const OrbitAnimation& orbit, float time);
bool parseAnimationConfig(const std::string& line, ObjectAnimation& animation);
void loadMaterialTextures(std::vector<Material>& materials, const std::string& baseDir);

// Dimensões da janela (pode ser alterado em tempo de execucao)
const GLuint WIDTH = 2880, HEIGHT = 1800;

// Configuracao de cena carregada do arquivo
SceneConfig sceneConfig;

bool light1Enabled = true;
bool light2Enabled = true;
bool light3Enabled = true;

// Vertex Shader
const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;
void main()
{
   	gl_Position = projection * view * model * vec4(position, 1.0);
	fragPos = model * vec4(position, 1.0);
	texCoord = texc;
	vNormal = mat3(transpose(inverse(model))) * normal;
	vColor = vec4(color, 1.0);
})";

// Fragment Shader
const GLchar *fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform vec3 lightPos3;
uniform vec3 camPos;
uniform vec3 materialKa;
uniform vec3 materialKd;
uniform vec3 materialKs;
uniform float materialShininess;
uniform float lightIntensity1;
uniform float lightIntensity2;
uniform float lightIntensity3;
uniform int isDecalMaterial;
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
	
	// Combinar textura com cor do material
	vec4 textureColor = texture(texBuff, texCoord);
	vec4 objectColor = vColor;
	
	// Tratamento especial para materiais de placas/decals
	if (isDecalMaterial == 1) {
		// Para placas, priorizar a textura e reduzir efeito da iluminação
		if (length(textureColor.rgb) > 0.05) {
			objectColor = mix(vColor, textureColor, 0.95); // 95% textura, 5% cor do material
		}
	} else {
		// Para outros materiais, comportamento normal
		if (length(textureColor.rgb) > 0.1) {
			objectColor = mix(vColor, textureColor, 0.8); // 80% textura, 20% cor do material
		}
	}

	//Coeficiente de luz ambiente
	vec3 ambient = materialKa * lightColor;

	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);

	//Coeficiente de reflexao difusa e especular
	vec3 N = normalize(vNormal);
	vec3 V = normalize(camPos - vec3(fragPos));

	// Luz Principal
	if (light1Enabled) {
		vec3 L1 = normalize(lightPos1 - vec3(fragPos));
		float diff1 = max(dot(N, L1),0.0);
		diffuse += materialKd * diff1 * lightIntensity1 * vec3(1.0, 1.0, 1.0);
		
		vec3 R1 = normalize(reflect(-L1,N));
		float spec1 = max(dot(R1,V),0.0);
		spec1 = pow(spec1, materialShininess);
		specular += materialKs * spec1 * lightIntensity1 * vec3(1.0, 1.0, 1.0);
	}

	// Luz de Preenchimento
	if (light2Enabled) {
		vec3 L2 = normalize(lightPos2 - vec3(fragPos));
		float diff2 = max(dot(N, L2),0.0);
		diffuse += materialKd * diff2 * lightIntensity2 * 0.4 * vec3(1.0, 1.0, 1.0);
		
		vec3 R2 = normalize(reflect(-L2,N));
		float spec2 = max(dot(R2,V),0.0);
		spec2 = pow(spec2, materialShininess);
		specular += materialKs * spec2 * lightIntensity2 * 0.4 * vec3(1.0, 1.0, 1.0);
	}

	// Luz de Fundo
	if (light3Enabled) {
		vec3 L3 = normalize(lightPos3 - vec3(fragPos));
		float diff3 = max(dot(N, L3),0.0);
		diffuse += materialKd * diff3 * lightIntensity3 * 0.65 * vec3(1.0, 1.0, 1.0);
		
		vec3 R3 = normalize(reflect(-L3,N));
		float spec3 = max(dot(R3,V),0.0);
		spec3 = pow(spec3, materialShininess);
		specular += materialKs * spec3 * lightIntensity3 * 0.65 * vec3(1.0, 1.0, 1.0);
	}

	vec3 result;
	
	if (isDecalMaterial == 1) {
		// Para placas/decals: reduzir impacto da iluminação, manter textura visível
		vec3 minLighting = vec3(0.6, 0.6, 0.6); // Iluminação mínima para placas
		vec3 lighting = max(minLighting, ambient + diffuse * 0.5); // Reduzir difusa
		result = lighting * vec3(objectColor) + specular * 0.2; // Reduzir especular
	} else {
		// Iluminação normal para outros materiais
		result = (ambient + diffuse) * vec3(objectColor) + specular;
	}
	
	// Garantir que a cor final seja sempre opaca (alpha = 1.0)
	color = vec4(result, 1.0);
	
	// Debug: garantir que não há cores muito escuras que possam parecer transparentes
	if (length(result) < 0.05) {
		color = vec4(0.1, 0.1, 0.1, 1.0); // Cor mínima visível
	}

})";

// Funcao MAIN
int main()
{
	// Inicializacao da GLFW
	glfwInit();

	// Configuracao OpenGL (descomente se necessario para sua placa)
	/*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);*/

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criacao da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Luzes: 1-3 | TAB: Objeto | TRC: Modo | WASD: Move | ESPACO: Camera/Objeto | F: Animacao", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Registro do callback de teclado
	glfwSetKeyCallback(window, key_callback);

	// Inicializar GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Informacoes de versao OpenGL
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Configurar viewport
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilar shaders
	GLuint shaderID = setupShader();
	
	// Configurar OpenGL
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);
	glClearDepth(1.0f);
	glDepthMask(GL_TRUE);

	// Carregar configuracao da cena
	bool configLoaded = loadSceneConfig("../src/GrauB2Config", sceneConfig);
	if (!configLoaded) {
		configLoaded = loadSceneConfig("src/GrauB2Config", sceneConfig);
	}

	// Configurar câmera baseada na configuracao
	if (configLoaded) {
		camera.position = sceneConfig.camera.position;
		camera.target = sceneConfig.camera.target;
	}

	// Carregando objetos baseados na configuracao
	if (configLoaded && !sceneConfig.objects.empty()) {
		for (const auto& objConfig : sceneConfig.objects) {
			Model model;
			model.name = objConfig.filename;
			model.position = objConfig.position;
			model.rotation = objConfig.rotation;
			model.scale = objConfig.scale;
			
			// Tentar carregar material do arquivo .mtl primeiro
			std::string mtlFile = getMTLFilename(objConfig.filename);
			std::vector<Material> materials;
			
			if (loadMTL(mtlFile, materials)) {
				if (!loadOBJWithMaterials(objConfig.filename.c_str(), model, materials)) {
					continue;
				}
			} else {
				// Fallback: usar loader antigo e criar material unico
				std::vector<glm::vec3> vertices, normals;
				std::vector<glm::vec2> uvs;
				
				if (loadOBJ(objConfig.filename.c_str(), vertices, uvs, normals)) {
					ModelPart part;
					part.vertices = vertices;
					part.uvs = uvs;
					part.normals = normals;
					part.nVertices = vertices.size();
					part.materialName = objConfig.materialType;
					part.material = createMaterial(objConfig.materialType);
					part.VAO = createModelPartVAO(part);
					
					model.parts.push_back(part);
				} else {
					continue;
				}
			}
			
			models.push_back(model);
			objectAnimations.push_back(objConfig.animation);
		}
	}

	std::cout << "Objetos carregados: " << models.size() << std::endl;
	std::cout << "Controles: TAB=selecionar | TRC=modo | WASD=mover | 123=luzes | F=animacao" << std::endl;


	
	// Configurar luzes baseado na configuracao
	vec3 lightPos, lightPos2, lightPos3;
	if (configLoaded && sceneConfig.lights.size() >= 3) {
		lightPos = sceneConfig.lights[0].position;
		lightPos2 = sceneConfig.lights[1].position;
		lightPos3 = sceneConfig.lights[2].position;
	} else {
		lightPos = vec3(2.0, 3.0, 4.0);
		lightPos2 = vec3(-2.0, 1.0, 2.0);
		lightPos3 = vec3(0.0, 1.0, -3.0);
	}
	
	vec3 camPos = camera.position;


	glUseProgram(shaderID);

	// Enviar a informacao de qual variavel armazenara o buffer da textura
	glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

	// Configuracoes iniciais
	glUniform3f(glGetUniformLocation(shaderID, "camPos"), camPos.x,camPos.y,camPos.z);
	glActiveTexture(GL_TEXTURE0);
	

	// Matriz de projecao perspectiva
	float fov = (configLoaded) ? sceneConfig.camera.fov : 45.0f;
	mat4 projection = perspective(radians(fov), (float)WIDTH / (float)HEIGHT, 0.01f, 2000.0f);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	// Loop principal
	float lastFrame = 0.0f;
	while (!glfwWindowShouldClose(window))
	{
		// Calcular deltaTime para animacões suaves
		float currentFrame = glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Processar eventos de input
		glfwPollEvents();

		// Atualizar animacões se estiverem ativadas
		if (animationEnabled) {
			updateAnimations(deltaTime);
			applyAnimationsToModels();
		}

		// Atualiza a matriz de view da câmera
		updateCameraMatrix(shaderID);

		// Posicões das luzes (sempre fixas)
		glUniform3f(glGetUniformLocation(shaderID, "lightPos1"), lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(glGetUniformLocation(shaderID, "lightPos2"), lightPos2.x, lightPos2.y, lightPos2.z);
		glUniform3f(glGetUniformLocation(shaderID, "lightPos3"), lightPos3.x, lightPos3.y, lightPos3.z);

		// Intensidades das luzes (com animacao se ativada)
		if (animationEnabled) {
			// Luzes piscando com intensidades variaveis
			vec3 light1Intensity = calculateLightIntensity(0, animationTime);
			vec3 light2Intensity = calculateLightIntensity(1, animationTime);
			vec3 light3Intensity = calculateLightIntensity(2, animationTime);
			
			glUniform1f(glGetUniformLocation(shaderID, "lightIntensity1"), light1Intensity.x);
			glUniform1f(glGetUniformLocation(shaderID, "lightIntensity2"), light2Intensity.x);
			glUniform1f(glGetUniformLocation(shaderID, "lightIntensity3"), light3Intensity.x);
		} else {
			// Intensidades normais quando animacao esta desativada
			glUniform1f(glGetUniformLocation(shaderID, "lightIntensity1"), 1.0f);
			glUniform1f(glGetUniformLocation(shaderID, "lightIntensity2"), 1.0f);
			glUniform1f(glGetUniformLocation(shaderID, "lightIntensity3"), 1.0f);
		}

		// Atualiza os uniformes das luzes no shader
		glUniform1i(glGetUniformLocation(shaderID, "light1Enabled"), light1Enabled);
		glUniform1i(glGetUniformLocation(shaderID, "light2Enabled"), light2Enabled);
		glUniform1i(glGetUniformLocation(shaderID, "light3Enabled"), light3Enabled);

		// Limpar buffers
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Estado OpenGL
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);



		// Desenha todos os modelos
		for (size_t i = 0; i < models.size(); i++) {
			// Destaca o modelo selecionado
			if (i == selectedModelIndex) {
				glLineWidth(3.0f);
				glPointSize(8.0f);
			} else {
				glLineWidth(1.0f);
				glPointSize(1.0f);
			}
			
			drawModel(shaderID, models[i]);
		}

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Cleanup
	for (Model& model : models) {
		for (ModelPart& part : model.parts) {
			glDeleteVertexArrays(1, &part.VAO);
		}
	}
	glfwTerminate();
	return 0;
}

// Callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	
	// Alternar entre controle de câmera e objetos
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		cameraControlMode = !cameraControlMode;
		if (cameraControlMode) {
			std::cout << "=== MODO CAMERA ATIVO === (WASD: mover, QE: subir/descer)" << std::endl;
		} else {
			std::cout << "=== MODO OBJETO ATIVO === (TAB: selecionar, TRC: modo)" << std::endl;
		}
	}
	
	// Controle de animacao
	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		animationEnabled = !animationEnabled;
		if (animationEnabled) {
			std::cout << "ANIMACAO ATIVADA!" << std::endl;
		} else {
			std::cout << "ANIMACAO DESATIVADA" << std::endl;
		}
	}
	
	// Selecao de objetos (apenas no modo objeto)
	if (!cameraControlMode && key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		selectedModelIndex = (selectedModelIndex + 1) % models.size();
		std::cout << "Modelo selecionado: " << selectedModelIndex << std::endl;
	}
	
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
		
		// Modos de transformacao (apenas no modo objeto)
		else if (!cameraControlMode && key == GLFW_KEY_T) {
			isTranslating = true;
			isScaling = false;
			isRotating = false;
			currentMode = TRANSLATE;
			std::cout << "Modo: Translacao" << std::endl;
		}
		else if (!cameraControlMode && key == GLFW_KEY_C) {
			isTranslating = false;
			isScaling = true;
			isRotating = false;
			currentMode = SCALE;
			std::cout << "Modo: Escala" << std::endl;
		}
		else if (!cameraControlMode && key == GLFW_KEY_R) {
			isTranslating = false;
			isScaling = false;
			isRotating = true;
			currentMode = ROTATE;
			std::cout << "Modo: Rotacao" << std::endl;
		}
	}
	
	// Controle da câmera ou transformacões do modelo selecionado
	if ((action == GLFW_PRESS || action == GLFW_REPEAT)) {
		float deltaTime = 0.016f; // ~60 FPS
		
		if (cameraControlMode) {
			// Controle da câmera
			processCameraMovement(key, deltaTime);
		}
		else if (models.size() > 0) {
			// Transformacões do modelo selecionado
			Model& selectedModel = models[selectedModelIndex];
			float step = 0.1f;
			float rotationStep = 5.0f;
			
			if (isTranslating) {
				if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
					selectedModel.position.x -= step;
					std::cout << "Movendo para esquerda" << std::endl;
				}
				if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
					selectedModel.position.x += step;
					std::cout << "Movendo para direita" << std::endl;
				}
				if (key == GLFW_KEY_UP || key == GLFW_KEY_W) {
					selectedModel.position.y += step;
					std::cout << "Movendo para cima" << std::endl;
				}
				if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) {
					selectedModel.position.y -= step;
					std::cout << "Movendo para baixo" << std::endl;
				}
			}
			else if (isScaling) {
				if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
					selectedModel.scale -= vec3(step);
					std::cout << "Diminuindo escala" << std::endl;
				}
				if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
					selectedModel.scale += vec3(step);
					std::cout << "Aumentando escala" << std::endl;
				}
				if (key == GLFW_KEY_UP || key == GLFW_KEY_W) {
					selectedModel.scale += vec3(step);
					std::cout << "Aumentando escala" << std::endl;
				}
				if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) {
					selectedModel.scale -= vec3(step);
					std::cout << "Diminuindo escala" << std::endl;
				}
			}
			else if (isRotating) {
				if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
					selectedModel.rotation.y -= rotationStep;
					std::cout << "Rotacionando Y para esquerda" << std::endl;
				}
				if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
					selectedModel.rotation.y += rotationStep;
					std::cout << "Rotacionando Y para direita" << std::endl;
				}
				if (key == GLFW_KEY_UP || key == GLFW_KEY_W) {
					selectedModel.rotation.x += rotationStep;
					std::cout << "Rotacionando X para cima" << std::endl;
				}
				if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) {
					selectedModel.rotation.x -= rotationStep;
					std::cout << "Rotacionando X para baixo" << std::endl;
				}
			}
		}
	}
}

// Compila e linka os shaders
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilacao
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
	// Checando erros de compilacao
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

	// Carregamento da imagem usando a funcao stbi_load da biblioteca stb_image
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
	// Matriz de modelo: transformacões na geometria (objeto)
	mat4 model = mat4(1); // matriz identidade
	// Translacao
	model = translate(model, position);
	// Rotacao
	model = rotate(model, radians(angle), axis);
	// Escala
	model = scale(model, dimensions);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	//glUniform4f(glGetUniformLocation(shaderID, "inputColor"), color.r, color.g, color.b, 1.0f); // enviando cor para variavel uniform inputColor
																								//  Chamada de desenho - drawcall
																								//  Poligono Preenchido - GL_TRIANGLES
	glDrawArrays(GL_TRIANGLES, 0, nVertices);
}

GLuint createSimpleVAO(const std::vector<vec3>& vertices, const std::vector<vec2>& uvs, const std::vector<vec3>& normals) {
    // Buffer simples - apenas posicao, cor fixa, normal basica e UV padrao
    std::vector<GLfloat> vBuffer;
    vec3 color = vec3(1.0f, 0.5f, 0.0f); // Laranja
    
    for (size_t i = 0; i < vertices.size(); i++) {
        // Posicao
        vBuffer.push_back(vertices[i].x);
        vBuffer.push_back(vertices[i].y);
        vBuffer.push_back(vertices[i].z);
        
        // Cor fixa
        vBuffer.push_back(color.r);
        vBuffer.push_back(color.g);
        vBuffer.push_back(color.b);
        
        // Normal basica (apontando pra frente)
        vBuffer.push_back(normals[i].x);
        vBuffer.push_back(normals[i].y);
        vBuffer.push_back(normals[i].z);
        
        // UV padrao
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

GLuint createModelVAO(const Model& model) {
    // Esta função agora é obsoleta, mas mantida para compatibilidade
    // Retorna VAO da primeira parte se existir
    if (!model.parts.empty()) {
        return model.parts[0].VAO;
    }
    return 0;
}

void drawModel(GLuint shaderID, const Model& model) {
    // Matriz de modelo: transformacões na geometria (objeto)
    mat4 modelMatrix = mat4(1); // matriz identidade
    
    // Aplicar transformacões
    modelMatrix = translate(modelMatrix, model.position);
    modelMatrix = rotate(modelMatrix, radians(model.rotation.x), vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = rotate(modelMatrix, radians(model.rotation.y), vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = rotate(modelMatrix, radians(model.rotation.z), vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = scale(modelMatrix, model.scale);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(modelMatrix));

    // Desenhar cada parte do modelo com seu proprio material
    for (size_t partIndex = 0; partIndex < model.parts.size(); partIndex++) {
        const auto& part = model.parts[partIndex];
        
        // Garantir que o estado do OpenGL está limpo para esta parte
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        
        // Enviar propriedades do material específicas desta parte
        glUniform3fv(glGetUniformLocation(shaderID, "materialKa"), 1, value_ptr(part.material.ka));
        glUniform3fv(glGetUniformLocation(shaderID, "materialKd"), 1, value_ptr(part.material.kd));
        glUniform3fv(glGetUniformLocation(shaderID, "materialKs"), 1, value_ptr(part.material.ks));
        glUniform1f(glGetUniformLocation(shaderID, "materialShininess"), part.material.shininess);
        
        // Indicar se é material de placa/decal
        int isDecal = (part.materialName == "Decals") ? 1 : 0;
        glUniform1i(glGetUniformLocation(shaderID, "isDecalMaterial"), isDecal);

        // Aplicar textura específica desta parte
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0); // Limpar qualquer textura anterior
        
        if (part.material.textureID != 0) {
            glBindTexture(GL_TEXTURE_2D, part.material.textureID);
        } else {
            // Criar texturas específicas baseadas no material
            static std::map<std::string, GLuint> textureCache;
            
            if (textureCache.find(part.materialName) == textureCache.end()) {
                // Criar nova textura específica para este material
                unsigned char colorPixel[4];
                
                if (part.materialName == "Asphalt") {
                    colorPixel[0] = 60; colorPixel[1] = 60; colorPixel[2] = 60; colorPixel[3] = 255; // Cinza escuro
                } else if (part.materialName == "Ground") {
                    colorPixel[0] = 120; colorPixel[1] = 80; colorPixel[2] = 40; colorPixel[3] = 255; // Marrom terra
                } else if (part.materialName == "Decals") {
                    colorPixel[0] = 255; colorPixel[1] = 255; colorPixel[2] = 255; colorPixel[3] = 255; // Branco para marcações
                } else if (part.materialName == "Leafs_Mat") {
                    colorPixel[0] = 50; colorPixel[1] = 120; colorPixel[2] = 30; colorPixel[3] = 255; // Verde para folhagem
                } else if (part.materialName == "Metal") {
                    colorPixel[0] = 180; colorPixel[1] = 180; colorPixel[2] = 180; colorPixel[3] = 255; // Prata para metal
                } else if (part.materialName == "Front_End") {
                    colorPixel[0] = 30; colorPixel[1] = 80; colorPixel[2] = 200; colorPixel[3] = 255; // Azul para moto
                } else if (part.materialName.find("Tire") != std::string::npos) {
                    colorPixel[0] = 30; colorPixel[1] = 30; colorPixel[2] = 30; colorPixel[3] = 255; // Preto para pneus
                } else if (part.materialName.find("Brake") != std::string::npos) {
                    colorPixel[0] = 100; colorPixel[1] = 100; colorPixel[2] = 120; colorPixel[3] = 255; // Cinza azulado para freios
                } else if (part.materialName.find("Chain") != std::string::npos) {
                    colorPixel[0] = 120; colorPixel[1] = 120; colorPixel[2] = 100; colorPixel[3] = 255; // Dourado para corrente
                } else {
                    colorPixel[0] = 160; colorPixel[1] = 160; colorPixel[2] = 160; colorPixel[3] = 255; // Cinza claro padrão
                }
                
                GLuint newTexture;
                glGenTextures(1, &newTexture);
                glBindTexture(GL_TEXTURE_2D, newTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, colorPixel);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                
                textureCache[part.materialName] = newTexture;
                

            }
            
            glBindTexture(GL_TEXTURE_2D, textureCache[part.materialName]);
        }

        // Desenhar esta parte
        if (part.VAO != 0 && part.nVertices > 0) {
            glBindVertexArray(part.VAO);
            glDrawArrays(GL_TRIANGLES, 0, part.nVertices);
            glBindVertexArray(0);
        }
        
        // Desbindar a textura para evitar vazamento entre partes
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void updateCameraMatrix(GLuint shaderID) {
    // Criar e enviar a matriz de view atualizada
    mat4 view = lookAt(camera.position, camera.target, camera.up);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));
}

void processCameraMovement(int key, float deltaTime) {
    float velocity = camera.speed * deltaTime;
    
    // Calcular direcões da câmera
    vec3 forward = normalize(camera.target - camera.position);
    vec3 right = normalize(cross(forward, camera.up));
    
    // Movimentos da câmera
    if (key == GLFW_KEY_W) {
        camera.position += forward * velocity;
        camera.target += forward * velocity;
        std::cout << "Câmera: frente" << std::endl;
    }
    if (key == GLFW_KEY_S) {
        camera.position -= forward * velocity;
        camera.target -= forward * velocity;
        std::cout << "Câmera: tras" << std::endl;
    }
    if (key == GLFW_KEY_A) {
        camera.position -= right * velocity;
        camera.target -= right * velocity;
        std::cout << "Câmera: esquerda" << std::endl;
    }
    if (key == GLFW_KEY_D) {
        camera.position += right * velocity;
        camera.target += right * velocity;
        std::cout << "Câmera: direita" << std::endl;
    }
    if (key == GLFW_KEY_Q) {
        camera.position -= camera.up * velocity;
        camera.target -= camera.up * velocity;
        std::cout << "Câmera: descer" << std::endl;
    }
    if (key == GLFW_KEY_E) {
        camera.position += camera.up * velocity;
        camera.target += camera.up * velocity;
        std::cout << "Câmera: subir" << std::endl;
    }
}

bool loadSceneConfig(const std::string& filename, SceneConfig& config) {
    
    // Try different paths
    std::vector<std::string> pathsToTry = {
        filename,                           // Original path
        "./" + filename,                    // Explicit current dir
        ".\\" + filename,                   // Windows style
        filename.substr(4),                 // Remove "src/" prefix
        "GrauB2Config"                      // Just filename
    };
    
    std::ifstream file;
    bool fileOpened = false;
    std::string workingPath;
    
    for (const auto& path : pathsToTry) {
        file.open(path, std::ios::in);
        if (file.is_open()) {
            workingPath = path;
            fileOpened = true;
            break;
        }
        file.clear(); // Clear error flags
    }
    
    if (!fileOpened) {
        return false;
    }
    
    std::string line;
    
    ObjectAnimation pendingAnimation; // Animacao para o próximo objeto
    bool hasAnimation = false;
    
    while (std::getline(file, line)) {
        // Ignorar comentarios e linhas vazias
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "CAMERA") {
            iss >> config.camera.position.x >> config.camera.position.y >> config.camera.position.z
                >> config.camera.target.x >> config.camera.target.y >> config.camera.target.z
                >> config.camera.fov;
            std::cout << "Câmera configurada: pos(" << config.camera.position.x << "," << config.camera.position.y << "," << config.camera.position.z << ")" << std::endl;
        }
        else if (command == "LIGHT1" || command == "LIGHT2" || command == "LIGHT3") {
            SceneConfig::LightConfig light;
            iss >> light.position.x >> light.position.y >> light.position.z >> light.intensity;
            config.lights.push_back(light);
            std::cout << "Luz " << config.lights.size() << " configurada: pos(" << light.position.x << "," << light.position.y << "," << light.position.z << ") intensidade=" << light.intensity << std::endl;
        }
        else if (command == "ANIMATION") {
            // Parsear animacao para aplicar ao proximo objeto
            if (parseAnimationConfig(line, pendingAnimation)) {
                hasAnimation = true;
            }
        }
        else if (command == "OBJECT") {
            SceneConfig::ObjectConfig obj;
            
            // Parse manual para lidar com nomes de arquivo com espaços
            std::string remainingLine;
            std::getline(iss, remainingLine);
            
            // Encontrar onde começam os parâmetros numéricos (posição x y z)
            // Procurar por padrão: espaço + número + espaço + número + espaço + número
            std::regex paramPattern(R"(\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?))");
            std::smatch match;
            
            if (std::regex_search(remainingLine, match, paramPattern)) {
                // Extrair filename (tudo antes dos parâmetros)
                size_t paramStart = match.position();
                obj.filename = remainingLine.substr(0, paramStart);
                
                // Remover espaços do final do filename
                obj.filename.erase(obj.filename.find_last_not_of(" \t") + 1);
                // Remover espaço do início se houver
                if (!obj.filename.empty() && obj.filename[0] == ' ') {
                    obj.filename = obj.filename.substr(1);
                }
                
                // Parse do resto da linha (números)
                std::string numbers = remainingLine.substr(paramStart);
                std::istringstream numberStream(numbers);
                numberStream >> obj.position.x >> obj.position.y >> obj.position.z
                            >> obj.rotation.x >> obj.rotation.y >> obj.rotation.z
                            >> obj.scale.x >> obj.scale.y >> obj.scale.z >> obj.materialType;
            } else {
                // Fallback: tentar split por último espaço antes de números
                std::vector<std::string> tokens;
                std::istringstream tokenStream(remainingLine);
                std::string token;
                while (tokenStream >> token) {
                    tokens.push_back(token);
                }
                
                // Assumir que os últimos 10 tokens são os parâmetros numéricos + material
                if (tokens.size() >= 10) {
                    // Reconstruir filename dos primeiros tokens
                    obj.filename = "";
                    for (size_t i = 0; i < tokens.size() - 10; i++) {
                        if (i > 0) obj.filename += " ";
                        obj.filename += tokens[i];
                    }
                    
                    // Parse dos últimos 10 tokens
                    size_t paramStart = tokens.size() - 10;
                    obj.position.x = std::stof(tokens[paramStart]);
                    obj.position.y = std::stof(tokens[paramStart + 1]);
                    obj.position.z = std::stof(tokens[paramStart + 2]);
                    obj.rotation.x = std::stof(tokens[paramStart + 3]);
                    obj.rotation.y = std::stof(tokens[paramStart + 4]);
                    obj.rotation.z = std::stof(tokens[paramStart + 5]);
                    obj.scale.x = std::stof(tokens[paramStart + 6]);
                    obj.scale.y = std::stof(tokens[paramStart + 7]);
                    obj.scale.z = std::stof(tokens[paramStart + 8]);
                    obj.materialType = tokens[paramStart + 9];
                } else {
                    // Último fallback
                    std::istringstream fallbackStream(remainingLine);
                    fallbackStream >> obj.filename >> obj.position.x >> obj.position.y >> obj.position.z
                                  >> obj.rotation.x >> obj.rotation.y >> obj.rotation.z
                                  >> obj.scale.x >> obj.scale.y >> obj.scale.z >> obj.materialType;
                }
            }
            
            // Aplicar animacao pendente se houver
            if (hasAnimation) {
                obj.animation = pendingAnimation;
                obj.animation.originalPosition = obj.position;
                hasAnimation = false;
            } else {
                obj.animation.type = ANIM_NONE;
            }
            
            config.objects.push_back(obj);
        }
    }
    
    file.close();
    return true;
}

Material createMaterial(const std::string& materialType) {
    Material material;
    material.name = materialType; // Definir o nome do material
    
    if (materialType == "MATTE") {
        // Material fosco (cerâmica/borracha)
        material.ka = glm::vec3(0.3f, 0.1f, 0.1f);
        material.kd = glm::vec3(0.9f, 0.4f, 0.3f);
        material.ks = glm::vec3(0.1f, 0.1f, 0.1f);
        material.shininess = 2.0f;
    }
    else if (materialType == "SHINY") {
        // Material brilhante (metal polido)
        material.ka = glm::vec3(0.1f, 0.2f, 0.3f);
        material.kd = glm::vec3(0.3f, 0.5f, 0.7f);
        material.ks = glm::vec3(0.9f, 0.9f, 0.9f);
        material.shininess = 128.0f;
    }
    else {
        // Material padrao
        material.ka = glm::vec3(0.2f, 0.2f, 0.2f);
        material.kd = glm::vec3(0.8f, 0.8f, 0.8f);
        material.ks = glm::vec3(0.5f, 0.5f, 0.5f);
        material.shininess = 32.0f;
    }
    
    return material;
}

void updateAnimations(float deltaTime) {
    // Atualizar tempo de animacao global
    animationTime += deltaTime;
}

void applyAnimationsToModels() {
    // Aplicar animacões a todos os modelos
    for (size_t i = 0; i < models.size() && i < objectAnimations.size(); i++) {
        ObjectAnimation& anim = objectAnimations[i];
        
        // Aplicar curvas paramétricas se configuradas
        switch (anim.type) {
            case ANIM_BEZIER:
            {
                glm::vec3 newPos = calculateBezierPosition(anim.bezier, animationTime);
                models[i].position = newPos;
                
                // Aplicar rotação automática apenas para animação Bézier simples
                models[i].rotation.y = animationTime * (30.0f + i * 15.0f); // Graus
                models[i].rotation.x = sin(animationTime * 2.0f + i) * 15.0f;
                models[i].rotation.y = fmod(models[i].rotation.y, 360.0f);
                break;
            }
            
            case ANIM_BEZIER_TRACK:
            {
                glm::vec3 direction;
                glm::vec3 newPos = calculateBezierTrackPosition(anim.bezierTrack, animationTime, direction);
                
                // Manter altura constante da moto
                newPos.y = 0.8f;
                models[i].position = newPos;
                
                // Orientar a moto apenas no eixo Y (direção horizontal)
                if (length(direction) > 0.001f) {
                    // Calcular apenas ângulo Y para direção horizontal
                    float angleY = atan2(direction.x, direction.z) * 180.0f / M_PI;
                    // Manter a moto deitada com rodas no chão
                    models[i].rotation.x = -90.0f;   // Moto deitada (rodas tocam o chão)
                    models[i].rotation.y = angleY;   // Direção do movimento
                    models[i].rotation.z = 0.0f;     // Sem rotação lateral
                }
                break;
            }
            
            case ANIM_ORBIT:
            {
                glm::vec3 newPos = calculateOrbitPosition(anim.orbit, animationTime);
                models[i].position = newPos;
                
                // Aplicar rotação automática para animação orbital
                models[i].rotation.y = animationTime * (30.0f + i * 15.0f); // Graus
                models[i].rotation.x = sin(animationTime * 2.0f + i) * 15.0f;
                models[i].rotation.y = fmod(models[i].rotation.y, 360.0f);
                break;
            }
            
            case ANIM_LINEAR:
            {
                // Implementar movimento linear simples se necessario
                // Por enquanto, nao implementado
                break;
            }
            
            case ANIM_NONE:
            default:
                // Objetos estáticos - NÃO aplicar nenhuma rotação ou movimento
                // Manter posição e rotação original (pista deve ficar parada)
                break;
        }
    }
}

vec3 calculateLightIntensity(int lightIndex, float time) {
    // Cada luz pisca em frequências diferentes
    float frequency1 = 2.0f + lightIndex * 0.5f; // Hz
    float frequency2 = 1.5f + lightIndex * 0.3f; // Hz
    
    // Usar funcões senoidais para criar efeito de piscada suave
    float intensity1 = (sin(time * frequency1 * 2.0f * M_PI) + 1.0f) * 0.5f; // 0.0 a 1.0
    float intensity2 = (cos(time * frequency2 * 2.0f * M_PI) + 1.0f) * 0.5f; // 0.0 a 1.0
    
    // Combinar as intensidades para efeito mais complexo
    float finalIntensity = (intensity1 * 0.7f + intensity2 * 0.3f);
    
    // Garantir que a intensidade nao fique muito baixa (mínimo 0.3)
    finalIntensity = 0.3f + (finalIntensity * 0.7f);
    
    return vec3(finalIntensity, finalIntensity, finalIntensity);
}

bool loadMTL(const std::string& filename, std::vector<Material>& materials) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Arquivo MTL nao encontrado: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    Material currentMaterial;
    bool hasMaterial = false;
    bool hasKd = false; // Track if Kd was explicitly set
    
    std::cout << "Carregando materiais de: " << filename << std::endl;
    
    while (std::getline(file, line)) {
        // Ignorar comentarios e linhas vazias
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "newmtl") {
            // Se ja temos um material, salvar o anterior
            if (hasMaterial) {
                // Apply smart defaults if Kd wasn't explicitly set
                if (!hasKd) {
                    if (currentMaterial.name == "Asphalt") {
                        currentMaterial.kd = glm::vec3(0.3f, 0.3f, 0.3f); // Dark gray for asphalt
                    } else if (currentMaterial.name == "Decals") {
                        currentMaterial.kd = glm::vec3(1.0f, 1.0f, 1.0f); // White for signs (rely on texture)
                    } else if (currentMaterial.name == "Front_End") {
                        currentMaterial.kd = glm::vec3(0.1f, 0.3f, 0.8f); // Blue for motorcycle
                    } else if (currentMaterial.name.find("Metal") != std::string::npos || 
                              currentMaterial.name.find("Brake") != std::string::npos ||
                              currentMaterial.name.find("Chain") != std::string::npos) {
                        currentMaterial.kd = glm::vec3(0.7f, 0.7f, 0.7f); // Light gray for metal
                    } else if (currentMaterial.name.find("Tire") != std::string::npos) {
                        currentMaterial.kd = glm::vec3(0.1f, 0.1f, 0.1f); // Dark for tires
                    } else if (currentMaterial.name == "Ground") {
                        currentMaterial.kd = glm::vec3(0.4f, 0.3f, 0.2f); // Brown for ground
                    } else if (currentMaterial.name.find("Light") != std::string::npos) {
                        currentMaterial.kd = glm::vec3(0.9f, 0.9f, 0.8f); // Light color for lights
                    } else {
                        // Keep default kd for unknown materials
                    }
                }
                
                // Ajustar propriedades específicas para placas
                if (currentMaterial.name == "Decals") {
                    currentMaterial.ka = glm::vec3(0.6f, 0.6f, 0.6f); // Alto ambiente para visibilidade
                    currentMaterial.ks = glm::vec3(0.1f, 0.1f, 0.1f); // Baixo especular 
                    currentMaterial.shininess = 1.0f;                  // Sem brilho
                }
                materials.push_back(currentMaterial);
            }
            
            // Comecar novo material
            currentMaterial = Material(); // Reset
            iss >> currentMaterial.name;
            hasMaterial = true;
            hasKd = false; // Reset Kd flag
            std::cout << "Material encontrado: " << currentMaterial.name << std::endl;
        }
        else if (command == "Ka") {
            iss >> currentMaterial.ka.x >> currentMaterial.ka.y >> currentMaterial.ka.z;
        }
        else if (command == "Kd") {
            iss >> currentMaterial.kd.x >> currentMaterial.kd.y >> currentMaterial.kd.z;
            hasKd = true; // Mark that Kd was explicitly set
        }
        else if (command == "Ks") {
            iss >> currentMaterial.ks.x >> currentMaterial.ks.y >> currentMaterial.ks.z;
        }
        else if (command == "Ns") {
            iss >> currentMaterial.shininess;
        }
        else if (command == "map_Kd") {
            iss >> currentMaterial.diffuseTexture;
        }
        // Ignorar outros comandos por enquanto (illum, etc.)
    }
    
    // Salvar o último material
    if (hasMaterial) {
        // Apply smart defaults if Kd wasn't explicitly set for the last material
        if (!hasKd) {
            if (currentMaterial.name == "Asphalt") {
                currentMaterial.kd = glm::vec3(0.3f, 0.3f, 0.3f); // Dark gray for asphalt
            } else if (currentMaterial.name == "Decals") {
                currentMaterial.kd = glm::vec3(1.0f, 1.0f, 1.0f); // White for signs (rely on texture)
            } else if (currentMaterial.name == "Front_End") {
                currentMaterial.kd = glm::vec3(0.1f, 0.3f, 0.8f); // Blue for motorcycle
            } else if (currentMaterial.name.find("Metal") != std::string::npos || 
                      currentMaterial.name.find("Brake") != std::string::npos ||
                      currentMaterial.name.find("Chain") != std::string::npos) {
                currentMaterial.kd = glm::vec3(0.7f, 0.7f, 0.7f); // Light gray for metal
            } else if (currentMaterial.name.find("Tire") != std::string::npos) {
                currentMaterial.kd = glm::vec3(0.1f, 0.1f, 0.1f); // Dark for tires
            } else if (currentMaterial.name == "Ground") {
                currentMaterial.kd = glm::vec3(0.4f, 0.3f, 0.2f); // Brown for ground
            } else if (currentMaterial.name.find("Light") != std::string::npos) {
                currentMaterial.kd = glm::vec3(0.9f, 0.9f, 0.8f); // Light color for lights
            }
        }
        
        // Ajustar propriedades específicas para placas
        if (currentMaterial.name == "Decals") {
            currentMaterial.ka = glm::vec3(0.6f, 0.6f, 0.6f); // Alto ambiente para visibilidade
            currentMaterial.ks = glm::vec3(0.1f, 0.1f, 0.1f); // Baixo especular 
            currentMaterial.shininess = 1.0f;                  // Sem brilho
        }
        materials.push_back(currentMaterial);
    }
    
    file.close();
    std::cout << "Carregados " << materials.size() << " materiais de " << filename << std::endl;
    
    // Carregar texturas dos materiais
    if (materials.size() > 0) {
        loadMaterialTextures(materials, filename);
    }
    
    return materials.size() > 0;
}

Material findMaterialByName(const std::vector<Material>& materials, const std::string& name) {
    for (const auto& material : materials) {
        if (material.name == name) {
            return material;
        }
    }
    
    // Criar material especifico baseado no nome se nao encontrado
    
    Material defaultMaterial;
    defaultMaterial.name = name;
    
    // Cores específicas baseadas no nome do material solicitado
    if (name == "Front_End") {
        defaultMaterial.kd = glm::vec3(0.1f, 0.3f, 0.8f); // Azul para moto
        defaultMaterial.ks = glm::vec3(0.8f, 0.8f, 0.8f); // Especular brilhante
        defaultMaterial.shininess = 64.0f;
    } else if (name == "Asphalt") {
        defaultMaterial.kd = glm::vec3(0.3f, 0.3f, 0.3f); // Cinza escuro para asfalto
        defaultMaterial.ks = glm::vec3(0.2f, 0.2f, 0.2f); // Pouco especular
        defaultMaterial.shininess = 8.0f;
    } else {
        // Material padrão genérico
        defaultMaterial.kd = glm::vec3(0.7f, 0.7f, 0.7f);
        defaultMaterial.ks = glm::vec3(0.5f, 0.5f, 0.5f);
        defaultMaterial.shininess = 32.0f;
    }
    
    return defaultMaterial;
}

std::string getMTLFilename(const std::string& objFilename) {
    // Trocar extensao .obj por .mtl
    std::string mtlFile = objFilename;
    size_t pos = mtlFile.find_last_of('.');
    if (pos != std::string::npos) {
        mtlFile = mtlFile.substr(0, pos) + ".mtl";
    } else {
        mtlFile += ".mtl";
    }
    return mtlFile;
}

glm::vec3 calculateBezierPosition(const BezierCurve& curve, float t) {
    // Normalizar t para o intervalo [0, 1]
    if (curve.loop) {
        t = fmod(t, curve.duration) / curve.duration;
    } else {
        t = glm::clamp(t / curve.duration, 0.0f, 1.0f);
    }
    
    // Fórmula da curva de Bézier cúbica: B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    glm::vec3 point = uuu * curve.p0;           // (1-t)³P₀
    point += 3.0f * uu * t * curve.p1;          // 3(1-t)²tP₁
    point += 3.0f * u * tt * curve.p2;          // 3(1-t)t²P₂
    point += ttt * curve.p3;                    // t³P₃
    
    return point;
}

glm::vec3 calculateBezierTrackPosition(const BezierTrack& track, float t, glm::vec3& direction) {
    if (track.curves.empty()) {
        direction = glm::vec3(0.0f, 0.0f, 1.0f);
        return glm::vec3(0.0f);
    }
    
    // Normalizar t para o intervalo da duração total
    if (track.loop) {
        t = fmod(t, track.totalDuration);
    } else {
        t = glm::clamp(t, 0.0f, track.totalDuration);
    }
    
    // Encontrar qual curva usar baseado no tempo
    float accumulatedTime = 0.0f;
    for (size_t i = 0; i < track.curves.size(); i++) {
        const BezierCurve& curve = track.curves[i];
        if (t <= accumulatedTime + curve.duration) {
            // Esta é a curva atual
            float localT = (t - accumulatedTime) / curve.duration;
            
            // Calcular posição
            glm::vec3 position = calculateBezierPosition(curve, localT * curve.duration);
            
            // Calcular direção (derivada da curva de Bézier)
            float u = 1.0f - localT;
            float uu = u * u;
            float tt = localT * localT;
            
            direction = -3.0f * uu * curve.p0;                           // -3(1-t)²P₀
            direction += 3.0f * uu * curve.p1 - 6.0f * u * localT * curve.p1;  // 3(1-t)²P₁ - 6(1-t)tP₁
            direction += 6.0f * u * localT * curve.p2 - 3.0f * tt * curve.p2;  // 6(1-t)tP₂ - 3t²P₂
            direction += 3.0f * tt * curve.p3;                           // 3t²P₃
            
            direction = normalize(direction);
            return position;
        }
        accumulatedTime += curve.duration;
    }
    
    // Se chegou aqui, usar a última curva
    const BezierCurve& lastCurve = track.curves.back();
    direction = normalize(lastCurve.p3 - lastCurve.p2);
    return lastCurve.p3;
}

glm::vec3 calculateOrbitPosition(const OrbitAnimation& orbit, float time) {
    // Calcular ângulo baseado no tempo e velocidade
    float angle = time * orbit.speed * (M_PI / 180.0f); // Converter para radianos
    
    // Calcular posicao na órbita
    glm::vec3 position;
    
    // Órbita no plano XZ por padrao (eixo Y)
    if (orbit.axis.y > 0.9f) {
        position.x = orbit.center.x + orbit.radius * cos(angle);
        position.y = orbit.center.y;
        position.z = orbit.center.z + orbit.radius * sin(angle);
    }
    // Órbita no plano XY (eixo Z)
    else if (orbit.axis.z > 0.9f) {
        position.x = orbit.center.x + orbit.radius * cos(angle);
        position.y = orbit.center.y + orbit.radius * sin(angle);
        position.z = orbit.center.z;
    }
    // Órbita no plano YZ (eixo X)
    else {
        position.x = orbit.center.x;
        position.y = orbit.center.y + orbit.radius * cos(angle);
        position.z = orbit.center.z + orbit.radius * sin(angle);
    }
    
    return position;
}

bool parseAnimationConfig(const std::string& line, ObjectAnimation& animation) {
    std::istringstream iss(line);
    std::string command, type;
    iss >> command >> type;
    
    if (command != "ANIMATION") return false;
    
    if (type == "bezier") {
        animation.type = ANIM_BEZIER;
        iss >> animation.bezier.p0.x >> animation.bezier.p0.y >> animation.bezier.p0.z
            >> animation.bezier.p1.x >> animation.bezier.p1.y >> animation.bezier.p1.z
            >> animation.bezier.p2.x >> animation.bezier.p2.y >> animation.bezier.p2.z
            >> animation.bezier.p3.x >> animation.bezier.p3.y >> animation.bezier.p3.z
            >> animation.bezier.duration;
        animation.bezier.loop = true; // Padrao: loop ativo
        
        std::cout << "Animacao Bézier configurada: duracao=" << animation.bezier.duration << "s" << std::endl;
        return true;
    }
    else if (type == "track") {
        animation.type = ANIM_BEZIER_TRACK;
        
        // Criar pista de drift com pontos de controle realistas
        // Baseado no formato típico de uma pista de drift oval
        animation.bezierTrack.curves.clear();
        animation.bezierTrack.totalDuration = 0.0f;
        animation.bezierTrack.loop = true;
        
        // Trajeto seguindo a geometria real da pista de drift
        // Baseado em observação da pista: formato mais oval/retangular que circular
        
        // Segmento 1: Reta longa lateral direita
        BezierCurve curve1;
        curve1.p0 = glm::vec3(12.0f, 0.8f, -8.0f);     // Início - parte baixa direita
        curve1.p1 = glm::vec3(12.0f, 0.8f, -2.0f);     // Controle - subindo reta
        curve1.p2 = glm::vec3(12.0f, 0.8f, 2.0f);      // Controle - meio da reta
        curve1.p3 = glm::vec3(12.0f, 0.8f, 8.0f);      // Fim - parte alta direita
        curve1.duration = 4.0f;
        curve1.loop = false;
        animation.bezierTrack.curves.push_back(curve1);
        
        // Segmento 2: Curva superior conectando direita-esquerda
        BezierCurve curve2;
        curve2.p0 = glm::vec3(12.0f, 0.8f, 8.0f);      // Início - alta direita
        curve2.p1 = glm::vec3(8.0f, 0.8f, 12.0f);      // Controle - curvando para fora
        curve2.p2 = glm::vec3(-8.0f, 0.8f, 12.0f);     // Controle - meio da curva superior
        curve2.p3 = glm::vec3(-12.0f, 0.8f, 8.0f);     // Fim - alta esquerda
        curve2.duration = 4.0f;
        curve2.loop = false;
        animation.bezierTrack.curves.push_back(curve2);
        
        // Segmento 3: Reta longa lateral esquerda
        BezierCurve curve3;
        curve3.p0 = glm::vec3(-12.0f, 0.8f, 8.0f);     // Início - parte alta esquerda
        curve3.p1 = glm::vec3(-12.0f, 0.8f, 2.0f);     // Controle - descendo reta
        curve3.p2 = glm::vec3(-12.0f, 0.8f, -2.0f);    // Controle - meio da reta
        curve3.p3 = glm::vec3(-12.0f, 0.8f, -8.0f);    // Fim - parte baixa esquerda
        curve3.duration = 4.0f;
        curve3.loop = false;
        animation.bezierTrack.curves.push_back(curve3);
        
        // Segmento 4: Curva inferior conectando esquerda-direita
        BezierCurve curve4;
        curve4.p0 = glm::vec3(-12.0f, 0.8f, -8.0f);    // Início - baixa esquerda
        curve4.p1 = glm::vec3(-8.0f, 0.8f, -12.0f);    // Controle - curvando para fora
        curve4.p2 = glm::vec3(8.0f, 0.8f, -12.0f);     // Controle - meio da curva inferior
        curve4.p3 = glm::vec3(12.0f, 0.8f, -8.0f);     // Fim - baixa direita (volta ao início)
        curve4.duration = 4.0f;
        curve4.loop = false;
        animation.bezierTrack.curves.push_back(curve4);
        
        // Calcular duração total
        for (const auto& curve : animation.bezierTrack.curves) {
            animation.bezierTrack.totalDuration += curve.duration;
        }
        
        std::cout << "Animacao de pista configurada: " << animation.bezierTrack.curves.size() 
                  << " curvas, duracao total=" << animation.bezierTrack.totalDuration << "s" << std::endl;
        return true;
    }
    else if (type == "orbit") {
        animation.type = ANIM_ORBIT;
        iss >> animation.orbit.center.x >> animation.orbit.center.y >> animation.orbit.center.z
            >> animation.orbit.radius >> animation.orbit.speed;
        animation.orbit.axis = glm::vec3(0.0f, 1.0f, 0.0f); // Eixo Y padrao
        
        std::cout << "Animacao orbital configurada: centro=(" << animation.orbit.center.x 
                  << "," << animation.orbit.center.y << "," << animation.orbit.center.z 
                  << ") raio=" << animation.orbit.radius << " velocidade=" << animation.orbit.speed << "°/s" << std::endl;
        return true;
    }
    
    return false;
}

void loadMaterialTextures(std::vector<Material>& materials, const std::string& mtlFile) {
    
    // Extrair diretório base do arquivo MTL
    std::string mtlDir = mtlFile.substr(0, mtlFile.find_last_of("/\\"));
    
    for (auto& material : materials) {
        if (!material.diffuseTexture.empty()) {
            
            // Extrair nome base do arquivo (sem extensão)
            std::string textureName = material.diffuseTexture.substr(material.diffuseTexture.find_last_of("/\\") + 1);
            std::string baseName = textureName.substr(0, textureName.find_last_of('.'));
            
            // Extensões possíveis a tentar
            std::vector<std::string> extensions = {".jpeg", ".jpg", ".png", ".tga", ".bmp"};
            
            // Diretórios possíveis
            std::vector<std::string> baseDirs = {
                mtlDir,
                "../" + mtlDir,
                mtlDir + "/textures",
                "../" + mtlDir + "/textures"
            };
            
            bool found = false;
            
            // Primeiro tentar o nome exato conforme especificado
            for (const auto& baseDir : baseDirs) {
                std::string exactPath = baseDir + "/" + material.diffuseTexture;
                std::ifstream file(exactPath);
                if (file.good()) {
                    file.close();
                    int width, height;
                    material.textureID = loadTexture(exactPath, width, height);
                    if (material.textureID != 0) {
                        found = true;
                        break;
                    }
                }
            }
            
            // Se não encontrou, tentar diferentes extensões
            if (!found) {
                for (const auto& baseDir : baseDirs) {
                    for (const auto& ext : extensions) {
                        std::string altPath = baseDir + "/" + material.diffuseTexture.substr(0, material.diffuseTexture.find_last_of("/\\") + 1) + baseName + ext;
                        
                        std::ifstream file(altPath);
                        if (file.good()) {
                            file.close();
                            int width, height;
                            material.textureID = loadTexture(altPath, width, height);
                            if (material.textureID != 0) {
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found) break;
                }
            }
            
        }
    }
}