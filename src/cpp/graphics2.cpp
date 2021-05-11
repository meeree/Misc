// Copyright (c), Tamas Csala

#include "oglwrap_example.hpp"

#include <lodepng.h>
#include <oglwrap/oglwrap.h>
#include <oglwrap/shapes/rectangle_shape.h>

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
            srand(time(NULL));
        }

protected:
    virtual void Render() override {
        double t = glfwGetTime();
        std::cout << t << std::endl;

        // Update texture by running simulation.
        int width = 800, height = 800;
        if(t < 1.0)
        {
            int grid[width][height];
            int grid2[width][height];
            unsigned char data[width][height][3];
            for(int i = 0; i < width; ++i)
                for(int j = 0; j < height; ++j)
                    grid[i][j] = grid2[i][j] = false;

            int n_incs = 5;
            #define SET(i, j, r, g, b) {data[(i)][(j)][0] = (r); data[(i)][(j)][1] = (g); data[(i)][(j)][2] = (b);}
            for(int i = 0; i < width; ++i)
                for(int j = 0; j < height; ++j)
                    SET(i, j, 0, 0, 0)

            int nk = 9;
            int rad = width / 4;
            for(int k = 1; k <= nk; ++k)
            {
//                auto f = [rad](int i){return -rad + pow(i,10)/pow(rad,9);};
                auto f = [rad](int i){return -rad + fabs(i);};
                auto g = [rad](int i){return rad - fabs(i);};
//                auto g = [rad](int i){return rad - (2*i*i-i*i*i*i)/float(2*rad-rad*rad*rad);};
                for(int n = 0; n < n_incs; ++n)
                {
                    // Center points.
                    int ic = (rand() + rad) % (width - rad - 1);
                    int jc = (rand() + rad) % (height - rad - 1);
                    if(grid[ic][jc])
                        continue;

                    bool collision = false;
                    for(int i = -rad; i <= rad; ++i)
                        for(int j = f(i); j <= g(i); ++j)
                        {
                            grid2[ic + i][jc + j] = (i + rad) / float(2 * rad) * (j + rad) / float(2 * rad) * UCHAR_MAX;
                            if(grid[ic + i][jc + j])
                            {
                                collision = true;
                                break;
                            }
                        }

                    if(collision)
                        // Reset grid2 in region.
                        for(int i = -rad; i <= rad; ++i)
                            for(int j = f(i); j <= g(i); ++j)
                                grid2[ic + i][jc + j] = 0;
                    else
                        // Copy to grid in region.
                        for(int i = -rad; i <= rad; ++i)
                            for(int j = f(i); j <= g(i); ++j)
                                grid[ic + i][jc + j] = (i + rad) / float(2 * rad) * (j + rad) / float(2 * rad) * UCHAR_MAX;
                }

                int color = (0.2 + 0.8*k / float(nk)) * UCHAR_MAX;
                for(int i = 0; i < width; ++i)
                    for(int j = 0; j < height; ++j)
                    {
                        int g = grid[i][j];
                        if(g) 
                        {
                            int col = color * g / float(UCHAR_MAX);
                            SET(i, j, col, col > UCHAR_MAX / 2 ? col : UCHAR_MAX / 2 - col, UCHAR_MAX - col)
                        }
                    }

                rad /= 1.8;
                n_incs *= 5;

            }
            #undef SET
            
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

