// Copyright (c), Tamas Csala

#include "oglwrap_example.hpp"

#include <oglwrap/oglwrap.h>
#include "custom_shape.h"
#include <oglwrap/shapes/cube_shape.h>
#include <glm/gtc/matrix_transform.hpp>
#include <lodepng.h>

#include "voxels.h"
#include "player.h"

class Graphics : public OglwrapExample {
    private:
        int Ngrid;
        float scale_;
        float off_;
        Voxels voxels;
        Mesh mesh;

        // A shader program
        gl::Program prog_;

        gl::Texture2D tex_;

        glm::vec2 camAng = {0, -M_PI_2};
        glm::vec3 camForward = {1, 0, 0};

        std::vector<MeshPoint> points_;
        std::vector<unsigned> indices_;

        float t_click_prev_ = 0;

        Player player_;

    public:
        Graphics ()
            : Ngrid{100}, 
            scale_{2 / float(Ngrid)}, off_{1.0f},
            voxels(Ngrid, Ngrid, Ngrid), player_(glm::vec3(0.5,20,0))

        {
            for(int z = 1; z < voxels.Nz - 1; ++z)
                for(int y = 1; y < voxels.Ny - 1; ++y)
                    for(int x = 1; x < voxels.Nx - 1; ++x)
                    {
                        if(cos(x / float(Ngrid) * M_PI * 8) * sin(z / float(Ngrid) * M_PI * 8) > y / float(Ngrid) * 2 - 1)
                            voxels.data[voxels.GetIdx(x,y,z)] = eWood;
                    }

            // We need to add a few more lines to the shaders
            gl::ShaderSource vs_source;
            vs_source.set_source(R"""(
      #version 330 core
      layout (location=0) in vec3 inPos;
      layout (location=1) in vec3 inNormal;
      layout (location=2) in vec2 inUV;
      layout (location=3) in int inTexId;

      uniform mat4 mvp;

      out vec3 normal;
      out vec3 position;
      out vec2 uv;
      flat out int tex_id;

      void main() {
        normal = inNormal;
        position = inPos;
        uv = inUV;
        tex_id = inTexId;
        gl_Position = mvp * vec4(inPos, 1.0);
      })""");
            vs_source.set_source_file("example_shader.vert");
            gl::Shader vs(gl::kVertexShader, vs_source);

            gl::ShaderSource fs_source;
            fs_source.set_source(R"""(
      #version 330 core
      in vec3 normal;
      in vec3 position;
      in vec2 uv;
      flat in int tex_id;

      uniform vec3 lightPos1;
      uniform sampler2D tex;
      uniform int sprite_dim;

      out vec4 fragColor;

      void main() {
        vec3 lightVec1 = normalize(lightPos1 - position);
        float d1 = abs(dot(lightVec1, normal)); // Note the abs means normal sign does not matter.
        vec3 c1 = vec3(1);  

        // Retrieve bottom left coordinate of sprite from sprite sheet.
        vec2 sprite_coords = vec2(tex_id % sprite_dim, tex_id / sprite_dim + 1);
        sprite_coords = (sprite_coords + vec2(uv.x, -uv.y)) / float(sprite_dim);
        vec3 tex_col = vec3(texture(tex, sprite_coords));
		vec3 ambient = 0.8 * vec3(1.0);
        fragColor = vec4(tex_col * (ambient + d1 * c1), 1.0);
      })""");
            fs_source.set_source_file("example_shader.frag");
            gl::Shader fs(gl::kFragmentShader, fs_source);

            // Create a shader program
            prog_.attachShader(vs);
            prog_.attachShader(fs);
            prog_.link();
            gl::Use(prog_);

            // Bind the attribute locations
            (prog_ | "inPos").bindLocation(0);
            (prog_ | "inNormal").bindLocation(1);
            (prog_ | "inUV").bindLocation(2);

            gl::Enable(gl::kDepthTest);

            // Set the clear color
            gl::ClearColor(0.1f, 0.2f, 0.3f, 1.0f);

            // Setup texture.
            {
                gl::Bind(tex_);
                unsigned width, height;
                std::vector<unsigned char> data;
                std::string path = "../minecraft_sprites_real.png";
                unsigned error = lodepng::decode(data, width, height, path, LCT_RGBA, 8);
                if (error) {
                    std::cerr << "Image decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
                    std::terminate();
                }
                tex_.upload(gl::kSrgb8Alpha8, width, height,
                        gl::kRgba, gl::kUnsignedByte, data.data());
                tex_.generateMipmap();

                // Want to have some fun? Change kNearest to KLinear. 
                tex_.minFilter(gl::kNearest);
                tex_.magFilter(gl::kNearest);
                tex_.wrapS(gl::kRepeat);
                tex_.wrapT(gl::kRepeat);
            }

            gl::Uniform<int>(prog_, "sprite_dim") = 32;

            // Disable cursor.
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetInputMode(window_, GLFW_STICKY_KEYS, false);

            // Set callbacks.
            glfwSetMouseButtonCallback(window_, MouseDiscreteCallback);

            // Set user pointer. This is used in the callbacks, which are static.
            glfwSetWindowUserPointer(window_, this);

            #pragma omp parallel for 
            for(int z = 1; z < voxels.Nz - 1; ++z)
                for(int y = 1; y < voxels.Ny - 1; ++y)
                    for(int x = 1; x < voxels.Nx - 1; ++x)
                    {
                        if(voxels.Nz * (1 + cos(y / 10)) / 4 > z)
                            voxels.data[voxels.GetIdx(x,y,z)] = eWood;
                        else if(voxels.Nz * (1 + cos(y / 10)) / 2 > z)
                            voxels.data[voxels.GetIdx(x,y,z)] = ePumpkin;
                        else 
                            voxels.data[voxels.GetIdx(x,y,z)] = eAir;
                    }
        }

