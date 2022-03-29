#version 150

uniform sampler2D sampler;
uniform vec4 color_factor = vec4(1);

in vec2 ftex;

out vec4 fragOut;

void main() {
    vec4 val = color_factor * texture(sampler, ftex);// texture(…) in 130+
    fragOut = val;
}
