#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "globals.hpp"

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

GLint init_shaders();
bool loadOBJ(const char * path, std::vector<glm::vec3> & out_vertices, std::vector<glm::vec3> & out_normals);
int init_parser(char *fp);
int get_next_iteration_sphere_state();

static GLFWwindow* window;
std::vector<glm::vec3> verts;
std::vector<glm::vec3> normals;
static glm::vec3 cam_pos;
static glm::mat4 cam;
static glm::mat4 proj;
static GLint sphere_shader_prog;
// Uniform ids
static GLuint view_unif_id;
static GLuint proj_unif_id;
static GLuint trans_unif_id;
static GLuint scale_unif_id;
static GLuint light_pos_unif_id;
// Buffer ids
GLuint vertex_buffer;
GLuint normal_buffer;

float simulation_time_of_next_event;
float simulation_time_of_last_event;

// Sets up glfw, glew and OpenGL
static int init_gl() {
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return -1;
	}
	glfwWindowHint(GLFW_SAMPLES, 4);
	window = glfwCreateWindow(1600, 900, "Visualiser", NULL, NULL);
	if (window == NULL) {
		printf("Failed to open GLFW window.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		glfwTerminate();
		return -1;
	}
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	return 0;
}

// At the moment the camera and light position don't change so the uniforms
// for these only need to be uploaded once.
static void upload_buffers_and_uniforms() {
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(glm::vec3), &verts[0], GL_STATIC_DRAW);
	glGenBuffers(1, &normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
	cam_pos = glm::vec3(25.0f, 25.0f, 100.0f); // nice effect if this is set to all 0s
	cam = glm::lookAt(cam_pos, glm::vec3(25.0f, 25.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	proj = glm::perspective(glm::radians(90.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	glUniformMatrix4fv(view_unif_id, 1, GL_FALSE, &cam[0][0]);
	glUniformMatrix4fv(proj_unif_id, 1, GL_FALSE, &proj[0][0]);
	glUniform3fv(light_pos_unif_id, 1, &cam_pos[0]);
}

// Sets up the window and shaders needed.
// Also reads initial state from the binary file
static int init() {
	if (init_parser("data.bin") == -1) {
		return -1;
	}
	if (init_gl() == -1) {
		return -1;
	}
	sphere_shader_prog = init_shaders();
	if (sphere_shader_prog == -1) {
		glfwTerminate();
		return -1;
	}
	glUseProgram(sphere_shader_prog);
	view_unif_id = glGetUniformLocation(sphere_shader_prog, "view");
	proj_unif_id = glGetUniformLocation(sphere_shader_prog, "projection");
	trans_unif_id = glGetUniformLocation(sphere_shader_prog, "translation");
	scale_unif_id = glGetUniformLocation(sphere_shader_prog, "scale");
	if (loadOBJ("sphere.mdl", verts, normals) == false) {
		glfwTerminate();
		return -1;
	}
	upload_buffers_and_uniforms();
	return 0;
}

static void draw_sphere_at(glm::vec3 pos, float radius) {
	radius = radius * 2.0f; // TODO: spheres are half their actual size - need to fix properly
	glm::mat4 scale = glm::scale(glm::vec3(radius, radius, radius));
	glm::mat4 translation = glm::translate(pos);
	glUniformMatrix4fv(trans_unif_id, 1, GL_FALSE, &translation[0][0]);
	glUniformMatrix4fv(scale_unif_id, 1, GL_FALSE, &scale[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, verts.size() * sizeof(glm::vec3)); 
}

static void prepare_to_draw_spheres() {
	glUseProgram(sphere_shader_prog);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}

static const float tick_rate = 1.0f / 60.0f;

int main(void) {
	if (init() != 0) {
		getchar();
		return 1;
	}
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	float time_elapsed_iter = 0.0f;
	float cur_time = (float) glfwGetTime();
	bool last_iter = false;
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		prepare_to_draw_spheres();
		float time_left = simulation_time_of_next_event - simulation_time_of_last_event - time_elapsed_iter;
		for (int i = 0; i < num_spheres; i++) {
			draw_sphere_at(spheres[i].pos, spheres[i].radius);
			if (time_left < tick_rate) {
				update_sphere_position(&spheres[i], time_left);
			} else {
				update_sphere_position(&spheres[i], tick_rate);
			}
		}
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);
		glfwSwapBuffers(window);
		glfwPollEvents();
		// uncomment to run in real time
		// running at a faster rate is useful for debugging
		while (((float) glfwGetTime() - cur_time) < tick_rate) {}
		cur_time = glfwGetTime();
		time_elapsed_iter += tick_rate;
		// TODO: time between iterations may be less than tick rate
		if (time_elapsed_iter >= simulation_time_of_next_event - simulation_time_of_last_event) {
			time_elapsed_iter = 0.0f;
			if (last_iter) {
				break;
			} else {
				int fin = get_next_iteration_sphere_state();
				if (fin == 1) {
					last_iter = true;
				} else if (simulation_time_of_next_event - simulation_time_of_last_event <= tick_rate) {
					//printf("TODO: unhandled case\n");
					//getchar();
					//exit(1);
				}
			}
		}
	}
	glfwTerminate();
	return 0;
}
