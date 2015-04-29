#include <stdio.h>
#include <stdlib.h>
#include "glew.h"
#include "glfw3.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <stdio.h>
#include "GLSL.h"
#include "tiny_obj_loader.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp" //perspective, trans etc
#include "glm/gtc/type_ptr.hpp" //value_ptr

#define SIZE 10.0
#define BORDER 8.0
#define ROTATE_SPEED 0.25
#define MOVEMENT_SPEED 0.05
#define SHEAR_SPEED 0.01
#define HEAD_SPEED 0.2
#define NUM_MAT 4
#define ANIM_LIGHT_SPD 0.02
#define NUM_LIGHT_PAT 3
#define NUM_OBJS 625
#define NUM_CUBES 72
#define NUM_OBJ_TYPES 4
   #define SPHERE 0
   #define BUNNY 1
   #define CUBE 2
   #define GRASS 3
#define GROUND_MAT -1
#define LIGHT_MAT -2
#define CUBE_MAT -3
#define SKY_MAT -4
#define GRASS_MAT -5
// IDs
#define DEFAULT_ID 0
#define GROUND_ID 1
#define SKY_ID 2

#define SHEAR_MAG 0.5

using namespace std;

static string objectFiles[] = {"sphere.obj", "bunny.obj", "cube.obj", "Grass_02.obj"};

struct transform_t {
   glm::vec3 trans;
   glm::vec3 scale;
   float rotX;
   float rotY;
   float rotZ;
   float shearX;
   float shearZ;
};
struct bufID_t {
   GLuint pos;
   GLuint ind;
   GLuint nor;
   GLuint tex;
};

// Window
GLFWwindow* window;
int g_width;
int g_height;
int animating = 1;
int frame = 0;
int drawGrass = 1;
// Camera
glm::vec3 g_view(7, 0, 7);
float theta = -3*M_PI/4.0;
float phi = 0;
// Lights
glm::vec3 g_light1(0, 0, 0);
glm::vec3 g_light2(0, 0, 0);
glm::vec3 g_light3(0, 0, 0);
// Model vectors
vector<tinyobj::shape_t> allShapes[NUM_OBJ_TYPES];
vector<tinyobj::material_t> allMaterials[NUM_OBJ_TYPES];
transform_t transforms[NUM_OBJ_TYPES][NUM_OBJS];
GLuint ShadeProg;
// Obj buffer ids
bufID_t bufIDs[NUM_OBJ_TYPES+1];
// Shader handlers
GLint h_aPos;
GLint h_aNor;
GLint h_uP;
GLint h_uV;
GLint h_uM;
GLint h_uR;
GLint h_uView;
GLint h_uL1, h_uL2, h_uL3;
GLint h_uAClr, h_uDClr, h_uSClr, h_uS;
GLint h_uID;
// Misc
int bunnyMat = 0;

/** UTILITY **/
float randF() {
   return (float)rand() / RAND_MAX;
}

inline void safe_glUniformMatrix4fv(const GLint handle, const GLfloat data[]) {
   if (handle >= 0)
      glUniformMatrix4fv(handle, 1, GL_FALSE, data);
}

