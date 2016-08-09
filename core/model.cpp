#include "model.h"
#include "gl_util.h"

using namespace vr;
using namespace std;

model::model(string & name, RenderModel_t & vrModel, RenderModel_TextureMap_t & diffuse) {  
  // create and bind a VAO to hold state for this model
  glGenVertexArrays(1, &vertArray);
  glBindVertexArray(vertArray);
  objectLabelf(GL_VERTEX_ARRAY, vertArray, "%s vertex array", name.c_str());

  glGenBuffers(1, &vertBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * vrModel.unVertexCount, vrModel.rVertexData, GL_STATIC_DRAW);
  objectLabelf(GL_BUFFER, vertBuffer, "%s vertex buffer", name.c_str());

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vPosition));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vNormal));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, rfTextureCoord));

  // Create and populate the index buffer
  glGenBuffers(1, &indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * vrModel.unTriangleCount * 3, vrModel.rIndexData, GL_STATIC_DRAW);
  objectLabelf(GL_BUFFER, indexBuffer, "%s index buffer", name.c_str());

  glBindVertexArray(0);

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, diffuse.unWidth, diffuse.unHeight,
    0, GL_RGBA, GL_UNSIGNED_BYTE, diffuse.rubTextureMapData);

  // If this renders black ask McJohn what's wrong.
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

  // let it go..
  glBindTexture(GL_TEXTURE_2D, 0);

  vertexCount = vrModel.unTriangleCount * 3;
}

model::~model() {
  glDeleteBuffers(1, &indexBuffer);
  glDeleteVertexArrays(1, &vertArray);
  glDeleteBuffers(1, &vertBuffer);
}

void model::draw() {
  glBindVertexArray(vertArray);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, 0);
  glBindVertexArray(0);
}