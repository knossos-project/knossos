#version 120

attribute vec3 vertex;

uniform vec4 viewport;

varying float radius;
varying vec2  center;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);

    float R = 0.3;
    radius = R;

    gl_PointSize = R * min(viewport.z, viewport.w);

    center = gl_Position.xy;
}
