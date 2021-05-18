// Copyright (c), Tamas Csala

#include "oglwrap_example.hpp"

#include <lodepng.h>
#include <oglwrap/oglwrap.h>
#include <oglwrap/shapes/rectangle_shape.h>

enum Block 
{
    eAir=0, eGround, eWater, eCount
};

class TexturedSquareExample : public OglwrapExample {
private:
    // Defines a full screen rectangle (see oglwrap/shapes/rectangle_shape.h)
    gl::RectangleShape rectangle_shape_;

    // A shader program
    gl::Program prog_;

    // A 2D texture
    gl::Texture2D tex_;
    static int constexpr width = 100, height = 100;
    unsigned char data[height][width][3];
    Block blocks[height][width];
    Block blocks2[height][width];

public:
    TexturedSquareExample ()
        // Now we need texture coordinates too, not just position
        : rectangle_shape_({gl::RectangleShape::kPosition,
                gl::RectangleShape::kTexCoord})
        {
            // We need to add a few more lines to the shaders
            gl::ShaderSource vs_source;
            vs_source.set_source(R"""(
  #version 330 core
  in vec2 pos;
  in vec2 inTexCoord;

  out vec2 texCoord;

  void main() {
    texCoord = inTexCoord;

    // Shrink the full screen rectangle to a smaller size
    gl_Position = vec4(pos.x, pos.y, 0, 1);
  })""");
            vs_source.set_source_file("example_shader.vert");
            gl::Shader vs(gl::kVertexShader, vs_source);

            gl::ShaderSource fs_source;
            fs_source.set_source(R"""(
  #version 330 core
  in vec2 texCoord;
  uniform sampler2D tex;

  out vec4 fragColor;

  void main() {
    fragColor = texture(tex, vec2(texCoord.y, texCoord.x));
  })""");
            fs_source.set_source_file("example_shader.frag");
            gl::Shader fs(gl::kFragmentShader, fs_source);

            // Create a shader program
            prog_.attachShader(vs);
            prog_.attachShader(fs);
            prog_.link();
            gl::Use(prog_);

            // Bind the attribute positions to the locations that the RectangleShape uses
            (prog_ | "pos").bindLocation(gl::RectangleShape::kPosition);
            (prog_ | "inTexCoord").bindLocation(gl::RectangleShape::kTexCoord);

            // Set the texture uniform
            gl::UniformSampler(prog_, "tex") = 0;

            // Set the clear color to white
            gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);

            for(int i = 0; i < width; ++i)
                for(int j = 0; j < height; ++j)
                    blocks[i][j] = (i + j < 50 && 20 - i + j < 30 && i < 20 && i > 10) || (width - 1 - i) + j < 60 ? eGround : eAir;

            for(int i = 0; i < 10; ++i)
                blocks[i + 20][height-1] = eWater;
        }

protected:
    virtual void Render() override {
        double t = glfwGetTime();
        std::cout << t << std::endl;

        // Update texture by running simulation.
        gl::Bind(tex_);

        for(int i = 0; i < width; ++i)
            for(int j = 0; j < height; ++j)
                blocks2[i][j] = blocks[i][j];

        for(int i = 0; i < width; ++i)
        {
            for(int j = 0; j < height; ++j)
            {
                auto b = blocks[i][j];
                if(b == eWater)
                {
                    if(j > 0)
                    {
                        if(i < width - 1 && blocks[i+1][j] == eAir && blocks[i+1][j-1] == eGround)
                            blocks2[i+1][j] = eWater;
                        
                        if(i > 0 && blocks[i-1][j] == eAir && blocks[i-1][j-1] == eGround)
                            blocks2[i-1][j] = eWater;

                        if(blocks[i][j-1] == eAir)
                            blocks2[i][j-1] = eWater;
                        else if(blocks[i][j-1] == eGround)
                        {
                            if(i < width - 1 && blocks[i+1][j] == eAir)
                                blocks2[i+1][j] = eWater;
                            if(i > 0 && blocks[i-1][j] == eAir)
                                blocks2[i-1][j] = eWater;
                        }
                    }
                }
            }
        }

        for(int i = 0; i < width; ++i)
        {
            for(int j = 0; j < height; ++j)
            {
                switch(blocks2[i][j])
                {
                    case eAir:
                        data[i][j][0] = 0.1 * UCHAR_MAX;
                        data[i][j][1] = 0.1 * UCHAR_MAX;
                        data[i][j][2] = 0.5 * UCHAR_MAX;
                        break;
                    case eGround:
                        data[i][j][0] = 0.7 * UCHAR_MAX;
                        data[i][j][1] = 0.7 * UCHAR_MAX;
                        data[i][j][2] = 0.7 * UCHAR_MAX;
                        break;
                    case eWater:
                        data[i][j][0] = 0.3 * UCHAR_MAX;
                        data[i][j][1] = 0.3 * UCHAR_MAX;
                        data[i][j][2] = UCHAR_MAX;
                        break;
                }
            }
        }
        

        tex_.upload(gl::kRgb, width, height,
                gl::kRgb, gl::kUnsignedByte, data);
        tex_.minFilter(gl::kNearest);
        tex_.magFilter(gl::kNearest);

        rectangle_shape_.render();
        std::swap(blocks, blocks2);
    }
};

int main() {
    TexturedSquareExample().RunMainLoop();
}

