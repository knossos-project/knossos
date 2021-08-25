#version 110

uniform vec4 tree_color;
uniform float alpha_factor;

varying vec3 frag_normal;

vec4 diffuse(vec3, vec4);

void main() {
    gl_FragColor = diffuse(frag_normal, tree_color * vec4(1,1,1,alpha_factor));
}
