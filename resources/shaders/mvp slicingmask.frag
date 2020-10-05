#version 150

out vec4 fragOut;

void main() {
    if (gl_FrontFacing) {// front is visible means we are not inside
        fragOut = vec4(0.5, 0.0, 0.0, 0.0);// mark as to cut
    } else {
        fragOut = vec4(0.0, 0.5, 0.0, 0.0);// mark as valid
    }
}