/** INITIALIZING FOR DRAW **/
void resize_obj(std::vector<tinyobj::shape_t> &shapes) {
   float minX, minY, minZ;
   float maxX, maxY, maxZ;
   float scaleX, scaleY, scaleZ;
   float shiftX, shiftY, shiftZ;
   float epsilon = 0.001;

   minX = minY = minZ = 1.1754E+38F;
   maxX = maxY = maxZ = -1.1754E+38F;

   //Go through all vertices to determine min and max of each dimension
   for (size_t i = 0; i < shapes.size(); i++) {
      for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
         if(shapes[i].mesh.positions[3*v+0] < minX)
            minX = shapes[i].mesh.positions[3*v+0];
         if(shapes[i].mesh.positions[3*v+0] > maxX)
            maxX = shapes[i].mesh.positions[3*v+0];

         if(shapes[i].mesh.positions[3*v+1] < minY)
            minY = shapes[i].mesh.positions[3*v+1];
         if(shapes[i].mesh.positions[3*v+1] > maxY)
            maxY = shapes[i].mesh.positions[3*v+1];

         if(shapes[i].mesh.positions[3*v+2] < minZ)
            minZ = shapes[i].mesh.positions[3*v+2];
         if(shapes[i].mesh.positions[3*v+2] > maxZ)
            maxZ = shapes[i].mesh.positions[3*v+2];
      }
   }
   //From min and max compute necessary scale and shift for each dimension
   float maxExtent, xExtent, yExtent, zExtent;
   xExtent = maxX-minX;
   yExtent = maxY-minY;
   zExtent = maxZ-minZ;
   if (xExtent >= yExtent && xExtent >= zExtent) {
      maxExtent = xExtent;
   }
   if (yExtent >= xExtent && yExtent >= zExtent) {
      maxExtent = yExtent;
   }
   if (zExtent >= xExtent && zExtent >= yExtent) {
      maxExtent = zExtent;
   }
   scaleX = 2.0 /maxExtent;
   shiftX = minX + (xExtent/ 2.0);
   scaleY = 2.0 / maxExtent;
   shiftY = minY + (yExtent / 2.0);
   scaleZ = 2.0/ maxExtent;
   shiftZ = minZ + (zExtent)/2.0;

   // Go through all verticies shift and scale them
   for (size_t i = 0; i < shapes.size(); i++) {
      for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
         shapes[i].mesh.positions[3*v+0] = (shapes[i].mesh.positions[3*v+0] - shiftX) * scaleX;
         assert(shapes[i].mesh.positions[3*v+0] >= -1.0 - epsilon);
         assert(shapes[i].mesh.positions[3*v+0] <= 1.0 + epsilon);
         shapes[i].mesh.positions[3*v+1] = (shapes[i].mesh.positions[3*v+1] - shiftY) * scaleY;
         assert(shapes[i].mesh.positions[3*v+1] >= -1.0 - epsilon);
         assert(shapes[i].mesh.positions[3*v+1] <= 1.0 + epsilon);
         shapes[i].mesh.positions[3*v+2] = (shapes[i].mesh.positions[3*v+2] - shiftZ) * scaleZ;
         assert(shapes[i].mesh.positions[3*v+2] >= -1.0 - epsilon);
         assert(shapes[i].mesh.positions[3*v+2] <= 1.0 + epsilon);
      }
   }
}

void loadShapes(const string &objFile, std::vector<tinyobj::shape_t> &shapes, vector<tinyobj::material_t> &materials) {
   string err = tinyobj::LoadObj(shapes, materials, objFile.c_str());
   if(!err.empty()) {
      cerr << err << endl;
   }
   resize_obj(shapes);
}

bool installShaders(const string &vShaderName, const string &fShaderName) {
   GLint rc;

   // Create shader handles
   GLuint VS = glCreateShader(GL_VERTEX_SHADER);
   GLuint FS = glCreateShader(GL_FRAGMENT_SHADER);

   // Read shader sources
   const char *vshader = GLSL::textFileRead(vShaderName.c_str());
   const char *fshader = GLSL::textFileRead(fShaderName.c_str());
   glShaderSource(VS, 1, &vshader, NULL);
   glShaderSource(FS, 1, &fshader, NULL);

   // Compile vertex shader
   glCompileShader(VS);
   GLSL::printError();
   glGetShaderiv(VS, GL_COMPILE_STATUS, &rc);
   GLSL::printShaderInfoLog(VS);
   if(!rc) {
      printf("Error compiling vertex shader %s\n", vShaderName.c_str());
      return false;
   }

   // Compile fragment shader
   glCompileShader(FS);
   GLSL::printError();
   glGetShaderiv(FS, GL_COMPILE_STATUS, &rc);
   GLSL::printShaderInfoLog(FS);
   if(!rc) {
      printf("Error compiling fragment shader %s\n", fShaderName.c_str());
      return false;
   }

   // Create the program and link
   ShadeProg = glCreateProgram();
   glAttachShader(ShadeProg, VS);
   glAttachShader(ShadeProg, FS);
   glLinkProgram(ShadeProg);

   GLSL::printError();
   glGetProgramiv(ShadeProg, GL_LINK_STATUS, &rc);
   GLSL::printProgramInfoLog(ShadeProg);
   if(!rc) {
      printf("Error linking shaders %s and %s\n", vShaderName.c_str(), fShaderName.c_str());
      return false;
   }

   // Get handles to attribute data
   h_aPos = GLSL::getAttribLocation(ShadeProg, "aPos");
   h_aNor = GLSL::getAttribLocation(ShadeProg, "aNor");
   h_uP = GLSL::getUniformLocation(ShadeProg, "uP");
   h_uV = GLSL::getUniformLocation(ShadeProg, "uV");
   h_uM = GLSL::getUniformLocation(ShadeProg, "uM");
   h_uR = GLSL::getUniformLocation(ShadeProg, "uR");
   h_uView = GLSL::getUniformLocation(ShadeProg, "uView");
   h_uL1 = GLSL::getUniformLocation(ShadeProg, "uL1");
   h_uL2 = GLSL::getUniformLocation(ShadeProg, "uL2");
   h_uL3 = GLSL::getUniformLocation(ShadeProg, "uL3");
   h_uAClr = GLSL::getUniformLocation(ShadeProg, "uAClr");
   h_uDClr = GLSL::getUniformLocation(ShadeProg, "uDClr");
   h_uSClr = GLSL::getUniformLocation(ShadeProg, "uSClr");
   h_uS = GLSL::getUniformLocation(ShadeProg, "uS");
   h_uID = GLSL::getUniformLocation(ShadeProg, "uID");

   assert(glGetError() == GL_NO_ERROR);
   return true;
}

