vec4 diffuse(vec3 normal, vec4 color) {
    vec3 specular_color = vec3(1.0, 1.0, 1.0);
    float specular_exp = 3.0;
    vec3 view_dir = vec3(0.0, 0.0, 1.0);

    // diffuse lighting
    vec3 main_light_dir = normalize((/*modelview_matrix **/ vec4(0.0, -1.0, 0.0, 0.0)).xyz);
    float main_light_power = max(0.0, dot(-main_light_dir, normal));
    vec3 sub_light_dir = vec3(0.0, 1.0, 0.0);
    float sub_light_power = max(0.0, dot(-sub_light_dir, normal));

    vec3 fcolor = color.rgb;
    return vec4((0.25 * fcolor             // ambient
        + 0.75 * fcolor * main_light_power // diffuse(main)
        + 0.25 * fcolor * sub_light_power  // diffuse(sub)
    ), color.a);
}
