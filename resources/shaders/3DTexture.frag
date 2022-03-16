#version 150

uniform sampler3D cube;
uniform float textureOpacity;

in vec3 texCoordFrag;

void main() {
    gl_FragColor = vec4(vec3(texture(cube, texCoordFrag).r), textureOpacity);
}
