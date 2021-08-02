#include "body.h"

void Body::Draw(SpriteRenderer& renderer)
{
	float factor = this->force.length() / 1000.0f;

	glm::vec3 color = start_color * (1 - factor) + end_color * factor;

	renderer.DrawSprite(this->sprite, this->position, this->size, 0.0f, color);
}