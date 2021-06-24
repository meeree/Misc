// Copyright (c), Tamas Csala

#include "oglwrap_example.hpp"

#include <oglwrap/oglwrap.h>
#include "custom_shape.h"
#include <oglwrap/shapes/cube_shape.h>
#include <glm/gtc/matrix_transform.hpp>
#include <lodepng.h>

#include "voxels.h"
#include "player.h"
#include "perlin.h"
#include "pet.h"

void CreatePetMesh(int v, 
         std::vector<MeshPoint>& points, std::vector<unsigned>& inds)
{
    int spid = g_blockFaceSpriteLookup[v*6+0];
    points.emplace_back(-0.5f, -0.5f, -0.5f, -1, 0, 0, 0, 0, spid);
    points.emplace_back(-0.5f, -0.5f, +0.5f, -1, 0, 0, 1, 0, spid);
    points.emplace_back(-0.5f, +0.5f, +0.5f, -1, 0, 0, 1, 1, spid);
    points.emplace_back(-0.5f, +0.5f, -0.5f, -1, 0, 0, 0, 1, spid);

    spid = g_blockFaceSpriteLookup[v*6+1];
    points.emplace_back(+0.5f, -0.5f, -0.5f, 1, 0, 0, 0, 0, spid);
    points.emplace_back(+0.5f, -0.5f, +0.5f, 1, 0, 0, 1, 0, spid);
    points.emplace_back(+0.5f, +0.5f, +0.5f, 1, 0, 0, 1, 1, spid);
    points.emplace_back(+0.5f, +0.5f, -0.5f, 1, 0, 0, 0, 1, spid);

    spid = g_blockFaceSpriteLookup[v*6+2];
    points.emplace_back(-0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 0, spid); 
    points.emplace_back(-0.5f, -0.5f, +0.5f, 0, -1, 0, 0, 1, spid); 
    points.emplace_back(+0.5f, -0.5f, +0.5f, 0, -1, 0, 1, 1, spid); 
    points.emplace_back(+0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 0, spid); 

    spid = g_blockFaceSpriteLookup[v*6+3];
    points.emplace_back(-0.5f, +0.5f, -0.5f, 0, 1, 0, 0, 0, spid);
    points.emplace_back(-0.5f, +0.5f, +0.5f, 0, 1, 0, 0, 1, spid);
    points.emplace_back(+0.5f, +0.5f, +0.5f, 0, 1, 0, 1, 1, spid);
    points.emplace_back(+0.5f, +0.5f, -0.5f, 0, 1, 0, 1, 0, spid);

    spid = g_blockFaceSpriteLookup[v*6+4];
    points.emplace_back(-0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 0, spid); 
    points.emplace_back(-0.5f, +0.5f, -0.5f, 0, 0, -1, 0, 1, spid); 
    points.emplace_back(+0.5f, +0.5f, -0.5f, 0, 0, -1, 1, 1, spid); 
    points.emplace_back(+0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 0, spid); 

    spid = g_blockFaceSpriteLookup[v*6+5];
    points.emplace_back(-0.5f, -0.5f, +0.5f, 0, 0, 1, 0, 0, spid); 
    points.emplace_back(-0.5f, +0.5f, +0.5f, 0, 0, 1, 0, 1, spid); 
    points.emplace_back(+0.5f, +0.5f, +0.5f, 0, 0, 1, 1, 1, spid); 
    points.emplace_back(+0.5f, -0.5f, +0.5f, 0, 0, 1, 1, 0, spid); 

    // Indices are the same for each square.
    int inds_single[6]{0, 1, 2, 2, 3, 0};
    for(size_t i = 0; i < points.size() / 4; ++i)
    {
        inds.insert(inds.end(), inds_single, inds_single + 6);
        for(int& i: inds_single)
            i += 4;
    }
}

class Graphics : public OglwrapExample {
    private:
        int Ngrid;
        float scale_;
        float off_;
        Voxels voxels_read;
        Voxels voxels_write;

