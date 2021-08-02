#include "bhtree.h"
#include "body.h"

const int Oct::GetSubtree(Body* b)
{
	int x = b->position.x > center.x;
	int y = b->position.y > center.y;
	int z = b->position.z > center.z;

	return (
		(x << 0),
		(y << 1),
		(z << 2)
		);
}

BHTree::BHTree(Oct o)
	: oct(o)
	, is_external(false)
	, contains_body(false)
	, mass(0.0f)
	, com(glm::vec3(0, 0, 0))
{}

BHTree::~BHTree()
{
	for (int i = 0; i < 8; i++)
	{
		delete subtree[i];
	}
}

void BHTree::Insert(Body* inserted_body, int depth)
{
	if (depth > MAX_DEPTH) return;

	if (!contains_body)
	{
		body = inserted_body;
		contains_body = true;
		is_external = true;
	}
	else
	{
		if (is_external)
		{
			CreateSubtree(body);
			subtree[oct.GetSubtree(body)]->Insert(body, depth+1);

			body = nullptr;

			CreateSubtree(inserted_body);
			subtree[oct.GetSubtree(inserted_body)]->Insert(inserted_body, depth+1);

			is_external = false;
		}
		else
		{
			CreateSubtree(inserted_body);
			subtree[oct.GetSubtree(inserted_body)]->Insert(inserted_body, depth+1);
		}
	}

	com = ((com * mass) + (inserted_body->position * inserted_body->mass)) / (mass + inserted_body->mass);
	mass = mass + inserted_body->mass;
}

void BHTree::CreateSubtree(Body* b)
{
	int i = oct.GetSubtree(b);

	if (subtree[i]) return;

	Oct o{ oct.center, oct.length / 2.0f };

	float half_length = o.length / 2.0f;

	o.center.x += (i % 2		== 0) ? -half_length : half_length;
	o.center.y += (i / 2 % 2	== 0) ? -half_length : half_length;
	o.center.z += (i / 4 % 2	== 0) ? -half_length : half_length;

	subtree[i] = new BHTree(o);
}

void BHTree::UpdateForce(Body* b)
{
	float d2 = glm::distance2(b->position, com);

	if (is_external)
	{
		if (b != body)
		{
			float v = G_CONST * mass / sqrt(d2 + E_CONST);
			glm::vec3 Direction = glm::normalize(b->position - com);
			b->velocity -= Direction * v;
		}
	}
	else
	{
		if (oct.length / sqrt(d2) < THRESHOLD)
		{
			float v = G_CONST * mass / sqrt(d2 + E_CONST);
			glm::vec3 Direction = glm::normalize(b->position - com);
			b->velocity -= Direction * v;
		}
		else
		{
			for (int i = 0; i < 8; i++)
			{
				if (subtree[i]) subtree[i]->UpdateForce(b);
			}
		}
	}
}