
////// ////// ////// ////// ////// ////// ////// //////
//              Flat Color Shader
////// ////// ////// ////// ////// ////// ////// //////

#type vertex
    #version 400
    in vec3 i_pos;
    uniform mat4 viewProjection;
    uniform mat4 transformMatrix;

    void main(){
        gl_Position = viewProjection * transformMatrix * vec4(i_pos, 1.0);
    }

#type fragment
    #version 400
    in vec3 position;
    uniform vec4 u_color;

    out vec4 frag_color;
    void main(){
        frag_color = vec4(0.3, 0.8, 0.3, 1.0);
        frag_color = u_color;
    }
