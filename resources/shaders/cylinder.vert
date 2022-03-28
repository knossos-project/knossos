#version 110

attribute vec3 vertex;
attribute vec3 ref;
attribute vec4 color;

varying vec4 frag_color;

void main() {
    vec3 up = (gl_ModelViewProjectionMatrixInverse * vec4(0,1,0,1)).xyz;
    frag_color.xyz = (gl_ModelViewProjectionMatrix * vec4(up, 1.0)).xyz;
    frag_color.xyz = normalize(frag_color.xyz);

    vec3 pos = ref - sign(vertex-ref) * normalize(up) * 200.0;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}
