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

#define SIZE 5.0
#define ROTATE_SPEED 0.25
#define MOVEMENT_SPEED 0.1
#define NUM_MAT 5
#define LIGHT_SPEED 0.01
#define NUM_LIGHT_PAT 4

using namespace std;

// Window
GLFWwindow* window;
int g_width;
int g_height;
int frame = 0;
// Camera
glm::vec3 g_view(0, 1, 0);
float theta = -M_PI/2.0;
float phi = 0;
// Light
glm::vec3 g_light(0, 6, 0);
int lightPattern = 0;
// Model vectors
vector<tinyobj::shape_t> bunny_s;
vector<tinyobj::material_t> bunny_m;
vector<tinyobj::shape_t> sphere_s;
vector<tinyobj::material_t> sphere_m;
GLuint ShadeProg;
// Bunny buffer ids
GLuint bunnyPosBufID;
GLuint bunnyNorBufID;
GLuint bunnyIndBufID;
// Ground buffer ids
GLuint groundPosBufID;
GLuint groundNorBufID;
GLuint groundIndBufID;
// Shader handlers
GLint h_aPos;
GLint h_aNor;
GLint h_uP;
GLint h_uV;
GLint h_uM;
GLint h_uID;
GLint h_uView;
GLint h_uL;
GLint h_uAClr, h_uDClr, h_uSClr, h_uS;

inline void safe_glUniformMatrix4fv(const GLint handle, const GLfloat data[]) {
   if (handle >= 0)
      glUniformMatrix4fv(handle, 1, GL_FALSE, data);
}

void SetProjectionMatrix() {
   glm::mat4 Projection = glm::perspective(90.0f, (float)g_width/g_height, 0.1f, 100.f);
   safe_glUniformMatrix4fv(h_uP, glm::value_ptr(Projection));
}

glm::vec3 lookAtPt() {
   glm::vec3 lookAtPt = glm::vec3(cos(phi)*cos(theta), sin(phi), cos(phi)*cos((M_PI/2)-theta));
   lookAtPt += g_view;
   return lookAtPt;
}

void SetView() {
   glm::mat4 View = glm::lookAt(g_view, lookAtPt(), glm::vec3(0, 1, 0));
   safe_glUniformMatrix4fv(h_uV, glm::value_ptr(View));
   glUniform3f(h_uView, g_view.x, g_view.y, g_view.z);
}

void SetMaterial(int i) {
   // Cycle through the materials for everyone
   if (i >= 0) {
      i %= NUM_MAT;
   }
   glm::vec3 col;

   switch (i) {
      case -2: // super-bright white (light)
         glUniform3f(h_uAClr, 1.0, 1.0, 1.0);
         glUniform3f(h_uDClr, 1.0, 1.0, 1.0);
         glUniform3f(h_uSClr, 1.0, 1.0, 1.0);
         glUniform1f(h_uS, 1.0);
         break;
      case -1: // flat grey (ground)
         glUniform3f(h_uAClr, 0.13, 0.13, 0.14);
         glUniform3f(h_uDClr, 0.3, 0.3, 0.4);
         glUniform3f(h_uSClr, 0.0, 0.0, 0.0);
         glUniform1f(h_uS, 4.0);
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
      case 4: // shiny black
         glUniform3f(h_uAClr, 0.0, 0.0, 0.0);
         glUniform3f(h_uDClr, 0.0, 0.0, 0.0);
         glUniform3f(h_uSClr, 1.0, 1.0, 1.0);
         glUniform1f(h_uS, 150.0);
      default: // super-bright white
         glUniform3f(h_uAClr, 1.0, 1.0, 1.0);
         glUniform3f(h_uDClr, 1.0, 1.0, 1.0);
         glUniform3f(h_uSClr, 1.0, 1.0, 1.0);
         glUniform1f(h_uS, 1.0);
         break;
   }
}

