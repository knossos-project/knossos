#version 130

attribute vec3 vertex;
attribute vec3 ref;
attribute float radius;
attribute vec4 color;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

varying vec4 frag_color;
varying float fradius;
varying vec3 fdist;
varying vec3 fvpn;
varying vec3 fln;

void main() {
    float comma = (float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.2;
    float comma2 = (float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.4;
    float comma3 = (float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.6;
    float comma4 = 1-((float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.4);
    vec3 vpup = vec3(modelview_matrix[0][2], modelview_matrix[1][2], modelview_matrix[2][2]);
    vec3 side = cross(vertex-ref, vpup);
    vec3 pos = sign(comma4)*sign(comma2)*sign(comma3)*0.5*(1+sign(comma2)) * normalize(side) * radius;
    pos += vertex;
    vec3 dist = pos - vertex.xyz;
    pos += (1-0.5*(1+sign(comma2))) * normalize(cross(vertex-ref, side)) * radius;

    gl_Position = projection_matrix * modelview_matrix * vec4(pos, 1.0);
    frag_color = color;
    fradius = radius;
    fdist = dist;
    fvpn = cross(vertex-ref, side);
    fln = normalize((modelview_matrix * vec4(vertex, 1.0)).xyz * gl_LightSource[0].position.w - gl_LightSource[0].position.xyz);
}
