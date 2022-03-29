#version 150

in vec3 vertex;

out vec2 ftex;

void main() {
    gl_Position = vec4(vertex, 1);
    ftex = 0.5 * (vertex.xy + 1.0);
}
