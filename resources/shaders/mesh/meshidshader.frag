#version 130

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec3 vp_normal;

flat in vec4 frag_color;
in vec3 frag_normal;
in mat4 mvp_matrix;

out vec4 fragColorOut;

void main() {
    if (length(vp_normal) > 0.0) {
        if (!gl_FrontFacing) {// vp_normal faces towards the camera
            fragColorOut = frag_color;// show
        } else {
            fragColorOut = vec4(1.0, 1.0, 1.0, 1.0);// cut
        }
    } else {
        fragColorOut = frag_color;
    }
}
