#version 120

uniform vec4 viewport;
uniform float zoom;

varying vec2 node_center_ndc;
varying float fradius;
varying vec4 fcolor;

void main() {
    vec2 vp_center = 0.5 * viewport.zw;
    vec2 frag_pos_ndc = ((gl_FragCoord.xy - viewport.xy) - vp_center) / vp_center;

    float radius = 2.0 * zoom * fradius / viewport.z;
    vec2 pos = (frag_pos_ndc - node_center_ndc) / radius;
    float dist_squared = dot(pos, pos);

    if (dist_squared > 1.0) {
        discard;
    } else {
        float dist_edge = sqrt(1.0 - min(1.0, dist_squared));
        vec3 frag_normal = normalize(vec3(pos.x, -pos.y, -dist_edge));
        vec3 light_normal = normalize(gl_LightSource[0].position.xyz);
        float intensity = max(0.0, dot(frag_normal, light_normal));// spot
        gl_FragColor = fcolor * (gl_LightModel.ambient
                                     + intensity * gl_LightSource[0].diffuse + gl_LightSource[0].ambient);
    }
}
