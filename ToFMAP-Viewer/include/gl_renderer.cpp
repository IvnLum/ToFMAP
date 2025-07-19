#include "gl_renderer.hpp"


Camera get_Camera(float phi, float y, float dPhi, float dy)
{
	return (Camera) {
		.phi = phi,
		.x = 50,//10 * cos(phi),
		.y = y,
		.z = 10 * sin(phi),
		.dPhi = dPhi,
		.dy = dy
	};
}

void camera_move(Camera *cam, char target)
{
	if (!cam)
		return;
	switch (target) {
	case 'R':
		cam->x -= cam->dPhi;
		break;
	case 'L':
		cam->x += cam->dPhi;
		break;
	case 'U':
		cam->y += cam->dy;
		break;
	case 'D':
		cam->y -= cam->dy;
	}
	//printf("[%.2f %.2f, %.2f, %.2f]\n", cam->phi, cam->x, cam->y, cam->z);
}

Board get_Board(int h, int v, int height)
{
	Board get = {
		.gl_displayListId = glGenLists(1),
		.h = h,
		.v = v
	};
	glNewList(get.gl_displayListId, GL_COMPILE);
  GLfloat lightPosition[4];
	lightPosition[0] = h/2;
	lightPosition[1] = v/2;
	lightPosition[2] = 3;
	lightPosition[3] = 3;
  glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
  glBegin(GL_QUADS);
  glNormal3d(0, 1, 0);

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, GLColors::BLACK);
  glVertex3d(0, 0, 0);
  glVertex3d(h, 0, 0);
  glVertex3d(h, 0, v);
  glVertex3d(0, 0, v);

  glEnd();
  glEndList();

	return get;
}
