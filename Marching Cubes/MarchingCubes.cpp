#include "MarchingCubes.h"

Vector3D::Vector3D(int _x, int _y, int _z) {
	this->x = _x;
	this->y = _y;
	this->z = _z;
}

Vector3D::Vector3D(const Vector3D& v) {
	this->x = v.x;
	this->y = v.y;
	this->z = v.z;
}