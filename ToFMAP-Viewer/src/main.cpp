#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <atomic>
#include <thread>
#include <vector>

#include "vec3.h"
#include "custom_sockets.hpp"
#include "payload_types.h"
#include "gl_renderer.hpp"

#define PI acos(-1)

Vec3(float);
Vec3(int);

typedef struct Surface_particle
{
  double radius;
  GLfloat* color;
	Vec3_float loc;
} Surface_particle;

namespace ToF_MAP {
	Vec3_float Board_dim;
	Vec3_float Tof_rotation_orbit_axis_offset;
	Vec3_float Vehicle_location;

	std::atomic<int> Vehicle_Rotation_offset;
	std::atomic<Vehicle_data_t> Vehicle_data;
	std::atomic<std::vector<Surface_particle>*> Surface_particles;
	std::atomic<std::vector<Surface_particle>*> Surface_particles_buffer;
	std::atomic<int> Rotation_offset;

	Board Draw_Board;
	Camera Preview_Camera;
}

typedef struct Upd_args
{
	struct timespec t;
	char signal;
	char ack_signal;
} Upd_args;

typedef struct GL_args
{
	int *argc;
	char **argv;
	char signal;
	char ack_signal;
	Vec3_float dim;
} GL_args;

struct Vehicle_socket_t
{
	int PORT;
	std::atomic<Controller_udata_t> *udata;
	std::atomic<Vehicle_data_t> *data;
	Vehicle_socket_t(int PORT, std::atomic<Controller_udata_t> *udata, std::atomic<Vehicle_data_t> *data)
		: PORT(PORT), udata(udata), data(data)
	{}
};

Surface_particle get_Surface_particle(Vec3_float location);
void particle_update(Surface_particle *b, int x_limit, float radius, char vehicle);
void init(Vec3_float dim);
void reshape(GLint w, GLint h);
void timer(int v);
void special(int key, int a, int b);
void normal(unsigned char key, int a, int b);
Upd_args get_Upd_args(struct timespec t);
int upd_par(Upd_args *args);
GL_args get_GL_args(int *argc, char **argv, Vec3_float dim);
int gl_render_init_th(GL_args *args);
int socket_main_task(Vehicle_socket_t arg);

int main(int argc, char** argv)
{
	printf(""
			" _____    ___ __  __   _   ___ __     ____   ___\n"
			"|_   _|__| __|  \\/  | /_\\ | _ \\ _|_ _|_ \\ \\ / (_)_____ __ _____ _ _\n"
			"  | |/ _ \\ _|| |\\/| |/ _ \\|  _/ |\\ \\ /| |\\ V /| / -_) V  V / -_) '_|\n"
			"  |_|\\___/_| |_|  |_/_/ \\_\\_| | |/_\\_\\| | \\_/ |_\\___|\\_/\\_/\\___|_|\n"
			"                              |__|   |__|\n"
			);
	if (argc != 4) {
		fprintf(stderr, "Numero de argumentos incorrecto!\n");
		fprintf(stderr, "Se espera:\n");
		fprintf(stderr, "\t%s [Dimensiones (20-100)] [N bloques (1-200)] [Altura maxima (1-10)]\n", argv[0]);
		fprintf(stderr, "Ej:\t%s 100 5 4\n", argv[0]);
		return -1;
	}

	char err_msg[170] = {'\0'};
	int err = 0;
	int dim = atoi(argv[1]);
	int val = atoi(argv[2]);
	int height = atoi(argv[3]);
	
	std::atomic<Controller_udata_t> cd;
	
	if (dim < 15 || dim > 200) {
		sprintf(err_msg, "El valor de dimension debe estar entre 10 y 100, se introdujo %d\n", dim);
		err += -2;
	}

	if (val < 1 || val > 200) {
		sprintf(err_msg, "El valor de bloques debe estar entre 1 y 200, se introdujo %d\n", val);
		err += -3;
	}

	if (height < 1 || height > 10) {
		sprintf(err_msg + strlen(err_msg), "El valor de altura bloques debe estar entre 1 y 10, se introdujo %d\n", height);
		err += -4;
	}

	if (err) {
		fprintf(stderr, "%s", err_msg);
		return err;
	}
	
	ToF_MAP::Tof_rotation_orbit_axis_offset = get_Vec3_float(0.01, 0.12, 0);

	ToF_MAP::Board_dim = get_Vec3_float(dim, dim, dim);
	srand(time(NULL));
	ToF_MAP::Preview_Camera = get_Camera(1.9248, 0.8, 4, 4);


	cd.store((Controller_udata_t){0});

	ToF_MAP::Surface_particles.store(new std::vector<Surface_particle>());
	ToF_MAP::Surface_particles_buffer.store(new std::vector<Surface_particle>());
	
	struct timespec task_init_delay = {3600};

	/* Threads */ { 
		auto args = get_Upd_args((struct timespec) {0, 20000000});
		auto surface_particles_populate_task = std::thread(upd_par, &args);

		auto gl_args = get_GL_args(&argc, argv, ToF_MAP::Board_dim);
		auto render_task = std::thread(gl_render_init_th, &gl_args);

		auto socket_args = Vehicle_socket_t(2403, &cd, &ToF_MAP::Vehicle_data);
		auto socket_comm_task = std::thread(socket_main_task, socket_args);

		nanosleep(&task_init_delay, NULL);
		glutLeaveMainLoop();
	}


	std::vector<Surface_particle>().swap(*ToF_MAP::Surface_particles.load());
	delete ToF_MAP::Surface_particles.load();
	std::vector<Surface_particle>().swap(*ToF_MAP::Surface_particles_buffer.load());
	delete ToF_MAP::Surface_particles_buffer.load();

	return 0;
}