vector<float> computeNormals(vector<float> posBuf, vector<unsigned int> indBuf) {
   vector<float> norBuf;
   vector<glm::vec3> crossBuf;
   int idx1, idx2, idx3;
   glm::vec3 v1, v2, v3;
   glm::vec3 vec1, vec2, vec3;
   glm::vec3 cross1, cross2, cross3;

   // For every vertex initialize a normal to 0
   for (int j = 0; j < posBuf.size()/3; j++) {
      norBuf.push_back(0);
      norBuf.push_back(0);
      norBuf.push_back(0);

      crossBuf.push_back(glm::vec3(0, 0, 0));
   }

   // Compute normals for every face then add them to every associated vertex
   for (int i = 0; i < indBuf.size()/3; i++) {
      idx1 = indBuf[3*i+0];
      idx2 = indBuf[3*i+1];
      idx3 = indBuf[3*i+2];
      v1 = glm::vec3(posBuf[3*idx1 +0], posBuf[3*idx1 +1], posBuf[3*idx1 +2]); 
      v2 = glm::vec3(posBuf[3*idx2 +0], posBuf[3*idx2 +1], posBuf[3*idx2 +2]); 
      v3 = glm::vec3(posBuf[3*idx3 +0], posBuf[3*idx3 +1], posBuf[3*idx3 +2]); 

      vec1 = glm::normalize(v1 - v2);
      vec2 = glm::normalize(v2 - v3);
      vec3 = glm::normalize(v3 - v1);

      cross1 = glm::cross(vec1, vec2);
      cross2 = glm::cross(vec2, vec3);
      cross3 = glm::cross(vec3, vec1);

      crossBuf[idx1] += cross1;
      crossBuf[idx2] += cross2;
      crossBuf[idx3] += cross3;
   }

   // Cross products have been added together, normalize them and add to normal buffer
   for (int i = 0; i < indBuf.size()/3; i++) {
      idx1 = indBuf[3*i+0];
      idx2 = indBuf[3*i+1];
      idx3 = indBuf[3*i+2];

      cross1 = glm::normalize(crossBuf[idx1]);
      cross2 = glm::normalize(crossBuf[idx2]);
      cross3 = glm::normalize(crossBuf[idx3]);

      norBuf[3*idx1+0] = cross1.x;
      norBuf[3*idx1+1] = cross1.y;
      norBuf[3*idx1+2] = cross1.z;
      norBuf[3*idx2+0] = cross2.x;
      norBuf[3*idx2+1] = cross2.y;
      norBuf[3*idx2+2] = cross2.z;
      norBuf[3*idx3+0] = cross3.x;
      norBuf[3*idx3+1] = cross3.y;
      norBuf[3*idx3+2] = cross3.z;
   }

   return norBuf;
}

void cubeTransform(transform_t &transform, int idx) {
   // Produce a random transform
   float s = 0.45;
   float x = (idx%6) - (SIZE/4);
   float y = idx < 36 ? (s - 1.0) : (((idx-24)/6) - (SIZE/4));
   float z = idx < 36 ? ((idx/6) - (SIZE/4)) : -SIZE/4;
   float rotX = 0;
   float rotY = 0;
   float rotZ = 0;
   float shearX = 0;
   float shearZ = 0;

   transform.trans = glm::vec3(x, y, z);
   transform.scale = glm::vec3(s, s, s);
   transform.rotX = rotX;
   transform.rotY = rotY;
   transform.rotZ = rotZ;
   transform.shearX = shearX;
   transform.shearZ = shearZ;
}

void bunnyTransform(transform_t &transform) {
   // Produce a random transform
   float s = 2.0;
   float x = 0;
   float y = 1.8;
   float z = 0;
   float rotX = 0;
   float rotY = 0;
   float rotZ = 0;
   float shearX = 0;
   float shearZ = 0;

   transform.trans = glm::vec3(x, y, z);
   transform.scale = glm::vec3(s, s, s);
   transform.rotX = rotX;
   transform.rotY = rotY;
   transform.rotZ = rotZ;
   transform.shearX = shearX;
   transform.shearZ = shearZ;
}

