#version 110

attribute vec3 vertex;
attribute vec3 normal;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

varying vec3 frag_normal;

void main() {
    mat4 mvp_matrix = projection_matrix * modelview_matrix;
    gl_Position = mvp_matrix * vec4(vertex, 1.0);
    frag_normal = normal;
}
