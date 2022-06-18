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
    output_color = f_color;  
}