Surface_particle get_Surface_particle(Vec3_float location)
{
	return (Surface_particle) {
		.radius = 0,
		.color = NULL,
		.loc = location
	};
}

void particle_update(Surface_particle *b, int x_limit, float radius, char vehicle)
{
	if (!b)
		return;
  glPushMatrix();

  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, !vehicle ? GLColors::GREEN : GLColors::RED);
  glTranslated(x_limit - b->loc.x, b->loc.y, b->loc.z);

	if (!vehicle) {
  	glutSolidSphere(radius, 5, 5);
	} else {
		glRotatef(ToF_MAP::Vehicle_Rotation_offset + (float)ToF_MAP::Vehicle_data.load().dir_x / 1000, 0.0, 1.0, 0.0);
		glutSolidCone(0.2, 0.75,10, 10);
	}
  glPopMatrix();
}

void display();

// Application-specific initialization: Set up global lighting parameters
// and create display lists.
void init(Vec3_float dim)
{
  glClearColor(0.07, 0.14825, 0.13805, 0.0);
  glEnable(GL_DEPTH_TEST);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, GLColors::WHITE);
  glLightfv(GL_LIGHT0, GL_SPECULAR, GLColors::WHITE);
  glMaterialfv(GL_FRONT, GL_SPECULAR, GLColors::WHITE);
  glMaterialf(GL_FRONT, GL_SHININESS, 100);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
	ToF_MAP::Draw_Board = get_Board((int)round(dim.x), (int)round(dim.z), 0);
}


void display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	int particle_count = ToF_MAP::Surface_particles.load()->size();
	int i = 0;
	Vec3_float dist;
	Surface_particle p;

	p.loc.x = (float)ToF_MAP::Vehicle_data.load().loc_x / 1000;
	p.loc.z = (float)ToF_MAP::Vehicle_data.load().loc_y / 1000;
	p.loc.y = 10;
	particle_update(&p, ToF_MAP::Board_dim.x, 2.5, 1);
	
	while ((particle_count-i-1) >= 0) {
		dist = ToF_MAP::Surface_particles.load()->at(particle_count-i-1).loc;
		vec3_subv_float(&dist, ToF_MAP::Vehicle_location);
/*
		if (vec3_get_mod_float(&dist) < 5) {
			ToF_MAP::Surface_particles.load()->erase(ToF_MAP::Surface_particles.load()->begin() + particle_count-i-1);
			particle_count--;
			continue;
		}*/

		particle_update(&ToF_MAP::Surface_particles.load()->at(particle_count-i-1), ToF_MAP::Board_dim.x, 0.125, 0);
		if (++i == particle_count || i == 10000)
			break;
	}

  glLoadIdentity();
  gluLookAt(ToF_MAP::Preview_Camera.x, ToF_MAP::Preview_Camera.y, ToF_MAP::Preview_Camera.z,
            ToF_MAP::Draw_Board.h/2, 0.0, ToF_MAP::Draw_Board.v/2,
            0.0, 1.0, 0.0);
  glCallList(ToF_MAP::Draw_Board.gl_displayListId);
  glFlush();
  glutSwapBuffers();
}

// On reshape, constructs a camera that perfectly fits the window.
void reshape(GLint w, GLint h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(40.0, (GLfloat)w / (GLfloat)h, 1.0, 150.0);
  glMatrixMode(GL_MODELVIEW);
}

// Requests to draw the next frame.
void timer(int v)
{
  glutPostRedisplay();
  glutTimerFunc(1000/60, timer, v);
}

// Moves the camera according to the key pressed, then ask to refresh the
// display.
void special(int key, int a, int b)
{
  switch (key) {
    case GLUT_KEY_RIGHT: camera_move(&ToF_MAP::Preview_Camera,'R'); break;
    case GLUT_KEY_LEFT: camera_move(&ToF_MAP::Preview_Camera,'L'); break;
    case GLUT_KEY_UP: camera_move(&ToF_MAP::Preview_Camera,'U'); break;
    case GLUT_KEY_DOWN: camera_move(&ToF_MAP::Preview_Camera,'D'); break;
  }
  glutPostRedisplay();
}

