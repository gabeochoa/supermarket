

////// ////// ////// ////// ////// ////// ////// //////
//              Texture Shader
////// ////// ////// ////// ////// ////// ////// ////

#type vertex
    #version 400
    in vec3 i_pos;
    in vec2 i_texcoord;
    in vec4 i_color;
    in float i_texindex;

    uniform mat4 viewProjection;

    out vec2 v_texcoord;
    out vec4 v_color;
    out float v_texindex;

    void main(){
        gl_Position = viewProjection * vec4(i_pos, 1.0);
        v_texcoord = i_texcoord;
        v_color = i_color;
    }

#type fragment
    #version 400
    in vec3 position;
    in vec2 v_texcoord;
    in vec4 v_color;
    in float v_texindex;

    uniform float f_tiling;
    uniform sampler2D u_texture[32];

    out vec4 frag_color;
    void main(){
        frag_color = v_color;
        // frag_color = texture(u_texture, v_texcoord * 1.0) * v_color;
    }
