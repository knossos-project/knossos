#version 110

varying float dist;

void main() {
    gl_FragColor = vec4(sqrt(1.0-pow(dist,2.0)),0,0,1);
}