void SetModel(int id) {
   glm::vec3 trans;

   // TODO place objects randomly in world
   // TODO random rotation
   // TODO random scale
   switch (id) {
      case -2: // sun
         trans = g_light;
         break;
      case -1: // ground
         trans = glm::vec3(0.0, -1.0, 0.0);
         break;
      case 0:
         trans = glm::vec3(0.0, 0.0, -5.0); // front
         break;
      case 1:
         trans = glm::vec3(0.0, 0.0, 5.0); // back
         break;
      case 2:
         trans = glm::vec3(-5.0, 0.0, 0.0); // left
         break;
      case 3:
         trans = glm::vec3(5.0, 0.0, 0.0); // right
         break;
      default: // when in doubt, throw it smack dab in the center
         trans = glm::vec3(0.0, 0.0, 0.0);
         break;
   }

   glm::mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
   safe_glUniformMatrix4fv(h_uM, glm::value_ptr(Trans));

   SetMaterial(id);

   // Send the id of this object
   glUniform1i(h_uID, id);
}

// Given a vector of shapes which has already been read from an obj file resize all vertices to the range [-1, 1]
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

void loadShapes(const string &objFile, vector<tinyobj::shape_t> &shapes, vector<tinyobj::material_t> &materials) {
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
   h_uID = GLSL::getUniformLocation(ShadeProg, "uID");
   h_uView = GLSL::getUniformLocation(ShadeProg, "uView");
   h_uL = GLSL::getUniformLocation(ShadeProg, "uL");
   h_uAClr = GLSL::getUniformLocation(ShadeProg, "uAClr");
   h_uDClr = GLSL::getUniformLocation(ShadeProg, "uDClr");
   h_uSClr = GLSL::getUniformLocation(ShadeProg, "uSClr");
   h_uS = GLSL::getUniformLocation(ShadeProg, "uS");

   assert(glGetError() == GL_NO_ERROR);
   return true;
}

vector<float> computeNormals(vector<float> posBuf, vector<unsigned int> indBuf, vector<tinyobj::shape_t> &shapes) {
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
      idx1 = shapes[0].mesh.indices[3*i+0];
      idx2 = shapes[0].mesh.indices[3*i+1];
      idx3 = shapes[0].mesh.indices[3*i+2];

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
   glGenBuffers(1, &groundPosBufID);
   glBindBuffer(GL_ARRAY_BUFFER, groundPosBufID);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   /* What exactly do these do?
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(3, GL_FLOAT, 0, 0);
      */

   // Index array of ground
   GLuint indices[] = {
      0, 1, 2,
      1, 3, 2,
   };
   glGenBuffers(1, &groundIndBufID);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundIndBufID);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

   // Normal array of ground
   vector<float> posBuf(vertices, vertices+sizeof(vertices)/sizeof(vertices[0]));
   vector<unsigned int> indBuf(indices, indices+sizeof(indices)/sizeof(indices[0]));
   vector<float> norBuf = computeNormals(posBuf, indBuf, bunny_s); // TODO does this cause problems?

   glGenBuffers(1, &groundNorBufID);
   glBindBuffer(GL_ARRAY_BUFFER, groundNorBufID);
   glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
}

void initObject(vector<tinyobj::shape_t> &shapes, GLuint posBufID, GLuint norBufID, GLuint indBufID) {
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

   // Normal array of ground
   vector<float> norBuf = computeNormals(posBuf, indBuf, shapes);

   glGenBuffers(1, &norBufID);
   glBindBuffer(GL_ARRAY_BUFFER, norBufID);
   glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
}

void initGL() {
   // Set the background color
   glClearColor(0.6f, 0.6f, 0.8f, 1.0f);

   initGround();
   initObject(bunny_s, bunnyPosBufID, bunnyNorBufID, bunnyIndBufID);

   // Unbind the arrays
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   GLSL::checkVersion();
   assert(glGetError() == GL_NO_ERROR);
}