        // A shader program
        gl::Program prog_;

        gl::Texture2D tex_;

        glm::vec2 camAng = {0, -M_PI_2};
        glm::vec3 camForward = {1, 0, 0};

        Mesh mesh;
        std::vector<MeshPoint> points_;
        std::vector<unsigned> indices_;

        Mesh screenMesh;
        std::vector<MeshPoint> crossHairPoints_;
        std::vector<unsigned> crossHairInds_;

        Mesh petMesh;
        std::vector<MeshPoint> petMeshPoints_;
        std::vector<unsigned> petMeshInds_;
        Pet pet;

        Mesh skyBoxMesh;
        std::vector<MeshPoint> skyBoxMeshPoints_;
        std::vector<unsigned> skyBoxMeshInds_;

        float grav_const = 4;
        bool player_grounded_ = false;

        float t_click_prev_ = 0;

        Player player_;
        bool need_voxels_update_ = true;

        float sun_ang_ = 0.0;

    public:
        Graphics ()
            : Ngrid{64}, 
            scale_{2 / float(Ngrid)}, off_{1.0f},
            voxels_read(Ngrid, Ngrid, Ngrid), voxels_write(Ngrid, Ngrid, Ngrid),
            player_(glm::vec3(0.5,5,0)),
            pet(glm::vec3(0.5, 5, 0), 0.01)
        {
            std::ofstream perlin_out("../../PERLIN.txt");
            srand(100*time(NULL));
            auto grid = Perlin3D().Test(4, 16);

            crossHairPoints_.emplace_back(-0.02 , -0.002, 0, -1, 0, 0, 0, 0, 602);
            crossHairPoints_.emplace_back(+0.02 , -0.002, 0, -1, 0, 0, 1, 0, 602);
            crossHairPoints_.emplace_back(+0.02 , +0.002, 0, -1, 0, 0, 1, 1, 602);
            crossHairPoints_.emplace_back(-0.02 , +0.002, 0, -1, 0, 0, 0, 1, 602);
            crossHairPoints_.emplace_back(-0.002, -0.02 , 0, -1, 0, 0, 0, 0,  602);
            crossHairPoints_.emplace_back(-0.002, +0.02 , 0, -1, 0, 0, 1, 0,  602);
            crossHairPoints_.emplace_back(+0.002, +0.02 , 0, -1, 0, 0, 1, 1,  602);
            crossHairPoints_.emplace_back(+0.002, -0.02 , 0, -1, 0, 0, 0, 1,  602);
            crossHairInds_ = {0,1,2,2,3,0, 4,5,6,6,7,4};

            float max_dim = std::max(kScreenWidth, kScreenHeight);
            glm::vec3 aspect_scale{kScreenWidth / max_dim, kScreenHeight / max_dim, 1};
            for(auto& p: crossHairPoints_)
                p.pos /= aspect_scale;

            screenMesh.Set(&crossHairPoints_, &crossHairInds_);

            skyBoxMeshPoints_.emplace_back(-10, -10, 99, -1, 0, 0, 0, 0, 603);
            skyBoxMeshPoints_.emplace_back(+10, -10, 99, -1, 0, 0, 1, 0, 603);
            skyBoxMeshPoints_.emplace_back(+10, +10, 99, -1, 0, 0, 1, 1, 603);
            skyBoxMeshPoints_.emplace_back(-10, +10, 99, -1, 0, 0, 0, 1, 603);
            skyBoxMeshPoints_.emplace_back(-10, -10, 99, -1, 0, 0, 0, 0, 603);
            skyBoxMeshPoints_.emplace_back(-10, +10, 99, -1, 0, 0, 1, 0, 603);
            skyBoxMeshPoints_.emplace_back(+10, +10, 99, -1, 0, 0, 1, 1, 603);
            skyBoxMeshPoints_.emplace_back(+10, -10, 99, -1, 0, 0, 0, 1, 603);

            skyBoxMeshPoints_.emplace_back(-20, -20, 100, -1, 0, 0, 0, 0, 604);
            skyBoxMeshPoints_.emplace_back(+20, -20, 100, -1, 0, 0, 1, 0, 604);
            skyBoxMeshPoints_.emplace_back(+20, +20, 100, -1, 0, 0, 1, 1, 604);
            skyBoxMeshPoints_.emplace_back(-20, +20, 100, -1, 0, 0, 0, 1, 604);
            skyBoxMeshPoints_.emplace_back(-20, -20, 100, -1, 0, 0, 0, 0, 604);
            skyBoxMeshPoints_.emplace_back(-20, +20, 100, -1, 0, 0, 1, 0, 604);
            skyBoxMeshPoints_.emplace_back(+20, +20, 100, -1, 0, 0, 1, 1, 604);
            skyBoxMeshPoints_.emplace_back(+20, -20, 100, -1, 0, 0, 0, 1, 604);
            skyBoxMeshInds_ = {0,1,2,2,3,0, 4,5,6,6,7,4, 8,9,10,10,11,8, 12,13,14,14,15,12};
            skyBoxMesh.Set(&skyBoxMeshPoints_, &skyBoxMeshInds_);

            CreatePetMesh(9, petMeshPoints_, petMeshInds_);
            petMesh.Set(&petMeshPoints_, &petMeshInds_);

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
        vec4 tex_col = texture(tex, sprite_coords);
		vec3 ambient = 0.8 * vec3(1.0);
        fragColor = tex_col * vec4(ambient + d1 * c1, 1.0);
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
            gl::ClearColor(0.503, 0.829, 0.993, 1.0f);

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

            // Enable alpha blending
            gl::Enable(gl::kBlend);
            gl::BlendFunc(gl::kSrcAlpha, gl::kOneMinusSrcAlpha);

            #pragma omp parallel for 
            for(int z = 1; z < voxels_read.Nz - 1; ++z)
                for(int y = 1; y < voxels_read.Ny - 5; ++y)
                    for (int x = 1; x < voxels_read.Nx - 1; ++x)
                    {
                        if(y < voxels_read.Ny * 0.9)
                        {
                            if (grid[x][y][z] <= 0.15)
                                voxels_read.data[voxels_read.GetIdx(x, y, z)] = eStone;
                            else
                                voxels_read.data[voxels_read.GetIdx(x, y, z)] = eAir;
                        }
                        else 
                        {
                            voxels_read.data[voxels_read.GetIdx(x, y, z)] = eGrass;
                        }
                    }
        }

