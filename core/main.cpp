#include <SDL.h>
#include <GL/glew.h>
// #include <SDL_opengl.h>
#include <gl/glu.h>
#include <stdio.h>
#include <string>
#include <cstdlib>

#include <openvr.h>

#if defined(POSIX)
#include "unistd.h"
#endif

int main(int argc, char *argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	{
		printf("%s - SDL could not initialize! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}
	_sleep(1000);
	return 0;
}