void drawGround() {
   // Bind position buffer
   glBindBuffer(GL_ARRAY_BUFFER, groundPosBufID);
   glVertexAttribPointer(h_aPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
   // Bind index buffer
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundIndBufID);
   // Bind normal buffer
   glVertexAttribPointer(h_aNor, 3, GL_FLOAT, GL_FALSE, 0, 0);

   SetModel(-1);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void drawObject(vector<tinyobj::shape_t> &shapes, GLuint posBufID, GLuint norBufID, GLuint indBufID) {
   // Bind position buffer
   glBindBuffer(GL_ARRAY_BUFFER, posBufID);
   glVertexAttribPointer(h_aPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
   // Bind index buffer
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indBufID);
   // Bind normal buffer
   glVertexAttribPointer(h_aNor, 3, GL_FLOAT, GL_FALSE, 0, 0);

   int nIndices = (int)shapes[0].mesh.indices.size();
   // Set transforms per obj and draw
   for (int i = 0; i < 4; i++) {
      SetModel(i);
      glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
   }

   // Light source
   SetModel(-2);
   glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
}

void drawGL() {
   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   // Use "frag.glsl" and "vert.glsl"
   glUseProgram(ShadeProg);
   // Send position info to the attribute "aPos"
   GLSL::enableVertexAttribArray(h_aPos);
   // Send normal info to the attribute "aNor"
   GLSL::enableVertexAttribArray(h_aNor);
   // Send light position to the uniform "uL"
   glUniform3f(h_uL, g_light.x, g_light.y, g_light.z);

   SetProjectionMatrix();
   SetView();

   drawGround();
   drawObject(bunny_s, bunnyPosBufID, bunnyNorBufID, bunnyIndBufID);

   // Disable and unbind
   GLSL::disableVertexAttribArray(h_aPos);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glUseProgram(0);
   assert(glGetError() == GL_NO_ERROR);
}

/** Have the light move around in cool patterns **/
void updateLight() {
   float t = LIGHT_SPEED*frame;

   switch (lightPattern) {
   case 0: // Stop animating and control manually
      break;
   case 1: // Sunrise and sunset
      g_light = glm::vec3(2*SIZE*cos(t), SIZE*(sin(t)), 0);
      break;
   case 2: // Moving around floor
      g_light = glm::vec3(SIZE*cos(0.7*t), 0, SIZE*(sin(0.5*t)));
      break;
   default: // something must've gone wrong... quick! back to the sky!
      g_light = glm::vec3(0, SIZE, 0);
      break;
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
   case GLFW_KEY_SPACE: // Change light animating pattern
      lightPattern = (lightPattern+1)%NUM_LIGHT_PAT;
      break;
   }
}

void key_check(GLFWwindow* window) {
   glm::vec3 viewVector = lookAtPt() - g_view;
   glm::vec3 strafeVector = glm::cross(viewVector, glm::vec3(0, 1, 0));
   // Scale forward/backward speed
   viewVector *= MOVEMENT_SPEED;
   // Scale strafe speed
   strafeVector *= MOVEMENT_SPEED;

   if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // Move forward
      g_view += viewVector;
   if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // Move backward
      g_view -= viewVector;
   if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // Strafe left
      g_view -= strafeVector;
   if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // Strafe right
      g_view += strafeVector;

   if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) // Move light forward
      g_light.z -= MOVEMENT_SPEED;
   if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) // Move light backward
      g_light.z += MOVEMENT_SPEED;
   if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) // Move light up
      g_light.y += MOVEMENT_SPEED;
   if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) // Move light down
      g_light.y -= MOVEMENT_SPEED;
   if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) // Move light left
      g_light.x -= MOVEMENT_SPEED;
   if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) // Move light right
      g_light.x += MOVEMENT_SPEED;
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

int main(int argc, char **argv) {
   // Initialise GLFW
   if (!glfwInit()) {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return -1;
   }
   /* What exactly do these do?
      glfwWindowHint(GLFW_SAMPLES, 4);
      glfwWindowHint(GLFW_RESIZABLE,GL_FALSE);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
      */

   // Open a window and create its OpenGL context
   g_width = 1024;
   g_height = 768;
   window = glfwCreateWindow(g_width, g_height, "P3 - shading", NULL, NULL);
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

   // Load models
   loadShapes("bunny.obj", bunny_s, bunny_m);
   loadShapes("sphere.obj", sphere_s, sphere_m);

   initGL();
   installShaders("vert.glsl", "frag.glsl");

   do {
      updateLight();
      drawGL();

      glfwSwapBuffers(window);
      glfwPollEvents();
      key_check(window);
      frame++;
   } // Check if the ESC key was pressed or the window was closed
   while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS
         && glfwWindowShouldClose(window) == 0);

   // Close OpenGL window and terminate GLFW
   glfwTerminate();

   return 0;
}
