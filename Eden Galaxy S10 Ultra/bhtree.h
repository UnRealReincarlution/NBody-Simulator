#pragma once

#include <glm/gtx/norm.hpp>

const float G_CONST = 6.674e-5; //6.674e-11;
const float E_CONST = 1e-20;
const float THRESHOLD = 0.5;
const int MAX_DEPTH = 3;

class Body;

struct Oct
{
	glm::vec3 center;
	float length;

	const bool Contains(Body* b);
	const int GetSubtree(Body* b);
};

class BHTree
{
public:
	BHTree(Oct o);
	~BHTree();

	void Insert(Body* b, int depth = 0);
	void UpdateForce(Body* b);
	void CreateSubtree(Body* b);

	bool contains_body;
	bool is_external;

	Body* body;
	Oct oct;

	glm::vec3 com;
	float mass;

	BHTree* subtree[8];
};

