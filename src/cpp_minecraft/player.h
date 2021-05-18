#pragma once

#include "custom_shape.h"

class Player 
{
private:
    glm::vec3 m_pos;

public:
    Player (glm::vec3 pos = glm::vec3(0.0f)) : m_pos{pos} {}

    // TODO: MAKE REALISTIC
    void ApplyGravity (float dt_sqr, float grav_const) {m_pos.y -= dt_sqr * grav_const;}
    glm::vec3 const& GetPos () const {return m_pos;}
    void SetPos (glm::vec3 const& pos) {m_pos = pos;}
};
