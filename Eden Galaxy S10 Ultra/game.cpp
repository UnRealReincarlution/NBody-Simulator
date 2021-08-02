/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/

#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/closest_point.hpp>
#include <vector>

#include <iostream>
#include <irrklang/irrKlang.h>

#include <algorithm>
#include <cmath>

#include "body.h"
#include "bhtree.h"

using namespace irrklang;

ISoundEngine* SoundEngine = createIrrKlangDevice();
ISound* music;

const float N_CONST = 10000;

const glm::vec2 PARTICLE_SIZE = glm::vec2(0.1f, 0.1f);

// Game-related State data
SpriteRenderer* SolidRenderer;
SpriteRenderer* ParticleRenderer;

glm::vec3 center_of_mass = glm::vec3(0.0f, 0.0f, 0.0f);

float cam_distance = 10e7f;
float yaw = -90.0f;
float pitch = 0.0f;

float lastX;
float lastY;
float fov = 45.0f;
bool firstMouse = true;

bool transition;
float transitionProgress;

float transitionDuration = 0.5f;

glm::vec3 transitionStart;
glm::vec3 transitionEnd;

Body* pointed_body;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1500.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);


std::vector<Body> bodies;

Game::Game(unsigned int width, unsigned int height)
    : State(GAME_ACTIVE), Keys(), Width(width), Height(height)
{
    lastX = static_cast<float>(width) / 2.0;
    lastY = static_cast<float>(width) / 2.0;
}

Game::~Game()
{
    delete SolidRenderer;
    delete ParticleRenderer;
}

void Game::Init()
{
    // load sprite shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
    ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.frag", nullptr, "particle");
    // configure sprite shaders
    glm::mat4 projection = glm::perspective(
        glm::radians(90.0f),
        static_cast<float>(this->Width) / static_cast<float>(this->Height),
        0.01f, 100000.0f
    );

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 20.0f),
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);

    ResourceManager::GetShader("particle").Use().SetInteger("image", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);

    // ResourceManager::LoadShader("shaders/HUD.vs", "shaders/HUD.frag", nullptr, "HUD");

    // set render-specific controls
    static Shader sprite = ResourceManager::GetShader("sprite");
    static Shader particleShader = ResourceManager::GetShader("particle");

    SolidRenderer = new SpriteRenderer(sprite);
    ParticleRenderer = new SpriteRenderer(particleShader);

    // load textures
    ResourceManager::LoadTexture("textures/face3D.png", true, "face");
    ResourceManager::LoadTexture("textures/facePOG.png", true, "facePOG");
    ResourceManager::LoadTexture("textures/cursor.png", true, "cursor");
    ResourceManager::LoadTexture("textures/particle.png", true, "particle");

    ResourceManager::LoadTexture("textures/blue.png", true, "blue");
    ResourceManager::LoadTexture("textures/red.png", true, "red");

    bodies.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 10e5f, ResourceManager::GetTexture("red"));

    for (int i = 0; i < 10000; i++)
    {
        glm::vec3 spawn_location = glm::sphericalRand(glm::linearRand(2.0f, 1000.0f));
        spawn_location.z = spawn_location.z * 0.125f;

        glm::vec3 spawn_velocity = glm::vec3(-spawn_location.y, spawn_location.x, 0.0f) * 0.15f;
        float spawn_mass = glm::linearRand(50.0f, 5000.0f);

        bodies.emplace_back(spawn_location, spawn_velocity, spawn_mass, ResourceManager::GetTexture("red"));
    }

    bodies.emplace_back(glm::vec3(5000.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 10e5f, ResourceManager::GetTexture("blue"));

    for (int i = 0; i < 5000; i++)
    {
        glm::vec3 spawn_location = glm::sphericalRand(glm::linearRand(2.0f, 1000.0f));
        spawn_location.z = spawn_location.z * 0.125f;

        glm::vec3 spawn_velocity = glm::vec3(spawn_location.y, -spawn_location.x, 0.0f) * 0.15f;
        float spawn_mass = glm::linearRand(50.0f, 5000.0f);

        bodies.emplace_back(spawn_location + glm::vec3(5000.0f, 0.0f, 0.0f), spawn_velocity, spawn_mass, ResourceManager::GetTexture("blue"));
    }

    // SoundEngine->play2D("audio/breakout.mp3", true);
    // music = SoundEngine->play3D("audio/breakout.mp3", vec3df(0, 0, 0), true, false, true);
    // music->setMinDistance(3.0f);
    // music->setMaxDistance(50.0f);
}

