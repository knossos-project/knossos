#version 110

uniform mat4 modelview_matrix;
uniform vec4 light_bg;
uniform vec4 light_front;
uniform vec4 light_back;

varying vec4 fcolor;
varying float fradius;
varying vec3 fdist;
varying vec3 fvpn;
varying vec3 fln;

void main() {
    float thick = fradius*sqrt(1.0 - pow(length(fdist)/fradius,2.0));
    vec3 offset = fdist+normalize(fvpn)*thick;
    vec3 frag_normal = normalize((modelview_matrix * vec4(-offset,0)).xyz);
    float intensity = max(0.0, dot(frag_normal, fln));// spot
    gl_FragColor = fcolor * vec4(light_bg.rgb + intensity * light_front.rgb + light_back.rgb, 1);
}
