#version 120

attribute vec3 vertex;
attribute vec4 color;
attribute float radius;

uniform float zoom;

varying vec2 node_center_ndc;
varying float fradius;
varying vec4 fcolor;
varying vec3 flight_normal;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(vertex, 1.0);

    gl_PointSize = zoom * 2.0 * radius;
    flight_normal = normalize((gl_ModelViewMatrix * vec4(vertex, 1.0)).xyz * gl_LightSource[0].position.w - gl_LightSource[0].position.xyz);

    node_center_ndc = gl_Position.xy;
    fradius = radius;
    fcolor = color;
}
