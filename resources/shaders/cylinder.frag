#version 110

uniform mat4 modelview_matrix;

varying vec4 frag_color;
varying float fradius;
varying vec3 fdist;
varying vec3 fvpn;
varying vec3 fln;

void main() {
    float thick = fradius*sqrt(1.0 - pow(length(fdist)/fradius,2.0));
    vec3 offset = fdist+normalize(fvpn)*thick;
    vec3 frag_normal = normalize((modelview_matrix * vec4(-offset,0)).xyz);
    float intensity = max(0.0, dot(frag_normal, fln));// spot
    gl_FragColor = frag_color * (gl_LightModel.ambient
                                 + intensity * gl_LightSource[0].diffuse + gl_LightSource[0].ambient);
}
