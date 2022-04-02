#ifndef COLLISIONS_H
#define COLLISIONS_H

#include "raylib.h"

typedef struct {
    Vector3 p1;
    Vector3 p2;
    Vector3 p3;
} Triangle;

typedef struct {
    bool hit;
    Triangle triangle;
} TriangleCollisionInfo;

bool CheckCollisionBoxTriangle(BoundingBox box, Triangle triangle);

TriangleCollisionInfo CheckCollisionBoxMesh(BoundingBox box, Mesh mesh, Matrix transform);


#endif /* COLLISIONS_H */
