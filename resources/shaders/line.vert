#version 150

in vec3 vertex;

uniform vec4 color;
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

out vec4 fcolor;

void main() {
    mat4 mvp_matrix = projection_matrix * modelview_matrix;
    gl_Position = mvp_matrix * vec4(vertex, 1.0);
    fcolor = color;
}
