#pragma once 
#include "voxels.h"

struct Box 
{
    glm::vec3 min;
    glm::vec3 max;
};

struct Endpoint 
{
    float val;
    bool is_start;
};

class Collisions 
{
private:
    std::vector<Box> m_boxes;
    std::vector<Endpoint> m_axes[3];
    std::vector<int> m_vox_ids_axes[3];
    
public:
    void HandleCollisionsWithVoxels (Voxels& voxels, float scale, float off) 
    {
        // Find voxel indices from m_axes.
        for(int ax = 0; ax < 3; ++ax)
        {
            auto const& axis{m_axes[ax]};
            auto const& vox_idx_axis{m_vox_ids_axes[ax]};

            // Note that we add 0.5 to round to nearest voxel.
            for(int i = 0; i < axis.size(); ++i)
                vox_idx_axis[i] = (axis[i] + off) / scale + 0.5; 
        }
    }

    // TODO: IMPLEMENT
    void HandleCollisions () 
    {
        for(auto& axis: m_axes)
            std::sort(axis.begin(), axis.end());

    }
};
