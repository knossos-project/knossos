#version 150

uniform mat4 modelview_matrix;
uniform vec4 light_bg;
uniform vec4 light_front;
uniform vec4 light_back;

in vec4 fcolor;
in float fradius;
in vec3 fside_scaled;
in vec3 fcenter_n;
in vec3 flight_n;

out vec4 fragColor;

void main() {
    float thick = fradius * sqrt(1.0 - pow(length(fside_scaled) / fradius,2.0));
    vec3 offset = fside_scaled + fcenter_n * thick;
    vec3 frag_normal = normalize((modelview_matrix * vec4(-offset,0)).xyz);
    float intensity = max(0.0, dot(frag_normal, flight_n));// spotlowp
    fragColor = fcolor * vec4(light_bg.rgb + intensity * light_front.rgb + light_back.rgb, 1);
}