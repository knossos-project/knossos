#version 150

uniform mat4 projection_matrix;
uniform vec4 light_bg;
uniform vec4 light_front;
uniform vec4 light_back;
uniform bool picking;
uniform float invert = 1.0f;

in float fradius;
in vec4 fcolor;
in vec3 flight_normal;

out vec4 fragColor;

void main() {
    vec2 pos = (2.0 * vec2(gl_PointCoord.s, gl_PointCoord.t) - 1.0) * vec2(-1,-1);
    float dist_squared = dot(pos, pos);

    if (dist_squared > 1.0) {
        discard;
    } else {
        float dist_edge = sqrt(1.0 - min(1.0, dist_squared));
        vec3 frag_normal = normalize(vec3(pos.x, pos.y, dist_edge));
        float intensity = max(0.0, dot(frag_normal, flight_normal));// spot
        fragColor = fcolor * vec4(vec3(!picking) * vec3(light_bg.rgb + intensity * light_front.rgb + light_back.rgb) + vec3(picking), 1);

        float projectionDepthRange = (-2.0 / projection_matrix[2].z);
        gl_FragDepth = gl_FragCoord.z + invert * dist_edge * fradius * (gl_DepthRange.diff / projectionDepthRange);
        gl_FragDepth = clamp(gl_FragDepth, gl_DepthRange.near, gl_DepthRange.far);
    }
}