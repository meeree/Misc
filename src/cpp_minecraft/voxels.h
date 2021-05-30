#pragma once 

#include "custom_shape.h"

enum Block : int
{
    eWood=0, ePumpkin, eLava, eLavaPartial, eWater, eWaterPartial, eDirt, eGrass, eStone, eAir, eCount
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

    glm::ivec3 CastRay (glm::ivec3 pos_vox, glm::vec3 const& dir, glm::ivec3& vox_prev, std::vector<glm::ivec3>& voxs) 
    {
        // See http://www.cse.yorku.ca/~amana/research/grid.pdf.
        glm::ivec3 pos_vox_init = pos_vox;
        vox_prev = pos_vox;
        glm::ivec3 step = glm::ivec3(dir.x > 0, dir.y > 0, dir.z > 0) * 2 - 1;
        
        while(data[GetIdx(pos_vox)] == eAir)
        {
            vox_prev = pos_vox;
            voxs.push_back(pos_vox);
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
      0  ,  0  ,  0  ,  0  ,  0  ,  0  ,   
      2  ,  2  ,  2  ,  2  ,  2  ,  2  ,   
      2  ,  2  ,  2  ,  2  ,  2  ,  2  ,   
      162,  162,  162,  162,  162,  162,   
      332,  332,  162,  332,  332,  332,   
      308,  308,  308,  308,  308,  308,   
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

//static uint16_t edgeTable[256]{
//    0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
//    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
//    0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
//    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
//    0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
//    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
//    0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
//    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
//    0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
//    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
//    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
//    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
//    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
//    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
//    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
//    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
//    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
//    0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
//    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
//    0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
//    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
//    0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
//    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
//    0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
//    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
//    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
//    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
//    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
//    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
//    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
//    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
//    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };
//
//static int8_t triTable[256][16]{
//    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
//    {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
//    {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
//    {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
//    {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
//    {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
//    {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
//    {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
//    {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
//    {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
//    {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
//    {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
//    {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
//    {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
//    {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
//    {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
//    {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
//    {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
//    {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
//    {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
//    {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
//    {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
//    {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
//    {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
//    {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
//    {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
//    {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
//    {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
//    {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
//    {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
//    {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
//    {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
//    {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
//    {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
//    {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
//    {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
//    {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
//    {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
//    {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
//    {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
//    {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
//    {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
//    {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
//    {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
//    {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
//    {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
//    {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
//    {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
//    {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
//    {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
//    {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
//    {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
//    {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
//    {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
//    {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
//    {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
//    {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
//    {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
//    {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
//    {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
//    {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
//    {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
//    {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
//    {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
//    {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
//    {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
//    {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
//    {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
//    {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
//    {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
//    {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
//    {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
//    {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
//    {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
//    {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
//    {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
//    {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
//    {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
//    {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
//    {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
//    {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
//    {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
//    {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
//    {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
//    {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
//    {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
//    {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
//    {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
//    {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
//    {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
//    {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
//    {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
//    {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
//    {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
//    {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
//    {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
//    {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
//    {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
//    {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
//    {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
//    {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
//    {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
//    {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
//    {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
//    {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
//    {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
//    {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
//    {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
//    {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
//    {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
//    {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
//    {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
//    {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
//    {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
//    {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
//    {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
//    {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
//    {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
//    {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
//    {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
//    {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
//    {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
//    {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
//    {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
//    {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
//    {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
//    {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
//    {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
//    {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
//    {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
//    {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
//    {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
//    {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
//    {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
//    {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
//    {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
//    {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
//    {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
//    {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
//    {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
//    {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
//    {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
//    {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
//    {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
//    {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
//    {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
//    {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
//    {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
//    {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
//    {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
//    {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
//    {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
//    {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
//    {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
//    {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
//    {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
//    {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
//    {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
//    {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
//    {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
//    {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
//    {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
//    {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
//    {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
//    {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
//    {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
//    {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
//    {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
//    {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
//    {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
//    {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
//    {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
//    {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
//    {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
//    {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
//    {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
//    {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
//    {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
//    {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
//    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
//
//vec3 vertexInterp(iso_uint_t const& isolevel, glm::vec3 const& p1, glm::vec3 const& p2, iso_uint_t const& valp1, iso_uint_t const& valp2)
//{
//	if (isolevel == valp1)
//		return(p1);
//	if (isolevel == valp2)
//		return(p2);
//	if (valp1 == valp2)
//		return(p1);
//	double mu = (isolevel - valp1) / (valp2 - valp1);
//	return {p1.x + mu * (p2.x - p1.x),
//            p1.y + mu * (p2.y - p1.y), 
//            p1.z + mu * (p2.z - p1.z)};
//}
//
//
//int Polygonise(Voxels const& voxels, std::vector<MeshPoint>& points, NewVertexCount, Vec3f *Vertices)
//{
//    glm::vec3 vert_list[12];
//    glm::vec3 new_vert_list[12];
//	char local_remap[12];
//    for (unsigned z = 0; z < grid.mDim[2]-1; ++z)
//    {
//        for (unsigned y = 0; y < grid.mDim[1]-1; ++y)
//        {
//            for (unsigned x = 0; x < grid.mDim[0]-1; ++x)
//            {
//                iso_uint_t values[8]{
//                    voxels[voxels.GetIdx(x,y,z+1)],
//                    voxels[voxels.GetIdx(x+1,y,z+1)],
//                    voxels[voxels.GetIdx(x+1,y,z)],
//                    voxels[voxels.GetIdx(x,y,z)],
//                    voxels[voxels.GetIdx(x,y+1,z+1)],
//                    voxels[voxels.GetIdx(x+1,y+1,z+1)],
//                    voxels[voxels.GetIdx(x+1,y+1,z)],
//                    voxels[voxels.GetIdx(x,y+1,z)]
//                };
//                glm::vec3 positions[8]{
//                    x, y, z+1,
//                    x+1, y, z+1,
//                    x+1, y, z,
//                    x, y, z,
//                    x, y+1, z+1,
//                    x+1, y+1, z+1,
//                    x+1, y+1, z,
//                    x, y+1, z
//                };
//
//                uint8_t cubeindex = 0;
//                if (values[0] < isolevel)
//                     cubeindex |= 1;
//                if (values[1] < isolevel)
//                     cubeindex |= 2;
//                if (values[2] < isolevel)
//                     cubeindex |= 4;
//                if (values[3] < isolevel)
//                     cubeindex |= 8;
//                if (values[4] < isolevel)
//                     cubeindex |= 16;
//                if (values[5] < isolevel)
//                     cubeindex |= 32;
//                if (values[6] < isolevel)
//                     cubeindex |= 64;
//                if (values[7] < isolevel)
//                     cubeindex |= 128;
//
//                if (edgeTable[cubeindex] == 0)
//                    continue;
//
//                if (edgeTable[cubeindex] & 1)
//                    vert_list[0] = vertexInterp(isolevel,positions[0],positions[1],values[0],values[1]);
//                if (edgeTable[cubeindex] & 2)
//                    vert_list[1] = vertexInterp(isolevel,positions[1],positions[2],values[1],values[2]);
//                if (edgeTable[cubeindex] & 4)
//                    vert_list[2] = vertexInterp(isolevel,positions[2],positions[3],values[2],values[3]);
//                if (edgeTable[cubeindex] & 8)
//                    vert_list[3] = vertexInterp(isolevel,positions[3],positions[0],values[3],values[0]);
//                if (edgeTable[cubeindex] & 16)
//                    vert_list[4] = vertexInterp(isolevel,positions[4],positions[5],values[4],values[5]);
//                if (edgeTable[cubeindex] & 32)
//                    vert_list[5] = vertexInterp(isolevel,positions[5],positions[6],values[5],values[6]);
//                if (edgeTable[cubeindex] & 64)
//                    vert_list[6] = vertexInterp(isolevel,positions[6],positions[7],values[6],values[7]);
//                if (edgeTable[cubeindex] & 128)
//                    vert_list[7] = vertexInterp(isolevel,positions[7],positions[4],values[7],values[4]);
//                if (edgeTable[cubeindex] & 256)
//                    vert_list[8] = vertexInterp(isolevel,positions[0],positions[4],values[0],values[4]);
//                if (edgeTable[cubeindex] & 512)
//                    vert_list[9] = vertexInterp(isolevel,positions[1],positions[5],values[1],values[5]);
//                if (edgeTable[cubeindex] & 1024)
//                    vert_list[10] = vertexInterp(isolevel,positions[2],positions[6],values[2],values[6]);
//                if (edgeTable[cubeindex] & 2048)
//                    vert_list[11] = vertexInterp(isolevel,positions[3],positions[7],values[3],values[7]);
//
//                int new_vert_count = 0;
//                for (int i=0; i<12; i++)
//                    local_remap[i] = -1;
//
//                for (int i=0;triTable[cubeindex][i]!=-1;i++)
//                {
//                    if(local_remap[triTable[cubeindex][i]] == -1)
//                    {
//                        new_vert_list[new_vert_count] = vert_list[triTable[cubeindex][i]];
//                        local_remap[triTable[cubeindex][i]] = new_vert_count;
//                        ++new_vert_count;
//                    }
//                }
//
//                for (int i=0; i < new_vert_count/3; i++) 
//                {
//                    poin
//                }
//                    Vertices[i] = NewVertexList[i];
//
//                TriangleCount = 0;
//                for (UINT i=0;triTable[CubeIndex][i]!=-1;i+=3) {
//                    Triangles[TriangleCount].I[0] = LocalRemap[triTable[CubeIndex][i+0]];
//                    Triangles[TriangleCount].I[1] = LocalRemap[triTable[CubeIndex][i+1]];
//                    Triangles[TriangleCount].I[2] = LocalRemap[triTable[CubeIndex][i+2]];
//                    TriangleCount++;
//                }
//	int TriangleCount;
//	int CubeIndex;
//
//	Vec3f VertexList[12];
//	Vec3f NewVertexList[12];
//	char LocalRemap[12];
//
//	
//	//Determine the index into the edge table which
//	//tells us which vertices are inside of the surface
//	CubeIndex = 0;
//	if (Grid.val[0] < 0.0f) CubeIndex |= 1;
//	if (Grid.val[1] < 0.0f) CubeIndex |= 2;
//	if (Grid.val[2] < 0.0f) CubeIndex |= 4;
//	if (Grid.val[3] < 0.0f) CubeIndex |= 8;
//	if (Grid.val[4] < 0.0f) CubeIndex |= 16;
//	if (Grid.val[5] < 0.0f) CubeIndex |= 32;
//	if (Grid.val[6] < 0.0f) CubeIndex |= 64;
//	if (Grid.val[7] < 0.0f) CubeIndex |= 128;
//
//	//Cube is entirely in/out of the surface
//	if (edgeTable[CubeIndex] == 0)
//		return(0);
//
//	//Find the vertices where the surface intersects the cube
//	if (edgeTable[CubeIndex] & 1)
//		VertexList[0] =
//			VertexInterp(Grid.p[0],Grid.p[1],Grid.val[0],Grid.val[1]);
//	if (edgeTable[CubeIndex] & 2)
//		VertexList[1] =
//			VertexInterp(Grid.p[1],Grid.p[2],Grid.val[1],Grid.val[2]);
//	if (edgeTable[CubeIndex] & 4)
//		VertexList[2] =
//			VertexInterp(Grid.p[2],Grid.p[3],Grid.val[2],Grid.val[3]);
//	if (edgeTable[CubeIndex] & 8)
//		VertexList[3] =
//			VertexInterp(Grid.p[3],Grid.p[0],Grid.val[3],Grid.val[0]);
//	if (edgeTable[CubeIndex] & 16)
//		VertexList[4] =
//			VertexInterp(Grid.p[4],Grid.p[5],Grid.val[4],Grid.val[5]);
//	if (edgeTable[CubeIndex] & 32)
//		VertexList[5] =
//			VertexInterp(Grid.p[5],Grid.p[6],Grid.val[5],Grid.val[6]);
//	if (edgeTable[CubeIndex] & 64)
//		VertexList[6] =
//			VertexInterp(Grid.p[6],Grid.p[7],Grid.val[6],Grid.val[7]);
//	if (edgeTable[CubeIndex] & 128)
//		VertexList[7] =
//			VertexInterp(Grid.p[7],Grid.p[4],Grid.val[7],Grid.val[4]);
//	if (edgeTable[CubeIndex] & 256)
//		VertexList[8] =
//			VertexInterp(Grid.p[0],Grid.p[4],Grid.val[0],Grid.val[4]);
//	if (edgeTable[CubeIndex] & 512)
//		VertexList[9] =
//			VertexInterp(Grid.p[1],Grid.p[5],Grid.val[1],Grid.val[5]);
//	if (edgeTable[CubeIndex] & 1024)
//		VertexList[10] =
//			VertexInterp(Grid.p[2],Grid.p[6],Grid.val[2],Grid.val[6]);
//	if (edgeTable[CubeIndex] & 2048)
//		VertexList[11] =
//			VertexInterp(Grid.p[3],Grid.p[7],Grid.val[3],Grid.val[7]);
//
//	return(TriangleCount);
//}
//
//// Move collision table used to determine the edge to check for collision
//// in the map. For instance, if we lookup edge 0 we get (010(binary),4), 
//// corresponding to "edge 4 on cube situated at x-0,y-1,z-0." 
//static std::tuple<uint8_t, uint8_t> moveCollisionTable[12]{
//    std::make_tuple(2,0),
//    std::make_tuple(2,5),
//    std::make_tuple(2,6),
//    std::make_tuple(2,7),
//
//    std::make_tuple(-1,-1),
//    std::make_tuple(-1,-1),
//    std::make_tuple(1,4),
//    std::make_tuple(4,5),
//
//    std::make_tuple(4,9),
//    std::make_tuple(-1,-1),
//    std::make_tuple(1,9),
//    std::make_tuple(1,8)
//};
//
//void polygonise(Voxels const& voxels, std::vector<MeshPoint>& points, std::vector<unsigned>& indices)
//{
//    std::map<std::tuple<unsigned, unsigned, unsigned, uint8_t>, std::vector<unsigned>> edgePosIndexMap;
//    for (unsigned k = 0; k < grid.mDim[2]-1; ++k)
//    {
//        for (unsigned j = 0; j < grid.mDim[1]-1; ++j)
//        {
//            for (unsigned i = 0; i < grid.mDim[0]-1; ++i)
//            {
//                iso_uint_t values[8]{
//                    grid.mGrid[i][j][k+1],
//                    grid.mGrid[i+1][j][k+1],
//                    grid.mGrid[i+1][j][k],
//                    grid.mGrid[i][j][k],
//                    grid.mGrid[i][j+1][k+1],
//                    grid.mGrid[i+1][j+1][k+1],
//                    grid.mGrid[i+1][j+1][k],
//                    grid.mGrid[i][j+1][k]
//                };
//                glm::vec3 positions[8]{
//                    i, j, k+1,
//                    i+1, j, k+1,
//                    i+1, j, k,
//                    i, j, k,
//                    i, j+1, k+1,
//                    i+1, j+1, k+1,
//                    i+1, j+1, k,
//                    i, j+1, k
//                };
//                glm::vec3 vert_list[12];
//
//                uint8_t cubeindex = 0;
//                if (values[0] < isolevel)
//                     cubeindex |= 1;
//                if (values[1] < isolevel)
//                     cubeindex |= 2;
//                if (values[2] < isolevel)
//                     cubeindex |= 4;
//                if (values[3] < isolevel)
//                     cubeindex |= 8;
//                if (values[4] < isolevel)
//                     cubeindex |= 16;
//                if (values[5] < isolevel)
//                     cubeindex |= 32;
//                if (values[6] < isolevel)
//                     cubeindex |= 64;
//                if (values[7] < isolevel)
//                     cubeindex |= 128;
//
//                if (edgeTable[cubeindex] == 0)
//                    continue;
//
//                if (edgeTable[cubeindex] & 1)
//                    vertList[0] = vertexInterp(isolevel,positions[0],positions[1],values[0],values[1]);
//                if (edgeTable[cubeindex] & 2)
//                    vertList[1] = vertexInterp(isolevel,positions[1],positions[2],values[1],values[2]);
//                if (edgeTable[cubeindex] & 4)
//                    vertList[2] = vertexInterp(isolevel,positions[2],positions[3],values[2],values[3]);
//                if (edgeTable[cubeindex] & 8)
//                    vertList[3] = vertexInterp(isolevel,positions[3],positions[0],values[3],values[0]);
//                if (edgeTable[cubeindex] & 16)
//                    vertList[4] = vertexInterp(isolevel,positions[4],positions[5],values[4],values[5]);
//                if (edgeTable[cubeindex] & 32)
//                    vertList[5] = vertexInterp(isolevel,positions[5],positions[6],values[5],values[6]);
//                if (edgeTable[cubeindex] & 64)
//                    vertList[6] = vertexInterp(isolevel,positions[6],positions[7],values[6],values[7]);
//                if (edgeTable[cubeindex] & 128)
//                    vertList[7] = vertexInterp(isolevel,positions[7],positions[4],values[7],values[4]);
//                if (edgeTable[cubeindex] & 256)
//                    vertList[8] = vertexInterp(isolevel,positions[0],positions[4],values[0],values[4]);
//                if (edgeTable[cubeindex] & 512)
//                    vertList[9] = vertexInterp(isolevel,positions[1],positions[5],values[1],values[5]);
//                if (edgeTable[cubeindex] & 1024)
//                    vertList[10] = vertexInterp(isolevel,positions[2],positions[6],values[2],values[6]);
//                if (edgeTable[cubeindex] & 2048)
//                    vertList[11] = vertexInterp(isolevel,positions[3],positions[7],values[3],values[7]);
//
//                for (unsigned ind = 0; triTable[cubeindex][ind] != -1; ind += 3) 
//                {
//                    for (uint8_t coord = 0; coord < 3; ++coord)
//                    {
//                        // Note: we cast here because we can't get a -1.
//                        uint8_t edgeIndex{(uint8_t)triTable[cubeindex][ind+coord]};
//                        std::tuple<uint8_t,uint8_t> mvDir{moveCollisionTable[edgeIndex]};
//
//                        // Calculate offset from 3 bit uint:
//                        // 100 corresponds to (-1,0,0),
//                        // 010 corresponds to (0,-1,0),
//                        // 001 corresponds to (0,0,-1).
//                        unsigned offsetMove[3]{
//                            i-(std::get<0>(mvDir)==4),
//                            j-(std::get<0>(mvDir)==2),
//                            k-(std::get<0>(mvDir)==1)};
//                        auto edgePos = std::make_tuple(offsetMove[0],offsetMove[1],offsetMove[2],edgeIndex);
//
//                        auto edgeFind = edgePosIndexMap.find(edgePos);
//                        std::vector<unsigned>& lookup{edgePosIndexMap[edgePos]};
//                        if (std::get<0>(mvDir) == (uint8_t)-1)
//                        {
//                            indices.push_back(vertices.size());
//                            lookup.push_back(vertices.size());
//                            vertices.push_back({vertList[edgeIndex],{0,0,0}});
//                        }
//                        else if (edgeFind != edgePosIndexMap.end())
//                        {
//                            bool collision{false};
//                            for (auto const& testIndex: lookup)
//                            {
//                                if (testIndex == (unsigned)-1)
//                                    continue;
//                                Vertex& testVert{vertices[testIndex]};
//                                glm::vec3 diff{glm::abs(testVert.mPosition-vertList[edgeIndex])};
//                                glm::vec3 largest{glm::max(glm::abs(testVert.mPosition), glm::abs(vertList[edgeIndex]))};
////                                if (length(testVert.mPosition-vertList[edgeIndex]) <= FLT_EPSILON)
//                                // This threshold can be tweaked to connect vertices that are within a range,
//                                // making a more connected looking shape, but with the issue that the mesh
//                                // will be different dependent on which vertices are added to the map first. 
//                                if (diff.x <= FLT_EPSILON*largest.x && diff.y <= FLT_EPSILON*largest.y && diff.z <= FLT_EPSILON*largest.z)
//                                {
//                                    indices.push_back(testIndex);
//                                    collision = true;
//                                    break;
//                                } 
//                            }
//                            if (!collision)
//                            {
//                                indices.push_back(vertices.size());
//                                // Set up edges for deletion
//                                if (edgeIndex == 2 || edgeIndex == 3 || edgeIndex == 11)
//                                    lookup.push_back(-1);
//                                else 
//                                    lookup.push_back(vertices.size());
//                                vertices.push_back({vertList[edgeIndex],{0,0,0}});
//                            }
//                        }
//                        else
//                        {
//                            indices.push_back(vertices.size());
//                            if (edgeIndex == 2 || edgeIndex == 3 || edgeIndex == 11)
//                                lookup.push_back(-1);
//                            else 
//                                lookup.push_back(vertices.size());
//                            vertices.push_back({vertList[edgeIndex],{0,0,0}});
//                        }
//                    }
////                    // Set up top edges for deletion
////                    auto topEdgeFind = edgePosIndexMap.find(std::make_tuple(i,j,k,6));
////                    if (topEdgeFind != edgePosIndexMap.end())
////                        (*topEdgeFind).second.back() = -1;
////                    topEdgeFind = edgePosIndexMap.find(std::make_tuple(i,j,k,7));
////                    if (topEdgeFind != edgePosIndexMap.end())
////                        (*topEdgeFind).second.back() = -1;
//                }
////                auto topRightEdgeFind = edgePosIndexMap.find(std::make_tuple(i,j,k,5));
////                if (topRightEdgeFind != edgePosIndexMap.end())
////                    (*topRightEdgeFind).second.back() = -1;
////
//                // Deleting useless edges from map
////                for (auto edgeIndexPair = edgePosIndexMap.begin(); edgeIndexPair != edgePosIndexMap.end(); ++edgeIndexPair)
////                {
////                    if ((*edgeIndexPair).second.back() == (unsigned)-1)
////                    {
////                        edgePosIndexMap.erase(edgeIndexPair++);
////                    }
////                }
////                std::cout<<edgePosIndexMap.size()<<std::endl;
//            }
//        }
//    }
//}
