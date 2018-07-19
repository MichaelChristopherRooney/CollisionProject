#version 400

in vec3 eye_pos;
in vec3 eye_normal;

out vec4 colour;

uniform mat4 view;

uniform vec3 light_pos;
vec3 lightSpecular = vec3(1.0, 0.5, 1.0);
vec3 lightDiffuse = vec3(0.1, 0.1, 0.1);
vec3 lightAmbient = vec3(0.1, 0.1, 0.1);

vec3 surfaceSpecular = vec3(1.0, 1.0, 1.0);
vec3 surfaceDiffuse = vec3(1.0, 0.5, 0.0);
vec3 surfaceAmbient = vec3(1.0, 1.0, 1.0);
float specularExponent = 100.0;

void main () {

	vec3 lightEyePosition = vec3(view * vec4(light_pos, 1.0));
	vec3 distance = lightEyePosition - eye_pos;
	vec3 direction = normalize(distance);
	
	vec3 diffuseIntensity = lightDiffuse * surfaceDiffuse * max(dot(direction, eye_normal), 0.0);
	
	vec3 ambientIntensity = lightAmbient * surfaceAmbient;

	vec3 surfaceEyeDirection = normalize(-eye_pos);
	float specularFactor = pow(max(dot
							(normalize(surfaceEyeDirection + direction), eye_normal), 
							0.0), specularExponent);
	vec3 specularIntensity = lightSpecular * surfaceSpecular * specularFactor;

	
	colour = vec4(specularIntensity + diffuseIntensity + ambientIntensity, 1.0);
}