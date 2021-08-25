#version 110

varying vec3 frag_normal;
varying vec4 frag_color;

vec4 diffuse(vec3, vec4);

void main() {
    gl_FragColor = diffuse(frag_normal, frag_color);
}
