

////// ////// ////// ////// ////// ////// ////// //////
//              Texture Shader
////// ////// ////// ////// ////// ////// ////// ////

#type vertex
    #version 400
    in vec3 i_pos;
    in vec4 i_color;
    in vec2 i_texcoord;

    uniform mat4 viewProjection;

    out vec2 v_texcoord;
    out vec4 v_color;

    void main(){
        gl_Position = viewProjection * vec4(i_pos, 1.0);
        v_color = i_color;
        v_texcoord = i_texcoord;
    }

#type fragment
    #version 400
    in vec3 position;
    in vec4 v_color;
    in vec2 v_texcoord;

    uniform float f_tiling;
    uniform vec4 u_color;
    uniform sampler2D u_texture;

    out vec4 color;
    void main(){
        // Debug texcoord
        color = vec4(v_texcoord, 0.0, 1.0);
        
       
        // hide anything with basically no alpha 
        /*
            vec4 inter = texture(u_texture, v_texcoord * f_tiling);
            if(inter.a < 0.01){ discard; }
            color = inter * v_color;
        */
    }
