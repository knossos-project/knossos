#version 150

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec4 tree_color;
uniform vec3 vp_normal;

in vec4 frag_color;
in vec3 frag_normal;
in mat4 mvp_matrix;

out vec4 fragOut;

void main() {
    vec3 specular_color = vec3(1.0, 1.0, 1.0);
    float specular_exp = 3.0;
    vec3 view_dir = vec3(0.0, 0.0, 1.0);

    // diffuse lighting
    vec3 main_light_dir = normalize((/*modelview_matrix **/ vec4(0.0, -1.0, 0.0, 0.0)).xyz);
    float main_light_power = max(0.0, dot(-main_light_dir, frag_normal));
    vec3 sub_light_dir = vec3(0.0, 1.0, 0.0);
    float sub_light_power = max(0.0, dot(-sub_light_dir, frag_normal));

    vec3 fcolor = tree_color.rgb;
//            fragOut = vec4(0.5 + (0.5 * frag_normal), 1);
//            if (frag_normal.y <= 0.0) {
//                fragOut = vec4(0, frag_normal.y, 0, 1);
//            } else {
//                fragOut = vec4(frag_normal.y, 0, 0, 1);
//            }
    if (length(vp_normal) > 0.0) {
        float dot_value = dot(frag_normal, vp_normal);
        if (dot_value < 0.0) {// vp_normal faces towards the camera
            fragOut = tree_color;// show
//                    fragOut = vec4(0, dot_value, 0, 1);
        } else {
            fragOut = vec4(0.0, 0.0, 0.0, 0.0);// cut
//                    fragOut = vec4(1 + dot_value, 0, 0, 1);
        }
    } else {
         fragOut = vec4((0.25 * fcolor             // ambient
         + 0.75 * fcolor * main_light_power // diffuse(main)
         + 0.25 * fcolor * sub_light_power  // diffuse(sub)
         )
         , tree_color.a);
    }
}
