#version 150

uniform sampler2D samplerColor;
uniform vec2 screen;
uniform vec4 tree_color;
uniform float alpha_factor;

out vec4 fragOut;

void main() {
    vec4 color = texture(samplerColor, gl_FragCoord.xy / screen);// texture(…) in 130+
    if (color.g == 0.0 && color.r != 0.0) {
        discard;
    }
    fragOut = tree_color * vec4(1,1,1,alpha_factor);// show
}
