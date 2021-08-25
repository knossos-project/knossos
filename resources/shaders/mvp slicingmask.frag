#version 110

void main() {
    if (gl_FrontFacing) {// front is visible means we are not inside
        gl_FragColor = vec4(0.5, 0.0, 0.0, 0.0);// mark as to cut
    } else {
        gl_FragColor = vec4(0.0, 0.5, 0.0, 0.0);// mark as valid
    }
}
