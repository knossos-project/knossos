#version 120

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec4 light_pos;

attribute vec3 vertex;
attribute vec4 color;
attribute float radius;

uniform float zoom;

varying float fradius;
varying vec4 fcolor;
varying vec3 flight_normal;

void main() {
    gl_Position = projection_matrix * modelview_matrix * vec4(vertex, 1.0);

    gl_PointSize = zoom * 2.0 * radius;
    flight_normal = normalize((modelview_matrix * vec4(vertex, 1.0)).xyz * light_pos.w - (modelview_matrix * light_pos).xyz);

    fradius = radius;
    fcolor = color;
}
