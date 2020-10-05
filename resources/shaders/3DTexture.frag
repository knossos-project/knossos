#version 150

uniform sampler3D cube;
uniform float textureOpacity;

in vec3 texCoordFrag;

out vec4 fragOut;

void main() {
    fragOut = vec4(vec3(texture(cube, texCoordFrag).r), textureOpacity);
}
