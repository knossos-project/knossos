#version 110

uniform float textureOpacity;
uniform sampler3D texture;
varying vec3 texCoordFrag;//in
//varying vec4 gl_FragColor;//out

void main() {
    gl_FragColor = vec4(vec3(texture3D(texture, texCoordFrag).r), textureOpacity);
}
