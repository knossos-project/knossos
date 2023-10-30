#version 150

uniform sampler3D indexTexture;
uniform float factor;//expand float to uint8 range

uniform sampler1D textureLUT;
uniform float lutSize;//float(textureSize1D(textureLUT, 0));

uniform float textureOpacity;

in vec3 texCoordFrag;

out vec4 fragOut;

void main() {
    float index = texture(indexTexture, texCoordFrag).r;
    index *= factor;
    fragOut = texture(textureLUT, (index + 0.5) / lutSize);
    fragOut.a = textureOpacity;
}
