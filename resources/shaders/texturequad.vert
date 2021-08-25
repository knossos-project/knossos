#version 110

attribute vec3 vertex;

varying vec2 frag_tex;

void main() {
    gl_Position = vec4(vertex, 1);
    frag_tex = vertex.xy;
}
