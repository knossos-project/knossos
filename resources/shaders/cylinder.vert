#version 130

attribute vec3 vertex;
attribute vec3 ref;
attribute float radius;
attribute vec4 color;

varying vec4 frag_color;

void main() {
    float comma = (float(gl_VertexID)/4.0-int(float(gl_VertexID)/4.0))-0.2;
    float comma2 = (float(gl_VertexID+1)/4.0-int(float(gl_VertexID+1)/4.0))-0.6;
    vec3 vpup = vec3(gl_ModelViewMatrix[0][2], gl_ModelViewMatrix[1][2], gl_ModelViewMatrix[2][2]);
    vec3 pos = sign(comma) * sign(comma2) * normalize(cross(vpup, vertex-ref)) * radius;
    pos += vertex;

    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
    frag_color = color;
}