void skyTransform(transform_t &transform) {
   // Produce a random transform
   float s = 13.0;
   float x = 0;
   float y = 1.8;
   float z = 0;
   float rotX = 0;
   float rotY = 0;
   float rotZ = 0;
   float shearX = 0;
   float shearZ = 0;

   transform.trans = glm::vec3(x, y, z);
   transform.scale = glm::vec3(s, s, s);
   transform.rotX = rotX;
   transform.rotY = rotY;
   transform.rotZ = rotZ;
   transform.shearX = shearX;
   transform.shearZ = shearZ;
}

void grassTransform(transform_t &transform, int which) {
   // Produce a random transform
   float s = 1.0;
   float x = (which%25)*(SIZE*2/25) - SIZE;
   float y = -0.7;
   float z = (which/25)*(SIZE*2/25) - SIZE;
   float rotX = 0;
   float rotY = rand()%360;
   float rotZ = 0;
   float shearX = 0;
   float shearZ = 0;

   transform.trans = glm::vec3(x, y, z);
   transform.scale = glm::vec3(s, s, s);
   transform.rotX = rotX;
   transform.rotY = rotY;
   transform.rotZ = rotZ;
   transform.shearX = shearX;
   transform.shearZ = shearZ;
}


void initModel(int objType, int objNum, transform_t &transform) {
   switch (objType) {
   case SPHERE: // light source
      skyTransform(transform);
      break;
   case BUNNY: // center bunny
      bunnyTransform(transform);
      break;
   case CUBE: // shrine of cubes
      cubeTransform(transform, objNum);
      break;
   case GRASS: // floor grass
      grassTransform(transform, objNum);
      break;
   default:
      cout << "initializing unfamiliar object type: " << objType << endl;
      break;
   }
}

void initGround() {
   // Position array of ground
   GLfloat vertices[] = {
      -1.0f, 0.0, -1.0f, //0
      -1.0f, 0.0, +1.0f, //1
      +1.0f, 0.0, -1.0f, //2
      +1.0f, 0.0, +1.0f, //3
   };
   // Amplify ground size
   for (int i = 0; i < sizeof(vertices); i++) {
      // Don't amplify the floor height
      if (i % 3 != 1)
         vertices[i] *= SIZE;
   }
   glGenBuffers(1, &(bufIDs[NUM_OBJ_TYPES].pos));
   glBindBuffer(GL_ARRAY_BUFFER, bufIDs[NUM_OBJ_TYPES].pos);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   // Index array of ground
   GLuint indices[] = {
      0, 1, 2,
      1, 3, 2,
   };
   glGenBuffers(1, &(bufIDs[NUM_OBJ_TYPES].ind));
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs[NUM_OBJ_TYPES].ind);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

   // Normal array of ground
   vector<float> posBuf(vertices, vertices+sizeof(vertices)/sizeof(vertices[0]));
   vector<unsigned int> indBuf(indices, indices+sizeof(indices)/sizeof(indices[0]));
   vector<float> norBuf = computeNormals(posBuf, indBuf);

   glGenBuffers(1, &(bufIDs[NUM_OBJ_TYPES].nor));
   glBindBuffer(GL_ARRAY_BUFFER, bufIDs[NUM_OBJ_TYPES].nor);
   glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
}

void initObj(int objType, std::vector<tinyobj::shape_t> &shapes, GLuint &posBufID, GLuint &indBufID, GLuint &norBufID) {
   // Position array of object
   const vector<float> &posBuf = shapes[0].mesh.positions;
   glGenBuffers(1, &posBufID);
   glBindBuffer(GL_ARRAY_BUFFER, posBufID);
   glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);

   // Index array of object
   const vector<unsigned int> &indBuf = shapes[0].mesh.indices;
   glGenBuffers(1, &indBufID);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufID);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indBuf.size()*sizeof(unsigned int), &indBuf[0], GL_STATIC_DRAW);

   // Normal array of object
   vector<float> norBuf = computeNormals(posBuf, indBuf);
   glGenBuffers(1, &norBufID);
   glBindBuffer(GL_ARRAY_BUFFER, norBufID);
   glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);

   // Initialize transforms for each object
   switch (objType) {
   case SPHERE: // light source
      initModel(objType, 1, transforms[objType][0]);
      break;
   case BUNNY: // center bunny
      initModel(objType, 1, transforms[objType][0]);
      break;
   case CUBE: // shrine of cubes
      for (int i = 0; i < NUM_CUBES; i++) {
         initModel(objType, i, transforms[objType][i]);
      }
      break;
   case GRASS: // floor grass
      for (int i = 0; i < NUM_OBJS; i++) {
         initModel(objType, i, transforms[objType][i]);
      }
      break;
   default:
      cout << "initializing unfamiliar object type: " << objType << endl;
      break;
   }
}

