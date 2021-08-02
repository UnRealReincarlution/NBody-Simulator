#pragma once

#include <glm/gtx/norm.hpp>
#include <glm/gtc/random.hpp>
#include <vector>

#include "texture.h"
#include "sprite_renderer.h"

const int PARTICLE_PER_BODY = 0;
const float PARTICLE_LIFE = 3.0f;

const glm::vec3 start_color = glm::vec3(0.5, 0.8, 1.0);
const glm::vec3 end_color = glm::vec3(1.0, 1.0, 0.8);

struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    float life;

    Particle(glm::vec3 p, glm::vec3 v, float l)
        : position(p)
        , velocity(v)
        , life(l)
    {}
};

class Body
{
public:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 force;

    float mass;

    glm::vec2 size;
    std::vector<Particle> particles;
    float pog;
    bool isPogging;

    Texture2D sprite;

    Body(glm::vec3 p, glm::vec3 v, float m, Texture2D s)
        : position(p)
        , velocity(v)
        , mass(m)
        , sprite(s)
        , pog(glm::linearRand(0.1f, 0.5f))
        , isPogging(false)
    {
        size = glm::vec2(cbrt(m), cbrt(m));

        for (int i = 0; i < PARTICLE_PER_BODY; i++)
        {
            particles.emplace_back(p, v / 2.0f, PARTICLE_LIFE * ((float)i / PARTICLE_PER_BODY));
        }
    }

    void Draw(SpriteRenderer& renderer);
};