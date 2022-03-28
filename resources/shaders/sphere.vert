#version 120

attribute vec3 vertex;
attribute vec4 color;
attribute float radius;

uniform float zoom;

varying vec2 node_center_ndc;
varying float fradius;
varying vec4 fcolor;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);

    gl_PointSize = zoom * 2.0 * radius;

    node_center_ndc = gl_Position.xy;
    fradius = radius;
    fcolor = color;
}