void initGL() {
   // Set the background color
   glClearColor(0.6f, 0.6f, 0.8f, 1.0f);

   initGround();
   for (int i = 0; i < NUM_OBJ_TYPES; i++) {
      initObj(i, allShapes[i], bufIDs[i].pos, bufIDs[i].ind, bufIDs[i].nor);
   }

   // Unbind the arrays
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   GLSL::checkVersion();
   assert(glGetError() == GL_NO_ERROR);
}

/** DRAWING **/
glm::vec3 lookAtPt() {
   glm::vec3 lookAtPt = glm::vec3(cos(phi)*cos(theta), sin(phi), cos(phi)*cos((M_PI/2)-theta));
   lookAtPt += g_view;
   return lookAtPt;
}

void setProjectionMatrix() {
   glm::mat4 Projection = glm::perspective(90.0f, (float)g_width/g_height, 0.1f, 100.f);
   safe_glUniformMatrix4fv(h_uP, glm::value_ptr(Projection));
}

void setView() {
   glm::mat4 View = glm::lookAt(g_view, lookAtPt(), glm::vec3(0, 1, 0));
   safe_glUniformMatrix4fv(h_uV, glm::value_ptr(View));
   glUniform3f(h_uView, g_view.x, g_view.y, g_view.z);
}

void setMaterial(int i) {
   glUniform1i(h_uID, DEFAULT_ID);
   // Cycle through the materials for everyone
   if (i >= 0) {
      i %= NUM_MAT;
   }
   glm::vec3 col;

   switch (i) {
      case CUBE_MAT: // shiny (shrine of cubes)
         glUniform3f(h_uAClr, 0.09, 0.07, 0.08);
         glUniform3f(h_uDClr, 0.91, 0.782, 0.82);
         glUniform3f(h_uSClr, 1.0, 0.913, 0.8);
         glUniform1f(h_uS, 200.0);
         break;
      case LIGHT_MAT: // super-bright white (light)
         glUniform3f(h_uAClr, 1.0, 1.0, 1.0);
         glUniform3f(h_uDClr, 1.0, 1.0, 1.0);
         glUniform3f(h_uSClr, 1.0, 1.0, 1.0);
         glUniform1f(h_uS, 1.0);
         break;
      case GROUND_MAT: // flat brown (ground)
         glUniform1i(h_uID, GROUND_ID);
         break;
      case SKY_MAT: // sky blue (light) // TODO replace with texture
         glUniform1i(h_uID, SKY_ID);
         break;
      case GRASS_MAT: // green grass
         col = glm::vec3(0.313, 0.784, 0.470);
         glUniform3f(h_uAClr, col.x/10.0, col.y/10.0, col.z/10.0);
         glUniform3f(h_uDClr, col.x/3.0, col.y/3.0, col.z/3.0);
         glUniform3f(h_uSClr, col.x/1.0, col.y/1.0, col.z/1.0);
         glUniform1f(h_uS, 300.0);
         break;

      case 0: // emerald
         col = glm::vec3(0.313, 0.784, 0.470);
         glUniform3f(h_uAClr, col.x/10.0, col.y/10.0, col.z/10.0);
         glUniform3f(h_uDClr, col.x/3.0, col.y/3.0, col.z/3.0);
         glUniform3f(h_uSClr, col.x/1.0, col.y/1.0, col.z/1.0);
         glUniform1f(h_uS, 300.0);
         break;
      case 1: // gold
         glUniform3f(h_uAClr, 0.09, 0.07, 0.08);
         glUniform3f(h_uDClr, 0.91, 0.782, 0.82);
         glUniform3f(h_uSClr, 1.0, 0.913, 0.8);
         glUniform1f(h_uS, 200.0);
         break;
      case 2: // shiny blue plastic
         glUniform3f(h_uAClr, 0.02, 0.02, 0.1);
         glUniform3f(h_uDClr, 0.0, 0.08, 0.5);
         glUniform3f(h_uSClr, 0.14, 0.14, 0.4);
         glUniform1f(h_uS, 120.0);
         break;
      case 3: //ruby jewel
         col = glm::vec3(0.607, 0.066, 0.117);
         glUniform3f(h_uAClr, col.x/10.0, col.y/10.0, col.z/10.0);
         glUniform3f(h_uDClr, col.x/3.0, col.y/3.0, col.z/3.0);
         glUniform3f(h_uSClr, col.x/1.0, col.y/1.0, col.z/1.0);
         glUniform1f(h_uS, 300.0);
         break;

      default: // black
         glUniform3f(h_uAClr, 0.0, 0.0, 0.0);
         glUniform3f(h_uDClr, 0.0, 0.0, 0.0);
         glUniform3f(h_uSClr, 0.0, 0.0, 0.0);
         glUniform1f(h_uS, 1.0);
         break;
   }
}