void normal(unsigned char key, int a, int b)
{
  switch (key) {
		case 'w': ToF_MAP::Preview_Camera.z += 2; break;
		case 's': ToF_MAP::Preview_Camera.z -= 2; break;
		case 'q': // Canvas surface particles delete
							{
								std::vector<Surface_particle>().swap(*ToF_MAP::Surface_particles.load());
								delete ToF_MAP::Surface_particles.load();
								ToF_MAP::Surface_particles.store(new std::vector<Surface_particle>);
								break;
							}
		case 'a':
							ToF_MAP::Rotation_offset.store(ToF_MAP::Rotation_offset.load() + 3);
							break;
		case 'd':
							ToF_MAP::Rotation_offset.store(ToF_MAP::Rotation_offset.load() - 3);
							break;
		case 'z':
							ToF_MAP::Vehicle_Rotation_offset.store(ToF_MAP::Vehicle_Rotation_offset.load() - 8);
							break;
		case 'c':
							ToF_MAP::Vehicle_Rotation_offset.store(ToF_MAP::Vehicle_Rotation_offset.load() - 8);
							break;
		case 'g': // Clipboard copy
							{
								std::vector<Surface_particle>().swap(*ToF_MAP::Surface_particles_buffer.load());
                delete ToF_MAP::Surface_particles_buffer.load();
                ToF_MAP::Surface_particles_buffer.store(new std::vector<Surface_particle>);

								ToF_MAP::Surface_particles_buffer.load()->insert(
										ToF_MAP::Surface_particles_buffer.load()->end(),
										ToF_MAP::Surface_particles.load()->begin(), ToF_MAP::Surface_particles.load()->end()
										);

                break;
							}
		case 'h': // Clipboard paste
							{
								ToF_MAP::Surface_particles.load()->insert(
										ToF_MAP::Surface_particles.load()->end(),
										ToF_MAP::Surface_particles_buffer.load()->begin(), ToF_MAP::Surface_particles_buffer.load()->end()
										);
								break;
							}
		case 'k':
							break;

  }
  glutPostRedisplay();
}

Upd_args get_Upd_args(struct timespec t)
{
	return (Upd_args) {
		.t = t,
		.signal = 0,
		.ack_signal = 0
	};
}

int upd_par(Upd_args *args)
{
	if (!args)
		return -1;
	static float deg = (float)PI/2;
	static float incc = (float)PI / 2;
	static float mdeg = PI * 2;

	Vec3_float& init_pos = ToF_MAP::Vehicle_location;
	Vec3_float sum_pos = get_Vec3_float(0, 0, 0);
	float current_dist;
	int current_Rotation_offset;

	while (!args->signal) {
		current_Rotation_offset = ToF_MAP::Rotation_offset.load() % 360;
		current_dist = (float)ToF_MAP::Vehicle_data.load().dist / 1000;
		//current_dist = 0.5;
		init_pos.x = (float)ToF_MAP::Vehicle_data.load().loc_x / 1000;
		init_pos.z = (float)ToF_MAP::Vehicle_data.load().loc_y / 1000;
		nanosleep(&args->t, NULL);

		incc = (float)((float)ToF_MAP::Vehicle_data.load().inc / 1000000) / 360 * 2 * PI;
		deg = (float)(current_Rotation_offset + (float)ToF_MAP::Vehicle_data.load().azm / 1000000) / 360 * 2 * PI;
		sum_pos.x =
			sin(incc) * sin(mdeg - deg) * current_dist * 10;
		
		/*
		 *
		 * Only 3D model view
		 *
		 **/

		/*
		sum_pos.y =
			cos(incc) * sin(mdeg - deg) * current_dist * 10;*/

		sum_pos.y = PI;
		sum_pos.z = cos(mdeg - deg) * current_dist * 10;

		vec3_muls_float(&sum_pos, 1);
		vec3_addv_float(&sum_pos, init_pos);
		ToF_MAP::Surface_particles.load()->push_back(get_Surface_particle(sum_pos));
	}

	args->ack_signal = 1;

	return 0;
}

GL_args get_GL_args(int *argc, char **argv, Vec3_float dim)
{
	return (GL_args) {
		.argc = argc,
		.argv = argv,
		.signal = 0,
		.ack_signal = 0,
		.dim = dim
	};
}

int gl_render_init_th(GL_args *args)
{
	if (!args)
		return -1;

  glutInit(args->argc, args->argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowPosition(80, 80);
  glutInitWindowSize(800, 600);
  glutCreateWindow("ToF playground");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutSpecialFunc(special);
	glutKeyboardFunc(normal);
  glutTimerFunc(100, timer, 0);

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
  init(args->dim);
	args->signal = 1;
  glutMainLoop();

	args->ack_signal = 1;

	return 0;
}

int socket_main_task(Vehicle_socket_t arg)
{
	return socket_data_exchange_as_server_init_loop(arg.PORT, arg.udata, arg.data);
}
