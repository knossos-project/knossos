#version 150

in vec3 vertex;
in vec3 normal;
in vec4 color;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform float alpha_factor;

out vec3 frag_normal;
out vec4 frag_color;

void main() {
    mat4 mvp_matrix = projection_matrix * modelview_matrix;
    gl_Position = mvp_matrix * vec4(vertex, 1.0);
    frag_normal = normal;
    frag_color = vec4(color.rgb, color.a * alpha_factor);
}
