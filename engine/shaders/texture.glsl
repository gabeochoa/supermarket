

////// ////// ////// ////// ////// ////// ////// //////
//              Texture Shader
////// ////// ////// ////// ////// ////// ////// ////

#type vertex
    #version 400
    in vec3 i_pos;
    in vec4 i_color;
    in vec2 i_texcoord;
    in float i_texindex;
    in float i_tilingfactor;

    uniform mat4 viewProjection;

    out vec2 v_texcoord;
    out vec4 v_color;
    out float v_texindex;
    out float v_tilingfactor;

    void main(){
        gl_Position = viewProjection * vec4(i_pos, 1.0);
        v_texcoord = i_texcoord;
        v_color = i_color;
        v_texindex = i_texindex;
        v_tilingfactor = i_tilingfactor;
    }

#type fragment
    #version 400
    in vec3 position;
    in vec4 v_color;
    in vec2 v_texcoord;
    in float v_texindex;
    in float v_tilingfactor;

    uniform sampler2D u_textures[16]; // check SceneData->Max_Tex

    out vec4 frag_color;
    void main(){
        // Debug texcoord
        // frag_color = vec4(v_texcoord, 0.0, 1.0) * v_color;
        
        vec4 inter = texture(u_textures[int(v_texindex)], v_texcoord * v_tilingfactor);
        // hide anything with basically no alpha
        // if(inter.a < 0.01){ discard; }
        frag_color = inter * v_color;
    }
