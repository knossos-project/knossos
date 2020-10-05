#version 150

uniform sampler2D samplerColor;
uniform vec2 screen;
uniform vec4 tree_color;
uniform float alpha_factor;

vec4 meshSlicing(sampler2D, vec2, vec4);

out vec4 fragOut;

void main() {
    fragOut = meshSlicing(samplerColor, screen, tree_color * vec4(1,1,1,alpha_factor));
}
