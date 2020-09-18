#version 150

flat in vec4 frag_color;

out vec4 fragColorOut;

void main() {
    if (gl_FrontFacing) {// front is visible means we are not inside
        fragColorOut = vec4(0.0, 0.0, 0.0, 0.0);// cut
    } else {
        fragColorOut = frag_color;// render
    }
}