glm::mat4 shear(float shearX, float shearZ) {
   glm::mat4 m = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, 1));

   m[0][0] = 1; m[0][1] = 0; m[0][2] = shearX; m[0][3] = 0;
   m[1][0] = 0; m[1][1] = 1; m[1][2] = shearZ; m[1][3] = 0;
   m[2][0] = 0; m[2][1] = 0; m[2][2] = 1; m[2][3] = 0;
   m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;

   return m;
}

void setModel(transform_t &transform) {
   // Calculate each matrix
   glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.scale);
   glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.trans);
   glm::mat4 RX = glm::rotate(glm::mat4(1.0f), transform.rotX, glm::vec3(1.0, 0.0, 0.0));
   glm::mat4 RY = glm::rotate(glm::mat4(1.0f), transform.rotY, glm::vec3(0.0, 1.0, 0.0));
   glm::mat4 RZ = glm::rotate(glm::mat4(1.0f), transform.rotZ, glm::vec3(0.0, 0.0, 1.0));
   glm::mat4 SH = shear(transform.shearX, transform.shearZ);

   glm::mat4 R = RY*RZ*RX;
   glm::mat4 MV = T*SH*R*S;

   safe_glUniformMatrix4fv(h_uM, glm::value_ptr(MV));
   safe_glUniformMatrix4fv(h_uR, glm::value_ptr(R));
}

void drawGround() {
   // Bind position buffer
   glBindBuffer(GL_ARRAY_BUFFER, bufIDs[NUM_OBJ_TYPES].pos);
   glVertexAttribPointer(h_aPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
   // Bind index buffer
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs[NUM_OBJ_TYPES].ind);
   // Bind normal buffer
   glVertexAttribPointer(h_aNor, 3, GL_FLOAT, GL_FALSE, 0, 0);
/*
   // Bind texture
   glEnable(GL_TEXTURE_2D);
   glActiveTexture(GL_TEXTURE0);
   glUniform1i(h_uTexUnit, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   GLSL::enableVertexAttribArray(h_aTexCoord);
   glBindBuffer(GL_ARRAY_BUFFER, bufIDs[NUM_OBJ_TYPES].tex);
   glVertexAttribPointer(h_aTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0); 
*/
   // Set color
   setMaterial(GROUND_MAT);
   // Apply translation
   glm::vec3 trans = glm::vec3(0.0, -1.0, 0.0);
   glm::mat4 T = glm::translate(glm::mat4(1.0f), trans);
   safe_glUniformMatrix4fv(h_uM, glm::value_ptr(T));

   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void drawLight(std::vector<tinyobj::shape_t> &shapes, int which) {
   // Bind position buffer
   glBindBuffer(GL_ARRAY_BUFFER, bufIDs[0].pos);
   glVertexAttribPointer(h_aPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
   // Bind index buffer
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIDs[0].ind);
   // Bind normal buffer
   glVertexAttribPointer(h_aNor, 3, GL_FLOAT, GL_FALSE, 0, 0);

   // Set color
   setMaterial(LIGHT_MAT);
   // Apply transformation
   glm::mat4 T;
   switch (which) {
   case 0:
      T = glm::translate(glm::mat4(1.0f), g_light1);
      break;
   case 1:
      T = glm::translate(glm::mat4(1.0f), g_light2);
      break;
   case 2:
      T = glm::translate(glm::mat4(1.0f), g_light3);
      break;
   }
   glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.5, 0.5, 0.5));
   glm::mat4 MV = T*S;
   safe_glUniformMatrix4fv(h_uM, glm::value_ptr(MV));

   int nIndices = (int)shapes[0].mesh.indices.size();
   glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
}

