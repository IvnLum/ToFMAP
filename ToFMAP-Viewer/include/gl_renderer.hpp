#ifndef _GL_RENDERER_
#define _GL_RENDERER_


#include <GL/freeglut.h>
#include <cmath>

namespace GLColors {
	constexpr GLfloat WHITE[] = {1, 1, 1};
	constexpr GLfloat RED[] = {1, 0, 0};
	constexpr GLfloat GREEN[] = {0, 1, 0};
	constexpr GLfloat BLACK[] = {0, 0, 0};
}

typedef struct Camera
{
  double phi;
  double x, y, z;
  double dPhi;
  double dy;
} Camera;

typedef struct Board
{
  GLuint gl_displayListId;
  int h;
  int v;
} Board;

Camera get_Camera(float phi, float y, float dPhi, float dy);
void camera_move(Camera *cam, char target);

Board get_Board(int h, int v, int height);

#endif /* !_GL_RENDERER_ */ 
