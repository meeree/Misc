#pragma once 

#include "custom_shape.h"

enum Block : int
{
    eWood=0, ePumpkin, eLava, eAir, eCount
};

struct Voxels 
{
    int Nx, Ny, Nz;
    std::vector<Block> data;

    Voxels (int Nx_, int Ny_, int Nz_)
        : Nx{Nx_}, Ny{Ny_}, Nz{Nz_}, data(Nx_*Ny_*Nz_, eAir)
    {}

    int GetIdx (int x, int y, int z) const
    {
        return x + Nx * (y + Ny * z);
    }

    int GetIdx (glm::ivec3 const& p) const
    {
        return p.x + Nx * (p.y + Ny * p.z);
    }

    glm::ivec3 CastRay (glm::ivec3 pos_vox, glm::vec3 const& dir, glm::ivec3& vox_prev) 
    {
        // See http://www.cse.yorku.ca/~amana/research/grid.pdf.
        glm::ivec3 pos_vox_init = pos_vox;
        vox_prev = pos_vox;
        glm::ivec3 step = glm::ivec3(dir.x > 0, dir.y > 0, dir.z > 0) * 2 - 1;
        
        while(data[GetIdx(pos_vox)] == eAir)
        {
            vox_prev = pos_vox;
            // Distance in is just distance along each dimension divided by component in each dimension.
            // Note that we should really normalize dir and scale this while thing by scale. 
            // But it doesn't matter! Distance comparisons still work all the same!
            glm::vec3 distNext = glm::abs(glm::vec3(pos_vox + step - pos_vox_init) / dir); 

            // Move in direction of minimum distance. 
            if(distNext.x < distNext.y)
            {
                if(distNext.x < distNext.z)
                {
                    pos_vox.x += step.x;
                    if((dir.x > 0 && pos_vox.x > Nx - 1) || (dir.x <= 0 && pos_vox.x < 0))
                        break;
                }
                else
                {
                    pos_vox.z += step.z;
                    if((dir.z > 0 && pos_vox.z > Nx - 1) || (dir.z <= 0 && pos_vox.z < 0))
                        break;
                }
            }
            else
            {
                if(distNext.y < distNext.z)
                {
                    pos_vox.y += step.y;
                    if((dir.y > 0 && pos_vox.y > Nx - 1) || (dir.y <= 0 && pos_vox.y < 0))
                        break;
                }
                else 
                {
                    pos_vox.z += step.z;
                    if((dir.z > 0 && pos_vox.z > Nx - 1) || (dir.z <= 0 && pos_vox.z < 0))
                        break;
                }
            }
        }
        return pos_vox;
    }
};

void SetSquare (Voxels& grid, glm::ivec3 const& coord, int len=1, Block v = eAir)
{
    glm::ivec3 mx = coord + len;
    mx = glm::min(mx, glm::ivec3(grid.Nx, grid.Ny, grid.Nz));
    for(int z = coord.z; z < mx.z; ++z)
        for(int y = coord.y; y < mx.y; ++y)
            for(int x = coord.x; x < mx.x; ++x)
                grid.data[grid.GetIdx(x,y,z)] = v;
}

// Lookup sprite index given block type and face (one of 0 through 5).
// Indexed by blockType * 6 + faceIdx.
// faceIdx indexing is: LEFT, RIGHT, BOTTOM, TOP, BACK, FRONT
static int g_blockFaceSpriteLookup [] = 
{
      16 ,   16,   16,   16,   16,   16,    
      17 ,   17,  113,  113,   17,  464,
      0  ,  0  ,  0  ,  0  ,  0  ,  0  ,   
      602,  602,  602,  602,  602,  602,   
};