    protected:
        virtual void Render() override {
            float t = glfwGetTime();
            sun_ang_ = glm::radians(t) * 20;

            #pragma omp parallel for 
            for(int i = 0; i < voxels_read.data.size(); ++i)
                voxels_write.data[i] = voxels_read.data[i];

            //TODO : MOVE
            // Update water and other dynamics things.
            if(fmod(t, 0.3) < 0.01)
            {
                #pragma omp parallel for 
                for(int z = 1; z < voxels_read.Nz - 1; ++z)
                    for(int y = 1; y < voxels_read.Ny; ++y)
                        for (int x = 1; x < voxels_read.Nx - 1; ++x)
                        {
                            int idx = voxels_read.GetIdx(x,y,z);
                            auto v = voxels_read.data[idx];
                            if(v != eWater && v != eLava)
                                continue;

                            if(voxels_read.data[idx - voxels_read.Nx] == eAir)
                            {
                                voxels_write.data[idx - voxels_read.Nx] = v;
                                need_voxels_update_ = true;
                            }

                            if(voxels_read.data[idx - voxels_read.Nx * voxels_read.Ny] == eAir && voxels_read.data[idx - voxels_read.Nx * voxels_read.Ny - voxels_read.Nx] != eAir)
                            {
                                voxels_write.data[idx - voxels_read.Nx * voxels_read.Ny] = v;
                                need_voxels_update_ = true;
                            }

                            if(voxels_read.data[idx + voxels_read.Nx * voxels_read.Ny] == eAir && voxels_read.data[idx + voxels_read.Nx * voxels_read.Ny - voxels_read.Nx] != eAir)
                            {
                                voxels_write.data[idx + voxels_read.Nx * voxels_read.Ny] = v;
                                need_voxels_update_ = true;
                            }

                            if(voxels_read.data[idx - 1] == eAir && voxels_read.data[idx - 1 - voxels_read.Nx] != eAir)
                            {
                                voxels_write.data[idx - 1] = v;
                                need_voxels_update_ = true;
                            }

                            if(voxels_read.data[idx + 1] == eAir && voxels_read.data[idx + 1 - voxels_read.Nx] != eAir)
                            {
                                voxels_write.data[idx + 1] = v;
                                need_voxels_update_ = true;
                            }
                        }
            }
        
            // Lazy voxelization.
            if (need_voxels_update_)
            {
                Voxelize(voxels_read, indices_, points_);
                for(auto& pt: points_)
                    pt.pos = pt.pos * scale_ - off_;
                need_voxels_update_ = false;
            }

            // Player position update.
            glm::vec3 pos = player_.GetPos();

            // Collision detect with voxels_read if in grid. Otherwise, apply gravity/reset to top.
            int x_vox = (pos.x + off_) / scale_;
            int y_vox = (pos.y + off_) / scale_;
            int z_vox = (pos.z + off_) / scale_;
            if(y_vox > Ngrid - 1)
            {
                player_.SetForce({0, -grav_const, 0});
                player_.Integrate(0.01);
            }
            else if(y_vox < 2)
            {
                player_.SetPos(glm::vec3(0.5, 10, 0));  
            }
            else 
            {
                if(voxels_read.data[voxels_read.GetIdx(x_vox, y_vox-2, z_vox)] == eAir)
                {
                    player_grounded_ = false;
                    player_.SetForce({0, -grav_const, 0});
                }
                else if(!player_grounded_)
                {
                    player_grounded_ = true;
                    player_.ResetVelocity();
                    player_.ResetForce();
                }

                player_.Integrate(0.01);

                glm::vec3 prev_pos = pos;
                glm::vec3 pos = player_.GetPos();
                int x_vox = (pos.x + off_) / scale_;
                int y_vox = (pos.y + off_) / scale_;
                int z_vox = (pos.z + off_) / scale_;

                // On collision, rollback position and velocity.
                if(voxels_read.data[voxels_read.GetIdx(x_vox, y_vox  , z_vox)] != eAir
                || voxels_read.data[voxels_read.GetIdx(x_vox, y_vox-1, z_vox)] != eAir)
                {
                    player_.SetPos(prev_pos); // Rollback position.
                    player_.ResetVelocity();
                    player_.ResetForce();
                }
            }
            glm::mat4 camera_mat = glm::lookAt(player_.GetPos(), player_.GetPos() + camForward, glm::vec3{0.0f, 1.0f, 0.0f});
            glm::mat4 model_mat = glm::mat4x4(1.0f);
            glm::mat4 proj_mat = glm::perspectiveFov<float>(M_PI/2.0, kScreenWidth, kScreenHeight, 0.0001f, 50);
            gl::Uniform<glm::mat4>(prog_, "mvp") = proj_mat * camera_mat * model_mat;

            auto sun_mat = glm::rotate(glm::mat4x4(1.0f), sun_ang_, glm::vec3(1.0f, 0.0f, 0.0f));
            glm::vec3 lightPos1 = player_.GetPos();
            gl::Uniform<glm::vec3>(prog_, "lightPos1") = lightPos1;
            mesh.Set(&points_, &indices_);
            mesh.Render();

            // Render pet.
            glm::vec3 pet_diff = player_.GetPos() - pet.pos;
            float nm = glm::length(pet_diff);
            if(nm > 0.05)
            {
                pet.vel = pet_diff / nm;
                pet.Integrate(0.01);
            }

            model_mat = glm::translate(glm::mat4x4(1.0f), pet.pos);
            model_mat = glm::scale(model_mat, glm::vec3(pet.scale));
            gl::Uniform<glm::mat4>(prog_, "mvp") = proj_mat * camera_mat * model_mat;
            petMesh.Render();

            // Rotating sun.
            model_mat = sun_mat;
            gl::Uniform<glm::mat4>(prog_, "mvp") = proj_mat * camera_mat * model_mat;
            skyBoxMesh.Render();

            gl::Uniform<glm::mat4>(prog_, "mvp") = glm::mat4x4(1.0f);
            screenMesh.Render();
            HandleKeys();
            HandleMouse();

            // Update voxels.
            std::swap(voxels_read.data, voxels_write.data);
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
            glfwSetCursorPos(window_, kScreenWidth / 2, kScreenHeight / 2);

            // Convert xpos, ypos from screen coordinates to [-1,1] coordinates.
            glm::vec2 wpos = {(float)xpos, (float)ypos};
            wpos = glm::vec2(2) * wpos / glm::vec2(kScreenWidth, -kScreenHeight);
            wpos += glm::vec2(-1, 1);

            // Update camera angle. Notice that we restrict the y angle 
            // because we don't want to flip the camera by crossing pi/2 or -pi/2.
            camAng.x += wpos.x * 0.2f;
            float new_cam_y = camAng.y + wpos.y * 0.2f;
            if(fabs(new_cam_y) < M_PI_2) 
                camAng.y = new_cam_y;

            camForward = {cos(camAng.x) * cos(camAng.y), 
                       sin(camAng.y), 
                       sin(camAng.x) * cos(camAng.y)};

            // Limit the number of clicks possible per second. 
            float t = glfwGetTime();
            if(t - t_click_prev_ < 0.1)
                return;

            if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                glm::ivec3 pos_vox = (player_.GetPos() + off_) / scale_;
                glm::ivec3 vox_prev;
                std::vector<glm::ivec3> voxs;
                auto hit_pos = voxels_read.CastRay(pos_vox, camForward, vox_prev, voxs);
                SetSquare(voxels_write, hit_pos, 4);
                need_voxels_update_ = true;
            }

