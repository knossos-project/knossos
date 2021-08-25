#version 110

uniform sampler3D texture;
uniform float textureOpacity;

varying vec3 texCoordFrag;

void main() {
    gl_FragColor = vec4(vec3(texture3D(texture, texCoordFrag).r), textureOpacity);
}
