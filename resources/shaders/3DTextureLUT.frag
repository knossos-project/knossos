#version 110

uniform sampler3D indexTexture;
uniform float factor;//expand float to uint8 range

uniform sampler1D textureLUT;
uniform float lutSize;//float(textureSize1D(textureLUT, 0));

uniform float textureOpacity;

varying vec3 texCoordFrag;

void main() {
    float index = texture3D(indexTexture, texCoordFrag).r;
    index *= factor;
    gl_FragColor = texture1D(textureLUT, (index + 0.5) / lutSize);
    gl_FragColor.a *= textureOpacity;
}
