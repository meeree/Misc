// Copyright (c), Tamas Csala

#include "oglwrap_example.hpp"

#include <lodepng.h>
#include <oglwrap/oglwrap.h>
#include <oglwrap/shapes/rectangle_shape.h>
#include <vector>
#include <cstdint>
#include <set>
#include "lif.h"
#include <random>

class TexturedSquareExample : public OglwrapExample {
    private:
        std::vector<gl::RectangleShape> rectangle_shapes_;
        std::vector<glm::vec2> translations_;
        gl::Texture2D tex_;
        glm::vec2 scale_;

        std::vector<LIF> lif1_;
        std::vector<LIF> lif2_;
        std::vector<LIF> lif3_;

        int layer_size_;
        std::vector<unsigned char> firing_[3];

        std::vector<std::vector<float>> weights1_, weights2_;

        // A shader program
        gl::Program prog_;

        int loop_idx_ = 0;

    public:
        TexturedSquareExample ()
        {
            std::set<gl::RectangleShape::AttributeType> const attribs = 
                {gl::RectangleShape::kPosition, gl::RectangleShape::kTexCoord};
            rectangle_shapes_.emplace_back(attribs);
            rectangle_shapes_.emplace_back(attribs);
            rectangle_shapes_.emplace_back(attribs);
            translations_ = {{-0.5, 0}, {0.0, 0.0}, {0.5, 0.0}};

            layer_size_ = 100;
            firing_[0].resize(layer_size_, false);
            firing_[1].resize(layer_size_, false);
            firing_[2].resize(layer_size_, false);
            lif1_.resize(layer_size_);
            lif2_.resize(layer_size_);
            lif3_.resize(layer_size_);

            weights1_.resize(layer_size_, std::vector<float>(layer_size_, 1));
            weights2_.resize(layer_size_, std::vector<float>(layer_size_, 1));

            std::random_device rd; 
            std::mt19937 gen(rd()); 
            std::uniform_real_distribution<float> dist(0, 0.5);
            for(int i = 0; i < layer_size_; ++i)
            {
                for(int j = 0; j < layer_size_; ++j)
                {
                    weights1_[i][j] = dist(gen);
                    weights2_[i][j] = dist(gen);
                }
            }

            // We need to add a few more lines to the shaders
            gl::ShaderSource vs_source;
            vs_source.set_source(R"""(
  #version 330 core
  in vec2 pos;
  in vec2 inTexCoord;

  uniform vec2 scale;
  uniform vec2 translate;
  out vec2 texCoord;

  void main() {
    texCoord = inTexCoord;

    // Shrink the full screen rectangle to a smaller size
    gl_Position = vec4(pos * scale + translate, 0, 1);
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
    if(fragColor.r < 0.1)
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    else if(fragColor.r < 0.6)
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    else
        fragColor = vec4(0.0, 0.0, 1.0, 1.0);
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

            // Enable alpha blending
            gl::Enable(gl::kBlend);
            gl::BlendFunc(gl::kSrcAlpha, gl::kOneMinusSrcAlpha);

            // Set the clear color to white
            gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);

            scale_ = {0.1, 1};
            gl::Uniform<glm::vec2>(prog_, "scale") = scale_;
        }

    protected:
        virtual void Render() override 
        {
            for(int i = 0; i < lif1_.size(); ++i)
                firing_[0][i] = lif1_[i].Simulate(0.1, 3*(rand() % 100) / 100.0) * UCHAR_MAX;

            for(int i = 0; i < lif2_.size(); ++i)
            {
                float input = 0; 
                for(int j = 0; j < lif1_.size(); ++j)
                    if(firing_[0][j]) 
                        input += weights1_[i][j];
                firing_[1][i] = lif2_[i].Simulate(0.1, input) * UCHAR_MAX;
            }

            for(int i = 0; i < lif3_.size(); ++i)
            {
                float input = 0; 
                for(int j = 0; j < lif2_.size(); ++j)
                    if(firing_[1][j]) 
                        input += weights2_[i][j];
                firing_[2][i] = lif3_[i].Simulate(0.1, input) * UCHAR_MAX;
            }

            for(int i = 0; i < translations_.size(); ++i)
            {
                auto& shape = rectangle_shapes_[i];
                auto& trans = translations_[i];

                gl::Bind(tex_);
                tex_.upload(gl::kRed, layer_size_, 1,
                        gl::kRed, gl::kUnsignedByte, firing_[i].data());
                tex_.minFilter(gl::kNearest);
                tex_.magFilter(gl::kNearest);

                gl::Uniform<glm::vec2>(prog_, "translate") = trans;
                shape.render();
            }

            loop_idx_ += 1;
        }
};

int main() {
    TexturedSquareExample().RunMainLoop();
}
