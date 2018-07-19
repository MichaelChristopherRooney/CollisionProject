#version 400

layout(location = 0) in vec3 vert;
layout(location = 1) in vec3 normal;

out vec3 eye_pos;
out vec3 eye_normal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 translation;
uniform mat4 scale;

void main(){
    	eye_pos = vec3(view * translation * scale * vec4(vert, 1.0));
	eye_normal = vec3(view * translation * vec4(normal, 0.0));
	gl_Position = projection * vec4(eye_pos, 1.0);
}