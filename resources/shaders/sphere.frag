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
        gl_FragColor = fcolor;
        gl_FragDepth = gl_FragCoord.z + gl_DepthRange.diff / 2.0 * gl_ProjectionMatrix[2].z;
    }
}
