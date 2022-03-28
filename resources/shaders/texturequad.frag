#version 110

uniform sampler2D sampler;

varying vec2 ftex;

void main() {
    gl_FragColor = texture2D(sampler, ftex);// texture(…) in 130+
}