void drawObj(int objType, std::vector<tinyobj::shape_t> &shapes, GLuint &posBufID, GLuint &indBufID, GLuint &norBufID) {
   // Bind position buffer
   glBindBuffer(GL_ARRAY_BUFFER, posBufID);
   glVertexAttribPointer(h_aPos, 3, GL_FLOAT, GL_FALSE, 0, 0);

   // Bind index buffer
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufID);

   // Bind normal buffer
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
   glVertexAttribPointer(h_aNor, 3, GL_FLOAT, GL_FALSE, 0, 0);
   
   if (objType == BUNNY) {
      // Set the bunny's color
      setMaterial(bunnyMat);
      // Set the bunny's transformation
      setModel(transforms[objType][0]);

      int nIndices = (int)shapes[0].mesh.indices.size();
      glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
   } else if(objType == SPHERE) {
      // LIGHT HANDLED IN drawLight
 
      // Set the color
      setMaterial(SKY_MAT);
      // Set the model transformation
      setModel(transforms[objType][0]);

      int nIndices = (int)shapes[0].mesh.indices.size();
      glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
   } else if(objType == GRASS && drawGrass) {
       for (int i = 0; i < NUM_OBJS; i++) {
         // Set the color
         setMaterial(GRASS_MAT);
         // Set the model transformation
         setModel(transforms[objType][i]);

         int nIndices = (int)shapes[0].mesh.indices.size();
         glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
      }
   } else if (objType == CUBE) {
      // Send data to the GPU and draw
      for (int i = 0; i < NUM_CUBES; i++) {
         // Set the color
         setMaterial(CUBE_MAT);
         // Set the model transformation
         setModel(transforms[objType][i]);

         int nIndices = (int)shapes[0].mesh.indices.size();
         glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
      }
   }
}

void drawGL() {
   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   // Use "frag.glsl" and "vert.glsl"
   glUseProgram(ShadeProg);
/*
   // Enable textures
   glEnable(GL_TEXTURE_2D);
   glActiveTexture(GL_TEXTURE0);
*/
   // Send position info to the attribute "aPos"
   GLSL::enableVertexAttribArray(h_aPos);
   // Send normal info to the attribute "aNor"
   GLSL::enableVertexAttribArray(h_aNor);
/*
   // Send texture info to attribute "aTexCoord"
   GLSL::enableVertexAttribArray(h_aTexCoord);
*/
   // Send light position to the uniform "uL"
   glUniform3f(h_uL1, g_light1.x, g_light1.y, g_light1.z);
   glUniform3f(h_uL2, g_light2.x, g_light2.y, g_light2.z);
   glUniform3f(h_uL3, g_light3.x, g_light3.y, g_light3.z);

   setProjectionMatrix();
   setView();

   drawLight(allShapes[0], 0);
   drawLight(allShapes[0], 1);
   drawLight(allShapes[0], 2);
   drawGround();
   for (int i = 0; i < NUM_OBJ_TYPES; i++) {
      drawObj(i, allShapes[i], bufIDs[i].pos, bufIDs[i].ind, bufIDs[i].nor);
   }

   // Disable and unbind
   GLSL::disableVertexAttribArray(h_aPos);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glUseProgram(0);
   assert(glGetError() == GL_NO_ERROR);
}

/** ANIMATION **/
void updateLight() {
   float t = ANIM_LIGHT_SPD*frame;

   g_light1 = glm::vec3(5*cos(t), 2.5*sin(t)*cos(t)+2.5, 5*(cos(t)*sin(t)));
   t += M_PI/1.5;
   g_light2 = glm::vec3(5*cos(t), 2.5*sin(t)*cos(t)+2.5, 5*(cos(t)*sin(t)));
   t += M_PI/1.5;
   g_light3 = glm::vec3(5*cos(t), 2.5*sin(t)*cos(t)+2.5, 5*(cos(t)*sin(t)));
}

void animate() {
   updateLight();
   /*
      transforms[BUNNY][0].shearX = sin(SHEAR_SPEED*frame);
      transforms[BUNNY][0].shearZ = cos(SHEAR_SPEED*frame);
      */
   for (int i = 0; i < NUM_OBJS; i++) {
      transforms[GRASS][i].shearX = SHEAR_MAG*sin(SHEAR_SPEED*frame+i);
      transforms[GRASS][i].shearZ = SHEAR_MAG*cos(SHEAR_SPEED*frame+i);
   }
}

/** WINDOW CALLBACKS **/
void window_size_callback(GLFWwindow* window, int w, int h) {
   glViewport(0, 0, (GLsizei)w, (GLsizei)h);
   g_width = w;
   g_height = h;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
   // Only register single key presses
   if (action != GLFW_RELEASE)
      return;

   switch (key) {
   case GLFW_KEY_R: // Restart camera position
      g_view = glm::vec3(7, 0, 7);
      theta = -3*M_PI/4.0;
      phi = 0;
      break;
   case GLFW_KEY_X: // Switch bunny material
      bunnyMat = (bunnyMat+1) % NUM_MAT;
      break;
   case GLFW_KEY_G: // Toggle which type of ground
      drawGrass = !drawGrass;
      break;
   case GLFW_KEY_SPACE: // Animation toggle
      animating = !animating;
      break;
   }
}

