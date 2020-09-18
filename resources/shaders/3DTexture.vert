#version 150

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

attribute vec3 vertex;
attribute vec3 texCoordVertex;

varying vec3 texCoordFrag;

void main() {
    mat4 mvp_mat = projection_matrix * view_matrix * model_matrix;
    gl_Position = mvp_mat * vec4(vertex, 1.0);
    texCoordFrag = texCoordVertex;
}
