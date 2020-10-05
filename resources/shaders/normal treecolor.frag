#version 150

uniform vec4 tree_color;
uniform float alpha_factor;

in vec3 frag_normal;

vec4 diffuse(vec3, vec4);

out vec4 fragOut;

void main() {
    fragOut = diffuse(frag_normal, tree_color * vec4(1,1,1,alpha_factor));
}