            if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            {
                glm::ivec3 pos_vox = (player_.GetPos() + off_) / scale_;
                glm::ivec3 vox_prev;
                std::vector<glm::ivec3> voxs;
                auto hit_pos = voxels_read.CastRay(pos_vox, camForward, vox_prev, voxs);
                for(auto& v: voxs)
                    SetSquare(voxels_write, v, 1, ePumpkin);
                need_voxels_update_ = true;
            }

            t_click_prev_ = t;
        }

        void HandleKeys()
        {
            float moveSpeed = 0.5;

            // Sprinting
            if(glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                moveSpeed *= 2;

            // Handle movement with WASD
            bool gW = glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS;
            bool gA = glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS;
            bool gS = glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS;
            bool gD = glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS;
            if(gW | gA | gS | gD)
            {
                int dir = (gW | gD) ? 1 : -1;
                float ang_off_ = (gW | gS) ? 0 : M_PI_2;
                player_.m_move_vel = dir * moveSpeed * 
                        glm::vec3(cos(camAng.x + ang_off_), 0, sin(camAng.x + ang_off_));
            }
            else 
            {
                player_.m_move_vel = glm::vec3(0.0f);
            }

            // Jumping
            if(glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS)
                player_.SetVel({0, 1, 0});

            // End game
            if(glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwTerminate();
        }

};

int main() {
    Graphics().RunMainLoop();
}

