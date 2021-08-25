#version 110

uniform sampler2D sampler;

varying vec2 frag_tex;

void main() {
    vec4 val = texture2D(sampler, 0.5*(frag_tex+1.0));// texture(…) in 130+
    gl_FragColor = val;
}
