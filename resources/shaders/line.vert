#version 150

in vec3 vertex;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;


void main() {
    mat4 mvp_matrix = projection_matrix * modelview_matrix;
    gl_Position = mvp_matrix * vec4(vertex, 1.0);
}
