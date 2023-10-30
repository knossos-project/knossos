#version 150

in vec3 vertex;
in vec3 segment;
in float radius;
in int shift;
in uint raised;
in vec4 color;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform float side_flip;
uniform vec4 light_pos;

out vec4 fcolor;
out float fradius;
out vec3 fside_scaled;
out vec3 fcenter_n;
out vec3 flight_n;

void main() {
    vec3 vpup = vec3(modelview_matrix[0][2], modelview_matrix[1][2], modelview_matrix[2][2]);
    vec3 side_n   = normalize(cross(segment, vpup));
    vec3 center_n = normalize(cross(segment, side_n));
    vec3 side_scaled   = side_flip * shift   * radius * side_n;
    vec3 center_scaled = raised * radius * center_n;
    vec3 pos = vertex + side_scaled + center_scaled;

    gl_Position = projection_matrix * modelview_matrix * vec4(pos, 1.0);
    fcolor = color;
    fradius = radius;
    fside_scaled = side_scaled;
    fcenter_n = center_n;
    flight_n = normalize((modelview_matrix * vec4(vertex, 1.0)).xyz * light_pos.w - (modelview_matrix * light_pos).xyz);
}
