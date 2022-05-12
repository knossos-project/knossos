#version 120

attribute vec3 vertex;

uniform vec4 viewport;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

//varying float radius;
//varying vec2  center;

void main() {
    mat4 mvp_matrix = projection_matrix * modelview_matrix;
    gl_Position = mvp_matrix * vec4(vertex, 1.0);
}

//#version 120

//uniform vec4 viewport;

//attribute vec3 vertex;

//uniform mat4 modelview_matrix;
//uniform mat4 projection_matrix;

//varying mat4 VPMTInverse;
//varying mat4 VPInverse;
//varying vec3 centernormclip;

//void main() {

//	mat4 mvp = modelview_matrix * projection_matrix;
//	gl_Position   = mvp * vec4(vertex, 1.0);
//	gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);//gl_Color;

//	float R = 1.0;

//    mat4 T = mat4(
//            1.0, 0.0, 0.0, 0.0,
//            0.0, 1.0, 0.0, 0.0,
//            0.0, 0.0, 1.0, 0.0,
//	        vertex.x/R, vertex.y/R, vertex.z/R, 1.0/R);

//	mat4 PMTt = transpose(mvp * T);//transpose(gl_ModelViewProjectionMatrix * T);

//    vec4 r1 = PMTt[0];
//    vec4 r2 = PMTt[1];
//    vec4 r4 = PMTt[3];
//    float r1Dr4T = dot(r1.xyz,r4.xyz)-r1.w*r4.w;
//    float r1Dr1T = dot(r1.xyz,r1.xyz)-r1.w*r1.w;
//    float r4Dr4T = dot(r4.xyz,r4.xyz)-r4.w*r4.w;
//    float r2Dr2T = dot(r2.xyz,r2.xyz)-r2.w*r2.w;
//    float r2Dr4T = dot(r2.xyz,r4.xyz)-r2.w*r4.w;

//    gl_Position = vec4(-r1Dr4T, -r2Dr4T, gl_Position.z/gl_Position.w*(-r4Dr4T), -r4Dr4T);


//    float discriminant_x = r1Dr4T*r1Dr4T-r4Dr4T*r1Dr1T;
//    float discriminant_y = r2Dr4T*r2Dr4T-r4Dr4T*r2Dr2T;
//    float screen = max(float(viewport.z), float(viewport.w));

//    gl_PointSize = sqrt(max(discriminant_x, discriminant_y))*screen/(-r4Dr4T);

//    // prepare varyings

//    mat4 TInverse = mat4(
//            1.0,          0.0,          0.0,         0.0,
//            0.0,          1.0,          0.0,         0.0,
//            0.0,          0.0,          1.0,         0.0,
//	        -vertex.x, -vertex.y, -vertex.z, R);
//    mat4 VInverse = mat4( // TODO: move this one to CPU
//            2.0/float(viewport.z), 0.0, 0.0, 0.0,
//            0.0, 2.0/float(viewport.w), 0.0, 0.0,
//            0.0, 0.0,                   2.0/gl_DepthRange.diff, 0.0,
//            -float(viewport.z+2.0*viewport.x)/float(viewport.z), -float(viewport.w+2.0*viewport.y)/float(viewport.w), -(gl_DepthRange.near+gl_DepthRange.far)/gl_DepthRange.diff, 1.0);
//	VPMTInverse = TInverse*gl_ModelViewProjectionMatrixInverse*VInverse;
//	//VPMTInverse = TInverse * mvp * VInverse;
//	VPInverse = gl_ProjectionMatrixInverse*VInverse; // TODO: move to CPU
//	vec4 centerclip = modelview_matrix * vec4(vertex, 1.0);;
//    centernormclip = vec3(centerclip)/centerclip.w;
//}
