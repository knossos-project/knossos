#version 150

vec4 meshSlicing(sampler2D samplerColor, vec2 screen, vec4 frag_color) {
    vec4 color = texture(samplerColor, gl_FragCoord.xy / screen);// texture(…) in 130+
    if (color.g != 0.0) {
        return frag_color;// show
    } else if (color.r != 0.0) {
        discard;
    }
}
