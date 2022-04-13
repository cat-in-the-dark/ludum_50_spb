#ifndef COLLISIONS_H
#define COLLISIONS_H

#include "raylib.h"

typedef struct {
    Vector3 p1;
    Vector3 p2;
    Vector3 p3;
} Triangle;

typedef struct {
    union {
        struct {
            Vector3 p1;
            Vector3 p2;
        };
        Vector3 coords[2];
    };
} Line3d;

typedef struct {
    Vector3 norm;
    float dist;
} Plane;

typedef struct {
    bool hit;
    Triangle triangle;
} TriangleCollisionInfo;

bool CheckCollisionTrianglePlane(Triangle triangle, Plane plane, Line3d* outLine);

bool CheckCollisionBoxTriangle(BoundingBox box, Triangle triangle);

TriangleCollisionInfo CheckCollisionBoxMesh(BoundingBox box, Mesh mesh, Matrix transform);

bool CheckCollisionLineRec(Vector2 startPos, Vector2 endPos, Rectangle rec);

#endif /* COLLISIONS_H */
