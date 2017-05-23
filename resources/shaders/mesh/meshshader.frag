#version 110

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec4 tree_color;
uniform vec3 vp_normal;

varying vec4 frag_color;
varying vec3 frag_normal;
varying mat4 mvp_matrix;

void main() {
    vec3 specular_color = vec3(1.0, 1.0, 1.0);
    float specular_exp = 3.0;
    vec3 view_dir = vec3(0.0, 0.0, 1.0);

    // diffuse lighting
    vec3 main_light_dir = normalize((/*modelview_matrix **/ vec4(0.0, -1.0, 0.0, 0.0)).xyz);
    float main_light_power = max(0.0, dot(-main_light_dir, frag_normal));
    vec3 sub_light_dir = vec3(0.0, 1.0, 0.0);
    float sub_light_power = max(0.0, dot(-sub_light_dir, frag_normal));

    // pseudo ambient lighting
    vec3 pseudo_ambient_dir = view_dir;
    float pseudo_ambient_power = pow(abs(max(0.0, dot(pseudo_ambient_dir, frag_normal)) - 1.0), 3.0);

    // specular
    float specular_power = 0.0;
    if (dot(frag_normal, -main_light_dir) >= 0.0) {
        specular_power = pow(max(0.0, dot(reflect(-main_light_dir, frag_normal), view_dir)), specular_exp);
    }

    vec3 fcolor = frag_color.rgb;
    if (length(vp_normal) > 0.0) {
        float dot_value = dot(frag_normal, vp_normal);
        if (dot_value < 0.0) {// vp_normal faces towards the camera
            gl_FragColor = vec4(fcolor.rgb, 0.5);// show
        } else {
            gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);// cut
        }
    } else {
         gl_FragColor = vec4((0.25 * fcolor             // ambient
         + 0.75 * fcolor * main_light_power // diffuse(main)
         + 0.25 * fcolor * sub_light_power  // diffuse(sub)
         )
         , frag_color.a);
    }
    // gl_FragColor = //vec4((frag_normal+1.0)/2.0, 1.0); // display normals
}