    protected:
        virtual void Render() override {
            float t = glfwGetTime();

            Voxelize(voxels, indices_, points_);
            for(auto& pt: points_)
                pt.pos = pt.pos * scale_ - off_;

            // Player position update.
            glm::vec3 const& pos = player_.GetPos();

            // Collision detect with voxels if in grid. Otherwise, apply gravity/reset to top.
            int x_vox = (pos.x + off_) / scale_;
            int y_vox = (pos.y + off_) / scale_;
            int z_vox = (pos.z + off_) / scale_;
            if(y_vox > Ngrid - 1)
                player_.ApplyGravity(0.1, 1);
            else if(y_vox < 4)
                player_.SetPos(glm::vec3(0.5, 10, 0));  
            else 
            {
                if(voxels.data[voxels.GetIdx(x_vox, y_vox-4, z_vox)] == eAir)
                    player_.ApplyGravity(0.01, 1);
                else if(voxels.data[voxels.GetIdx(x_vox, y_vox, z_vox)] == eWood || voxels.data[voxels.GetIdx(x_vox, y_vox-1, z_vox)] == ePumpkin)
                    player_.ApplyGravity(-0.1, 1);
            }

            glm::mat4 camera_mat = glm::lookAt(player_.GetPos(), player_.GetPos() + camForward, glm::vec3{0.0f, 1.0f, 0.0f});
            glm::mat4 model_mat = glm::rotate(glm::mat4(1.0f), 0 * glm::radians(t) * 100, glm::vec3(0,1,0));
            glm::mat4 proj_mat = glm::perspectiveFov<float>(M_PI/3.0, kScreenWidth, kScreenHeight, 0.001, 100);
            gl::Uniform<glm::mat4>(prog_, "mvp") = proj_mat * camera_mat * model_mat;

            glm::vec3 lightPos1 = player_.GetPos();
            gl::Uniform<glm::vec3>(prog_, "lightPos1") = lightPos1;

            mesh.Set(&points_, &indices_);
            mesh.Render();
            HandleKeys();
            HandleMouse();
        }

        // Only called when mouse events (release/press) happen. This is good when you 
        // don't want to register multiple clicks when the user clicks once.
        static void MouseDiscreteCallback(GLFWwindow* window, int button, int action, int mods)
        {
//            Graphics* g = static_cast<Graphics*>(glfwGetWindowUserPointer(window));
        }

        void HandleMouse()
        {
            double xpos, ypos;
            glfwGetCursorPos(window_, &xpos, &ypos);

            // Limit the number of clicks possible per second. 
//            float t = glfwGetTime();
//            if(t - t_click_prev_ < 0.1)
//                return;

            // Convert xpos, ypos from screen coordinates to [-1,1] coordinates.
            glm::vec2 wpos = {(float)xpos, (float)ypos};
            wpos = glm::vec2(2) * wpos / glm::vec2(kScreenWidth, -kScreenHeight);
            wpos += glm::vec2(-1, 1);

            camAng += wpos * 0.2f;
            camForward = {cos(camAng.x) * cos(camAng.y), 
                       sin(camAng.y), 
                       sin(camAng.x) * cos(camAng.y)};

            if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                glm::ivec3 pos_vox = (player_.GetPos() + off_) / scale_;
                glm::ivec3 vox_prev;
                auto hit_pos = voxels.CastRay(pos_vox, camForward, vox_prev);
                SetSquare(voxels, hit_pos, 4);
            }

            if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            {
                glm::ivec3 pos_vox = (player_.GetPos() + off_) / scale_;
                glm::ivec3 vox_prev;
                auto hit_pos = voxels.CastRay(pos_vox, camForward, vox_prev);
                SetSquare(voxels, vox_prev, 1, ePumpkin);
            }

//            t_click_prev_ = t;
            glfwSetCursorPos(window_, kScreenWidth / 2, kScreenHeight / 2);
        }

        void HandleKeys()
        {
            float moveSpeed = 0.01;

            bool gW = glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS;
            bool gA = glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS;
            bool gS = glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS;
            bool gD = glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS;
            if(gW | gA | gS | gD)
            {
                int dir = (gW | gD) ? 1 : -1;
                float ang_off_ = (gW | gS) ? 0 : M_PI_2;
                player_.SetPos(player_.GetPos() + dir * moveSpeed * 
                        glm::vec3(cos(camAng.x + ang_off_), 0, sin(camAng.x + ang_off_)));
            }
        }

};

int main() {
    Graphics().RunMainLoop();
}

