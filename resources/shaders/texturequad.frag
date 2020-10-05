#version 150

uniform sampler2D sampler;
uniform vec4 color_factor = vec4(1);


varying vec2 ftex;

void main() {
    gl_FragColor = color_factor * texture2D(sampler, ftex);// texture(…) in 130+
}
