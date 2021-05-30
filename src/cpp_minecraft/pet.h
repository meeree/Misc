#pragma once 

#include "custom_shape.h"

struct Pet 
{
    glm::vec3 force;
    glm::vec3 vel;
    glm::vec3 pos;
    float scale;

    Pet (glm::vec3 pos_ = glm::vec3(0.0f), float scale_ = 1.0f) : force{0,0,0}, vel{0,0,0}, pos{pos_}, scale{scale_} {} 

    void Integrate (float dt)
    {
        vel += dt * force;
        pos += dt * vel;
    }
};