void key_check(GLFWwindow* window) {
   glm::vec3 viewVector = glm::normalize(lookAtPt() - g_view);
   glm::vec3 strafeVector = glm::normalize(glm::cross(viewVector, glm::vec3(0, 1, 0)));
   glm::vec3 crossVector = glm::normalize(glm::cross(viewVector, strafeVector));
   // Scale vectors
   viewVector *= MOVEMENT_SPEED;
   strafeVector *= MOVEMENT_SPEED;
   crossVector *= MOVEMENT_SPEED;

   if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // Move forward
      g_view += viewVector;
   if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // Move backward
      g_view -= viewVector;
   if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // Strafe left
      g_view -= strafeVector;
   if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // Strafe right
      g_view += strafeVector;

   if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE
      && glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE
      && glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE
      && glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) {
      g_view.y = 1;
   } else {
      g_view.y = 0.1*sin(HEAD_SPEED*frame)+1;
   }

   if (g_view.x < -BORDER || g_view.x > BORDER)
      g_view.x = g_view.x < 0 ? -BORDER : BORDER;
   if (g_view.z < -BORDER || g_view.z > BORDER)
      g_view.z = g_view.z < 0 ? -BORDER : BORDER;

/*
   if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) // Move light forward
      g_light += viewVector;
   if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) // Move light backward
      g_light -= viewVector;
   if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) // Move light left
      g_light -= strafeVector;
   if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) // Move light right
      g_light += strafeVector;
   if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) // Move light left
      g_light += crossVector;
   if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) // Move light right
      g_light -= crossVector;
*/
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
   // Update theta (x angle) and phi (y angle)
   float half_width = g_width / 2.0;
   float half_height = g_height / 2.0;
   float xPosFromCenter = xpos - half_width;
   float yPosFromCenter = ypos - half_height;
   float xMag = xPosFromCenter / half_width;
   float yMag = yPosFromCenter / half_height;

   theta += ROTATE_SPEED*M_PI*xMag;
   // Bound phi to 80 degrees
   float newPhi = phi - ROTATE_SPEED*M_PI*yMag/2.0;
   if (glm::degrees(newPhi) < 80 && glm::degrees(newPhi) > -80) {
      phi = newPhi;
   }

   // Keep mouse in center
   glfwSetCursorPos(window, g_width/2, g_height/2);
}

void enter_callback(GLFWwindow* window, int entered) {
   // Position mouse at center if enter screen
   glfwSetCursorPos(window, g_width/2, g_height/2);
}

/** MAIN **/
int main(int argc, char **argv) {
   // Seed randomness
   srand(time(NULL));

   // Initialise GLFW
   if (!glfwInit()) {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return -1;
   }
   glfwWindowHint(GLFW_SAMPLES, 4);
   glfwWindowHint(GLFW_RESIZABLE,GL_FALSE);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

   // Open a window and create its OpenGL context
   g_width = 1024;
   g_height = 768;
   window = glfwCreateWindow(g_width, g_height, "Final Prog by Chris Williams", NULL, NULL);
   if (window == NULL) {
      fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
      glfwTerminate();
      return -1;
   }
   glfwMakeContextCurrent(window);

   // Window callbacks
   glfwSetKeyCallback(window, key_callback);
   glfwSetWindowSizeCallback(window, window_size_callback);
   glfwSetCursorPosCallback(window, mouse_callback);
   glfwSetCursorEnterCallback(window, enter_callback);

   // Initialize GLEW
   if (glewInit() != GLEW_OK) {
      fprintf(stderr, "Failed to initialize GLEW\n");
      return -1;
   }

   // Input modes
   glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE); // Ensure we "hear" ESC
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN); // Hide the mouse

   // Enable alpha drawing
   glEnable (GL_BLEND);
   glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable (GL_DEPTH_TEST);

   // Load models (light source is the first object)
   for (int i = 0; i < NUM_OBJ_TYPES; i++) {
      loadShapes(objectFiles[i], allShapes[i], allMaterials[i]);
   }

   initGL();
   installShaders("vert.glsl", "frag.glsl");

   do {
      animate();

      drawGL();

      glfwSwapBuffers(window);
      glfwPollEvents();
      key_check(window);
      
      if (animating)
         frame++;
   } // Check if the ESC key was pressed or the window was closed
   while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS
         && glfwWindowShouldClose(window) == 0);

   // Close OpenGL window and terminate GLFW
   glfwTerminate();

   return 0;
}
