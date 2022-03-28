#version 120

uniform vec4 viewport;
uniform float zoom;

varying vec2 node_center_ndc;
varying float fradius;
varying vec4 fcolor;

void main() {
    vec2 vp_center = 0.5 * viewport.zw;
    vec2 frag_pos_ndc = ((gl_FragCoord.xy - viewport.xy) - vp_center) / vp_center;

    vec2 frag_offset_2d = frag_pos_ndc - node_center_ndc;
    float dist_squared = dot(frag_offset_2d, frag_offset_2d);
    float size_squared = pow(2 * zoom * fradius / viewport.z, 2);

    if (dist_squared > size_squared) {
        discard;
    } else {
        gl_FragColor = fcolor;
    }
}
