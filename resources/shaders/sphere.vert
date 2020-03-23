#version 120

attribute vec3 vertex;
attribute vec4 color;
attribute float radius;

uniform float zoom;

varying vec4 fcolor;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);

    gl_PointSize = zoom * 2.0 * radius;

    fcolor = color;
}
