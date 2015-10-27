/*!
*
* \author VaHiD AzIzI
*
*/


#include "obstacles/GJK_EPA.h"


SteerLib::GJK_EPA::GJK_EPA()
{
}

//Look at the GJK_EPA.h header file for documentation and instructions
bool SteerLib::GJK_EPA::intersect(float& return_penetration_depth, Util::Vector& return_penetration_vector, const std::vector<Util::Vector>& _shapeA, const std::vector<Util::Vector>& _shapeB)
{
	std::vector<Util::Vector> simplex;
    bool retval = gjk(_shapeA, _shapeB, simplex); 

    if (retval) {
    	// There is a collision
    	epa(_shapeA, _shapeB, simplex, return_penetration_depth, return_penetration_vector);
    } 

	return retval;
}

bool SteerLib::GJK_EPA::gjk(const std::vector<Util::Vector>& shapeA, const std::vector<Util::Vector>& shapeB, std::vector<Util::Vector>& simplex) {
	Util::Vector centerA = getShapeCenter(shapeA);
	Util::Vector centerB = getShapeCenter(shapeB);
	Util::Vector direction = centerB - centerA;
	simplex.push_back(getSimplexPointUsingDirection(shapeA, shapeB, direction));
	direction = -direction;
	while (true) {
		simplex.push_back(getSimplexPointUsingDirection(shapeA, shapeB, direction));
		if (dot(simplex.back(), direction) <= 0) return false;
		else if (simplexContainsOrigin(simplex, direction)) {
			if (simplex.size() < 3) {
				// The simplex will have 2 points in this case (2 points have already been added)
				simplex.push_back(getSimplexPointUsingDirection(shapeA, shapeB, direction));
			}
			return true;
		}
	}
}

bool SteerLib::GJK_EPA::simplexContainsOrigin(std::vector<Util::Vector>& simplex, Util::Vector& direction) {
	Util::Vector ptA = simplex.back();
	Util::Vector aToOrigin = -ptA;
	if (simplex.size() == 3) {
		Util::Vector ptB = simplex[1];
		Util::Vector ptC = simplex[0];
		Util::Vector aToB = ptB - ptA;
		Util::Vector aToC = ptC - ptA;
		Util::Vector abPerp = aToB * dot(aToB, aToC) - aToC * (dot(aToB, aToB));
		Util::Vector acPerp = aToC * dot(aToC, aToB) - aToB * (dot(aToC, aToC));
		if (dot(abPerp, aToOrigin) > 0) {
			simplex.erase(simplex.begin());
			direction = abPerp;
		}
		else if (dot(acPerp, aToOrigin) > 0) {
			simplex.erase(simplex.begin() + 1);
			direction = acPerp;
		}
		else return true;
	}
	else {
		Util::Vector ptB = simplex.at(0);
		Util::Vector aToB = ptB - ptA;
		Util::Vector abPerp = aToOrigin * dot(aToB, aToB) - aToB * dot(aToB, aToOrigin);
		direction = abPerp;
		if (dot(abPerp, aToOrigin) == 0) {
			float aToBDotaToOrigin = dot(aToB, aToOrigin);
			if (aToBDotaToOrigin >= 0 && aToBDotaToOrigin < dot(aToB, aToB)) return true;
		}
	}
	return false;
}

// support function
Util::Vector SteerLib::GJK_EPA::getSimplexPointUsingDirection(const std::vector<Util::Vector>& shapeA, const std::vector<Util::Vector>& shapeB, const Util::Vector& direction) {
	Util::Vector retVal = getFurthestPointInDirection(shapeA, direction) - getFurthestPointInDirection(shapeB, -direction);
	return retVal;
}

Util::Vector SteerLib::GJK_EPA::getShapeCenter(const std::vector<Util::Vector>& shape) {
	Util::Vector retVal(0, 0, 0);
	for (int i = 0; i < shape.size(); i++) {
		Util::Vector p = shape[i];
		retVal[0] += p[0];
		retVal[2] += p[2];
	}
	retVal[0] = retVal[0] / (float)shape.size();
	retVal[2] = retVal[2] / (float)shape.size();
	return retVal;
}

Util::Vector SteerLib::GJK_EPA::getFurthestPointInDirection(const std::vector<Util::Vector>& shape, const Util::Vector& direction) {
	Util::Vector retVal(0, 0, 0);
	float farthestDistance = dot(shape[0], direction);
	int farthestIndex = 0;
	for (int i = 1; i < shape.size(); i++) {
		float dotProd = dot(shape[i], direction);
		if (dotProd > farthestDistance) {
			farthestDistance = dotProd;
			farthestIndex = i;
		}
	}
	retVal[0] = shape[farthestIndex][0];
	retVal[1] = shape[farthestIndex][1];
	retVal[2] = shape[farthestIndex][2];
	return retVal;
}

float SteerLib::GJK_EPA::dot(const Util::Vector& vectorA, const Util::Vector& vectorB) {
	float retVal = 0;
	for (int i = 0; i < 3; i++) {
		retVal += vectorA[i] * vectorB[i];
	}
	return retVal;
}

void SteerLib::GJK_EPA::epa(const std::vector<Util::Vector>& _shapeA, const std::vector<Util::Vector>& _shapeB, std::vector<Util::Vector>& simplex, float& return_penetration_depth, Util::Vector& return_penetration_vector)
{
	float TOLERANCE = 0.00001;

	while (true)
	{
		float distance;
		Util::Vector normal;
		int index;
		findClosestEdge(simplex, distance, normal, index);

		Util::Vector supportPoint = getSimplexPointUsingDirection(_shapeA, _shapeB, normal);

		double d = supportPoint * normal;
		if (d - distance < TOLERANCE) {
			return_penetration_vector = normal * -1.0;
			return_penetration_depth = d;
			return;
		} else {
			simplex.insert(simplex.begin()+index, supportPoint);
		}
	}
}

Util::Vector SteerLib::GJK_EPA::tripleProduct(Util::Vector A, Util::Vector B, Util::Vector C)
{
	return (B * (C * A)) - (A * (C * B));
}

void SteerLib::GJK_EPA::findClosestEdge(std::vector<Util::Vector> simplex, float& distance, Util::Vector& normal, int& index)
{
	distance = FLT_MAX;
	for (int i = 0; i < simplex.size(); i++) {
		int j = i + 1 == simplex.size() ? 0 : i + 1;
		Util::Vector a = simplex[i];
		Util::Vector b = simplex[j];
		Util::Vector e = b - a;
		Util::Vector oa = a;
		Util::Vector n = tripleProduct(e, oa, e);

		Util::Vector n_norm = n / n.norm();

		double d = n_norm * a;
		if (d < distance) {
			distance = d;
			normal = n_norm;
			index = j;
		}
	}
}
