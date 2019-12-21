#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 0) out vec3 fragmentColor;

void main() {
    fragmentColor = color;
	gl_Position = vec4(position, 1.0);
}