void Voxelize(Voxels const& grid, 
        std::vector<unsigned>& indices, std::vector<MeshPoint>& points)
{
    // Note that resize to zero does not change capacity. Therefore, 
    // although we call emplace_back a bunch here, if we run this function 
    // a lot, the emplace_back will be constant time.
    indices.resize(0);
    points.resize(0);

    // We only voxelize interior voxels. Boundary voxels are slightly more complicated.
    for(int z = 1; z < grid.Nz - 1; ++z)
    {
        for(int y = 1; y < grid.Ny - 1; ++y)
        {
            for(int x = 1; x < grid.Nx - 1; ++x)
            {
                int idx = x + grid.Nx * (y + grid.Ny * z);
                Block v{grid.data[idx]};
                if(v == eAir)
                    continue;

                // Check X. 
                if(grid.data[idx-1] == eAir) // Left 
                {
                    int spid = g_blockFaceSpriteLookup[v*6+0];
                    points.emplace_back(x, y  , z  , -1, 0, 0, 0, 0, spid);
                    points.emplace_back(x, y  , z+1, -1, 0, 0, 1, 0, spid);
                    points.emplace_back(x, y+1, z+1, -1, 0, 0, 1, 1, spid);
                    points.emplace_back(x, y+1, z  , -1, 0, 0, 0, 1, spid);
                }
                if(grid.data[idx+1] == eAir) // Right
                {
                    int spid = g_blockFaceSpriteLookup[v*6+1];
                    points.emplace_back(x+1, y  , z  , 1, 0, 0, 0, 0, spid);
                    points.emplace_back(x+1, y  , z+1, 1, 0, 0, 1, 0, spid);
                    points.emplace_back(x+1, y+1, z+1, 1, 0, 0, 1, 1, spid);
                    points.emplace_back(x+1, y+1, z  , 1, 0, 0, 0, 1, spid);
                }

                // Check Y. 
                if(grid.data[idx-grid.Nx] == eAir) // Bottom 
                {
                    int spid = g_blockFaceSpriteLookup[v*6+2];
                    points.emplace_back(x  , y, z  , 0, -1, 0, 0, 0, spid); 
                    points.emplace_back(x  , y, z+1, 0, -1, 0, 0, 1, spid); 
                    points.emplace_back(x+1, y, z+1, 0, -1, 0, 1, 1, spid); 
                    points.emplace_back(x+1, y, z  , 0, -1, 0, 1, 0, spid); 
                }
                if(grid.data[idx+grid.Nx] == eAir) // Top
                {
                    int spid = g_blockFaceSpriteLookup[v*6+3];
                    points.emplace_back(x  , y+1, z  , 0, 1, 0, 0, 0, spid);
                    points.emplace_back(x  , y+1, z+1, 0, 1, 0, 0, 1, spid);
                    points.emplace_back(x+1, y+1, z+1, 0, 1, 0, 1, 1, spid);
                    points.emplace_back(x+1, y+1, z  , 0, 1, 0, 1, 0, spid);
                }

                // Check Z. 
                if(grid.data[idx-grid.Nx*grid.Ny] == eAir) // Back
                {
                    int spid = g_blockFaceSpriteLookup[v*6+4];
                    points.emplace_back(x  , y  , z, 0, 0, -1, 0, 0, spid); 
                    points.emplace_back(x  , y+1, z, 0, 0, -1, 0, 1, spid); 
                    points.emplace_back(x+1, y+1, z, 0, 0, -1, 1, 1, spid); 
                    points.emplace_back(x+1, y  , z, 0, 0, -1, 1, 0, spid); 
                }
                if(grid.data[idx+grid.Nx*grid.Ny] == eAir) // Front
                {
                    int spid = g_blockFaceSpriteLookup[v*6+5];
                    points.emplace_back(x  , y  , z+1, 0, 0, 1, 0, 0, spid); 
                    points.emplace_back(x  , y+1, z+1, 0, 0, 1, 0, 1, spid); 
                    points.emplace_back(x+1, y+1, z+1, 0, 0, 1, 1, 1, spid); 
                    points.emplace_back(x+1, y  , z+1, 0, 0, 1, 1, 0, spid); 
                }
            }
        }
    }

    // Indices are the same for each square.
    int inds_single[6]{0, 1, 2, 2, 3, 0};
    for(size_t i = 0; i < points.size() / 4; ++i)
    {
        indices.insert(indices.end(), inds_single, inds_single + 6);
        for(int& i: inds_single)
            i += 4;
    }
}
