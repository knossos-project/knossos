#version 150

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

in vec3 vertex;
in vec3 texCoordVertex;

out vec3 texCoordFrag;

void main() {
    mat4 mvp_mat = projection_matrix * view_matrix * model_matrix;
    gl_Position = mvp_mat * vec4(vertex, 1.0);
    texCoordFrag = texCoordVertex;
}
