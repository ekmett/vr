#pragma once

#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>

const char * gl_source(GLenum source);
const char * gl_message_type(GLenum type);
void objectLabelf(GLenum id, GLuint name, const char *fmt, ...);
