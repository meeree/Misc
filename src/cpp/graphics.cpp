// Copyright (c), Tamas Csala

#include "oglwrap_example.hpp"

#include <lodepng.h>
#include <oglwrap/oglwrap.h>
#include <oglwrap/shapes/rectangle_shape.h>

#include "simulate.h"

class TexturedSquareExample : public OglwrapExample {
private:
    // Defines a full screen rectangle (see oglwrap/shapes/rectangle_shape.h)
    gl::RectangleShape rectangle_shape_;

    // A shader program
    gl::Program prog_;

    // A 2D texture
    gl::Texture2D tex_;
    unsigned width_, height_;

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
    fragColor = texture(tex, vec2(texCoord.x, 1-texCoord.y));
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

            width_ = 100;
            height_ = 100;
        }

protected:
    virtual void Render() override {
        double t = glfwGetTime();
        std::cout << t << std::endl;

        // Update texture by running simulation.
        int width = 800, height = 800;
        Object a1, a2, a3, a4, a5, a6;
        a1.p.x = width/2  +  width/4 * cos(0);
        a1.p.y = height/2 + height/4 * sin(0);
        a2.p.x = width/2  +  width/4 * cos(2 * M_PI / 3);
        a2.p.y = height/2 + height/4 * sin(2 * M_PI / 3);
        a3.p.x = width/2  +  width/4 * cos(4 * M_PI / 3);
        a3.p.y = height/2 + height/4 * sin(4 * M_PI / 3);
        a4.p.x = width/2 + sin(t/3) * width/10;
        a4.p.y = height/2;
//        a4.p.x = width/2 + width/30 * cos(2 * M_PI * t / 7);
//        a4.p.y = height/2 + height/30 * sin(2 * M_PI * t / 7);
        a5.p.x = width/2 + width/10 * cos(M_PI + 2 * M_PI * t / 10);
        a5.p.y = height/2 + height/10 * sin(M_PI + 2 * M_PI * t / 10);
        a6.p.x = width/2;
        a6.p.y = height/2 + fabs(sin(t/10)) * height/20;
        a6.p.x = width/2 + width/30 * cos(M_PI/3 + 2 * M_PI * t / 5);
        a6.p.y = height/2 + height/30 * sin(M_PI/3 + 2 * M_PI * t / 5);

        a1.m = 1e-2;
        a2.m = 1e-2;
        a3.m = 1e-2;
        a4.m = 1e5;
        a5.m = 1;
        a6.m = 1;
        {
            gl::Bind(tex_);
            unsigned char data[height][width][3];
            #pragma omp parallel for
            for(int i = 0; i < height; ++i)
            {
                for(int j = 0; j < width; ++j)
                {
                    Object b; 
                    b.m = 0.1;
                    b.p = {i, j};
                    int min_idx = 0;
                    int t = 0;
                    int n_incs = 4e1;
                    for(int t = 0; t < n_incs; ++t)
                    {
                        Simulate(b, a1, a2, a3, a4, a5, a6);
                        float d1{Dist(b, a1)}, d2{Dist(b, a2)}, d3{Dist(b, a3)};
                        min_idx = 0; 
                        float min_dist = d1;
                        if(d2 < min_dist)
                        {
                            min_dist = d2;
                            min_idx = 1;
                        }
                        if(d3 < min_dist)
                        {
                            min_dist = d3;
                            min_idx = 2;
                        }

                        if(min_dist < width/40)
                            break;
                    }

                    data[i][j][0] = UCHAR_MAX;
                    data[i][j][1] = UCHAR_MAX;
                    data[i][j][2] = UCHAR_MAX;
                    if(t < n_incs)
                    {
						if(t > 0)
							std::cout << t << std::endl;
                        data[i][j][min_idx] = (1 - t / float(n_incs)) * UCHAR_MAX;
                        data[i][j][(min_idx+1)%3] = 0; 
                        data[i][j][(min_idx+2)%3] = 0; 
                    }
                }
            }

            #pragma omp parallel for
            for(int i = 0; i < height/20; ++i)
                for(int j = 0; j < width/20; ++j)
                {
                    data[int(a4.p.x) + i - height/40][int(a4.p.y) + j - width/40][0] = UCHAR_MAX;
                    data[int(a4.p.x) + i - height/40][int(a4.p.y) + j - width/40][1] = UCHAR_MAX;
                    data[int(a4.p.x) + i - height/40][int(a4.p.y) + j - width/40][2] = UCHAR_MAX;

//						data[int(a5.p.x) + i - height/40][int(a5.p.y) + j - width/40][0] = UCHAR_MAX;
//						data[int(a5.p.x) + i - height/40][int(a5.p.y) + j - width/40][1] = UCHAR_MAX;
//						data[int(a5.p.x) + i - height/40][int(a5.p.y) + j - width/40][2] = UCHAR_MAX;
//
//						data[int(a6.p.x) + i - height/40][int(a6.p.y) + j - width/40][0] = UCHAR_MAX;
//						data[int(a6.p.x) + i - height/40][int(a6.p.y) + j - width/40][1] = UCHAR_MAX;
//						data[int(a6.p.x) + i - height/40][int(a6.p.y) + j - width/40][2] = UCHAR_MAX;
                }
            
            tex_.upload(gl::kRgb, width, height,
                    gl::kRgb, gl::kUnsignedByte, data);
            tex_.minFilter(gl::kLinear);
            tex_.magFilter(gl::kLinear);
        }

        rectangle_shape_.render();
    }
};

int main() {
    TexturedSquareExample().RunMainLoop();
}

