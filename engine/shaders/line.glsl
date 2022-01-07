

////// ////// ////// ////// ////// ////// ////// //////
//              Line Shader
////// ////// ////// ////// ////// ////// ////// ////

#type vertex
    #version 400
    in vec4 i_clip_coord;
    in vec4 i_color;
    in vec2 i_texcoord;
    in float i_width;
    in float i_length;

    uniform mat4 viewProjection;

    out vec4 v_clip_coord;
    out vec4 v_color;
    out vec2 v_texcoord;
    out float v_width;
    out float v_length;

    void main(){
        gl_Position = viewProjection * i_clip_coord;
        v_texcoord = i_texcoord;
        v_color = i_color;
        v_width = i_width;
        v_length = i_length;
    }

#type fragment
    #version 400
    in vec4 v_color;
    in vec2 v_texcoord;
    in float v_width;
    in float v_length;

    uniform int u_caps; //0 none, 1 square, 2 round, 3 triangle

    out vec4 frag_color;

    float calculateD(float dx, float dy)
    {
        float d;
        switch(u_caps)
        {
            case 0: d = v_width; break;
            case 1: d = dx > dy ? dx : dy; break;
            case 2: d = sqrt(dx * dx + dy * dy); break;
            case 3: d = dx + dy; break;
        }
        return d;
    }

    void main()
    {
        // if(v_texcoord.x < 0)
        // {
            // float dx = -v_texcoord.x;
            // float dy = v_texcoord.y > 0.0f ? v_texcoord.y : -v_texcoord.y;
            // float d = calculateD(dx, dy);
            // if(d > 0.5f * v_width) discard;
        // }
        // else if (v_texcoord.x > v_length)
        // {
            // float dx = v_texcoord.x - v_length;
            // float dy = v_texcoord.y > 0.0f ? v_texcoord.y : -v_texcoord.y;
            // float d = calculateD(dx, dy);
            // if(d > 0.5f * v_width) discard;
        // }

        frag_color = v_color;
    }