void Game::Update(float dt)
{
    dt *= 1.0f;

    std::cout << 1 / dt << "\t" << "FPS \t" << dt << "ms" << std::endl;

    pointed_body = getPointedObject();

    // UpdateBruteForce(dt);

    UpdateBarnesHut(dt);

    for (Body& body : bodies) {
        //body.pog = 1 + 0.2f * pow(sin(glfwGetTime() * (3.1415) * 2), 10);
        //body.isPogging = body.pog > 1.1f;

        const glm::vec3 displacement = body.velocity * dt;
        body.position += displacement;

        for (Particle& particle : body.particles)
        {
            particle.position += particle.velocity * dt;

            particle.life -= dt;

            if (particle.life <= 0.0f)
            {
                particle = Particle(body.position, body.velocity / 2.0f, PARTICLE_LIFE);
            }
        }
    }

    if (transition)
    {
        cameraPos = transitionStart + transitionProgress * (transitionEnd - transitionStart);
        transitionProgress += dt / transitionDuration;

        if (transitionProgress > 1.0f)
        {
            transition = false;
        }
    }

    /*SoundEngine->setListenerPosition(
        vec3df(cameraPos.x, cameraPos.y, -cameraPos.z),
        vec3df(cameraFront.x, cameraFront.y, -cameraFront.z),
        vec3df(0,0,0),
        vec3df(cameraUp.x, cameraUp.y, -cameraUp.z)
    );

    music->setPosition(
        vec3df(bodies.front().position.x, bodies.front().position.y, bodies.front().position.z)
    );*/

    glm::mat4 view = glm::lookAt(
        cameraPos,
        cameraPos + cameraFront,
        cameraUp
    );

    ResourceManager::GetShader("sprite").Use().SetMatrix4("view", view);
    ResourceManager::GetShader("particle").Use().SetMatrix4("view", view);
    ResourceManager::GetShader("particle").SetFloat("hue", glfwGetTime());
}

void Game::UpdateBruteForce(float dt)
{
    float g = 6.674e1; //6.674e-11;
    float e = 1e-2;

    for (int i = 0; i < bodies.size(); i++) {
        for (int j = i + 1; j < bodies.size(); j++) {
            Body& b1 = bodies[i];
            Body& b2 = bodies[j];

            float force = (g * b1.mass * b2.mass) / (glm::distance2(b1.position, b2.position) + e);
            glm::vec3 direction = glm::normalize(b1.position - b2.position);

            b1.velocity -= direction * (force / (b1.mass)) * dt;
            b2.velocity += direction * (force / (b2.mass)) * dt;
        }
    }
}

void Game::UpdateBarnesHut(float dt)
{
    float min_coord = 0.0f;
    float max_coord = 0.0f;

    for (Body& body : bodies)
    {
        min_coord = std::min({ min_coord, body.position.x, body.position.y, body.position.z });
        max_coord = std::max({ max_coord, body.position.x, body.position.y, body.position.z });
    }

    BHTree root(Oct{ glm::vec3(0.0f, 0.0f, 0.0f), std::max(abs(min_coord), max_coord) + 128.0f });

    for (Body& body : bodies)
    {
        root.Insert(&body);
    }

    for (Body& body : bodies)
    {
        root.UpdateForce(&body);
    }

    center_of_mass = root.com;
    // cameraPos = center_of_mass - glm::vec3(0.0f, 0.0f, 1000.0f);
}

void Game::ProcessInput(float dt)
{
    if (firstMouse)
    {
        lastX = MouseX;
        lastY = MouseY;
        firstMouse = false;
    }

    float xoffset = MouseX - lastX;
    float yoffset = lastY - MouseY;

    lastX = MouseX;
    lastY = MouseY;

    float sens = 0.1f;
    xoffset *= sens;
    yoffset *= sens;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;

    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);


    const float cameraSpeed = 1000.0f;
    glm::vec3 movement = glm::vec3(0, 0, 0);

    if (Keys[GLFW_KEY_W])
        movement += cameraFront * cameraSpeed * dt;
    if (Keys[GLFW_KEY_S])
        movement -= cameraFront * cameraSpeed * dt;
    if (Keys[GLFW_KEY_A])
        movement -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * dt;
    if (Keys[GLFW_KEY_D])
        movement += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed * dt;

    cameraPos += movement;
    std::cout << "Movement: " << "x: " << movement.x << " y: " << movement.y << " z: " << movement.z << std::endl;
}

void Game::Render()
{
    static Texture2D face = ResourceManager::GetTexture("face");
    static Texture2D face_selected = ResourceManager::GetTexture("facePOG");
    static Texture2D particle_ = ResourceManager::GetTexture("particle");

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (Body& body : bodies) {
        glm::vec2 size = body.size * body.pog;

        if (&body == pointed_body) SolidRenderer->DrawSprite(face_selected, body.position, size);
        else SolidRenderer->DrawSprite(body.sprite, body.position, size);

        for (const Particle& particle : body.particles) {
            ParticleRenderer->DrawSprite(particle_, particle.position, PARTICLE_SIZE);
        }
    }

    static Texture2D hud = ResourceManager::GetTexture("cursor");
    SolidRenderer->DrawSprite(hud, cameraPos + cameraFront, glm::vec2(0.01f, 0.01f));
}

Body* Game::getPointedObject()
{
    glm::vec3 start = cameraPos;
    glm::vec3 end = cameraPos + (cameraFront * 10000.0f);

    Body* pointed_body = nullptr;
    float closest_d2_to_camera = 10000.0f;

    for (Body& body : bodies)
    {
        float d2_to_camera = glm::distance2(start, body.position);
        if (d2_to_camera > closest_d2_to_camera) continue;

        glm::vec3 closest_point = glm::closestPointOnLine(body.position, start, end);

        float d2 = glm::distance2(closest_point, body.position);
        float r2 = 0.0f; // pow(body.size[0] / 2.0f, 2);

        if (r2 > d2)
        {
            pointed_body = &body;
            closest_d2_to_camera = d2;
        }
    }

    return pointed_body;
}

void Game::onClick(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (pointed_body)
        {
            beginTransition(cameraPos, pointed_body->position);
        }
    }
}

void Game::beginTransition(glm::vec3 start, glm::vec3 end)
{
    transitionStart = start;
    transitionEnd = end;

    transition = true;
    transitionProgress = 0.0f;
}

// Barnes-Hut Algorithm

