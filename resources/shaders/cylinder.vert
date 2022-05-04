#version 150

in vec3 vertex;
in vec3 ref;
in float radius;
in vec4 color;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec4 light_pos;

out vec4 fcolor;
out float fradius;
out vec3 fdist;
out vec3 fvpn;
out vec3 fln;

void main() {
    // [[-1, 1, 1], [-1, -1, 1], [1, -1, -1], [1, -1, -1]]
    // -1 1 1 1 comma4*comma2*comma3
    // 0 0 2 2 (1+comma2)

    //float comma  = sign(    (float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.2);
    float comma2 = sign(    (float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.4);
    float comma3 = sign(-1*((float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.2));
    float comma4 = sign(-1*((float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.4));
    vec3 vpup = vec3(modelview_matrix[0][2], modelview_matrix[1][2], modelview_matrix[2][2]);
    vec3 side = cross(vertex-ref, vpup);
    vec3 pos = comma4*comma2*comma3*(1+comma2) * normalize(side) * radius*0.5;
    pos += vertex;
    vec3 dist = pos - vertex.xyz;
    pos += (1-0.5*(1+comma2)) * normalize(cross(vertex-ref, side)) * radius;

    gl_Position = projection_matrix * modelview_matrix * vec4(pos, 1.0);
    fcolor = color;
    fradius = radius;
    fdist = dist;
    fvpn = cross(vertex-ref, side);
    fln = normalize((modelview_matrix * vec4(vertex, 1.0)).xyz * light_pos.w - (modelview_matrix * light_pos).xyz);
}
