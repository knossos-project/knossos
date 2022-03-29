#version 110

in vec3 vertex;
in vec2 tex;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

varying vec2 ftex;

void main() {
    gl_Position = projection_matrix * modelview_matrix * vec4(vertex, 1);
    ftex = tex;
}
