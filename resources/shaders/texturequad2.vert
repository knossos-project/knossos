#version 110

attribute vec3 vertex;
attribute vec2 tex;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

varying vec2 ftex;

void main() {
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(vertex, 1);
    gl_Position = projection_matrix * modelview_matrix * vec4(vertex, 1);
    ftex = tex;
}
