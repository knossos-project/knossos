#version 130

attribute vec3 vertex;
attribute vec3 ref;
attribute float radius;
attribute vec4 color;

varying float dist;

void main() {
    float comma = (float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.2;
    float comma2 = (float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.4;
    float comma3 = (float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.6;
    float comma4 = 1-((float(gl_VertexID)/4.0-floor(float(gl_VertexID)/4.0))-0.4);
    vec3 vpup = vec3(gl_ModelViewMatrix[0][2], gl_ModelViewMatrix[1][2], gl_ModelViewMatrix[2][2]);
    vec3 pos = sign(comma4)*sign(comma2)*sign(comma3)*0.5*(1+sign(comma2)) * normalize(cross(vertex-ref, vpup)) * radius;
    pos += vertex;
    dist = length(pos - vertex.xyz) / radius;
    pos -= (1-0.5*(1+sign(comma2))) * normalize(vpup) * radius;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}
