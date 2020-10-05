#version 150

in vec3 frag_normal;
out vec4 frag_color;

vec4 diffuse(vec3, vec4);

out vec4 fragOut;

void main() {
    fragOut = diffuse(frag_normal, frag_color);
}
