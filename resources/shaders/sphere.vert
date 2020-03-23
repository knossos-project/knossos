#version 120

attribute vec3 vertex;
attribute float radius;

uniform float zoom;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);

    gl_PointSize = zoom * 2.0 * radius;
}
