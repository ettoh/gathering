#version 460

in vec4 f_color;
in vec4 f_normal;
in vec3 f_coord3d;
in vec2 f_texcoord;

out vec4 output_color;

layout (std140) uniform Global{	
    mat4 view;
    mat4 projection;
    vec3 light_source;
};

void main(void) {
    float occ = f_color.a;
    float ambient = 0.2;
    vec3 N = normalize(f_normal).xyz;
    vec3 L = light_source - f_coord3d; // light source
    L = normalize(L);

    // shade objects from inside the same as from outside
    float lambertian_outside = max(dot(N, L), 0.0);
    float lambertian_inside = max(dot(-N, L), 0.0);
    float lambertian = max(lambertian_outside, lambertian_inside);

    vec3 diffuse_color = f_color.xyz;
    vec3 light_color = vec3(1.0) * 0.75;

    vec3 color = vec3(0.0);
    color += diffuse_color * ambient; // ambient
    color += diffuse_color * lambertian * light_color; // diffuse
    output_color = vec4(color, occ);    
}
