#include "collisions.h"

#include "raymath.h"

#include <stdio.h>

float DistFromPlane(Plane plane, Vector3 point) {
    float res = Vector3DotProduct(plane.norm, point) - plane.dist;
    // printf("[%.2f;%.2f;%.2f] -> %f: %f\n", point.x, point.y, point.z, plane.dist, res);
    return res;
}

bool CheckCollisionLinePlane(Line3d line, Plane plane, Vector3* outVec) {
    float d1 = DistFromPlane(plane, line.p1);
    float d2 = DistFromPlane(plane, line.p2);

    // printf("%f;%f\n", d1, d2);

    if (d1 * d2 > 0) {
        return false;
    }

    float t = d1 / (d1 - d2); // 'time' of intersection point on the segment
    *outVec = Vector3Add(line.p1, Vector3Scale(Vector3Subtract(line.p2, line.p1), t));

    // printf("POC: [%.2f;%.2f;%.2f]\n", outVec->x, outVec->y, outVec->z);

    return true;
}

bool CheckCollisionTrianglePlane(Triangle triangle, Plane plane, Line3d* outLine) {
    Vector3 temp = {0};
    int idx = 0;
    if (CheckCollisionLinePlane((Line3d){triangle.p1, triangle.p2}, plane, &temp)) {
        outLine->coords[idx++] = temp;
    }

    if (CheckCollisionLinePlane((Line3d){triangle.p2, triangle.p3}, plane, &temp)) {
        outLine->coords[idx++] = temp;
    }

    if (idx == 0) {
        return false;
    }

    if (idx == 2) {
        return true;
    }

    if (CheckCollisionLinePlane((Line3d){triangle.p3, triangle.p1}, plane, &temp)) {
        outLine->coords[idx++] = temp;
        return true;
    } else {
        return false;
    }
}

void Project(int vertexCount, Vector3* vertices, Vector3 axis, float* min, float* max) {
    *min = INFINITY;
    *max = -INFINITY;
    for (int i = 0; i < vertexCount; i++) {
        float val = Vector3DotProduct(vertices[i], axis);
        if (val < *min) {
            *min = val;
        }

        if (val > *max) {
            *max = val;
        }
    }    
}

void ProjectBoundingBox(BoundingBox box, Vector3 axis, float* min, float* max) {
    float width = box.max.x - box.min.x;
    float height = box.max.y - box.min.y;
    float length = box.max.z - box.min.z;
    Vector3 vertices[] = {
        box.min,
        {box.min.x, box.min.y + height, box.min.z},
        {box.min.x + width, box.min.y, box.min.z},
        {box.min.x, box.min.y, box.min.z + length},
        {box.min.x + width, box.min.y, box.min.z + length},
        {box.min.x, box.min.y + height, box.min.z + length},
        {box.min.x + width, box.min.y + height, box.min.z},
        box.max
    };

    Project(8, (Vector3*)&vertices, axis, min, max);
}

Vector3 TriangleNormal(Triangle triangle) {
    Vector3 A = Vector3Subtract(triangle.p2, triangle.p1);
    Vector3 B = Vector3Subtract(triangle.p3, triangle.p1);

    return Vector3CrossProduct(A, B);
}

bool CheckCollisionBoxTriangle(BoundingBox box, Triangle triangle) {
    float triangleMin, triangleMax;
    float boxMin, boxMax;

    // test box normals (x-, y- and z-axes)
    Vector3 boxNormals[] = {{1,0,0}, {0,1,0}, {0,0,1}};
    for (int i = 0; i < 3; i++) {
        Project(3, (Vector3*)&triangle, boxNormals[i], &triangleMin, &triangleMax);
        if (triangleMax < ((float*)&(box.min))[i] || triangleMin > ((float*)&(box.max))[i]) {
            return false;
        }
    }

    // Test the triangle normal
    Vector3 triangleNormal = TriangleNormal(triangle);
    float triangleOffset = Vector3DotProduct(triangleNormal, triangle.p1);
    ProjectBoundingBox(box, triangleNormal, &boxMin, &boxMax);
    if (boxMax < triangleOffset || boxMin > triangleOffset) {
        return false;
    }

    // Test the nine edge cross-products
    Vector3 triangleEdges[] = {
        Vector3Subtract(triangle.p1, triangle.p2),
        Vector3Subtract(triangle.p2, triangle.p3),
        Vector3Subtract(triangle.p3, triangle.p1)
    };

    float epsilon = 0.000001f;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Vector3 axis = Vector3CrossProduct(triangleEdges[i], boxNormals[j]);
            if (Vector3Length(axis) < epsilon) {
                continue;
            }

            ProjectBoundingBox(box, axis, &boxMin, &boxMax);
            Project(3, (Vector3*)&triangle, axis, &triangleMin, &triangleMax);
            if (boxMax <= triangleMin || boxMin >= triangleMax) {
                return false;
            }
        }
    }

    return true;
}

TriangleCollisionInfo CheckCollisionBoxMesh(BoundingBox box, Mesh mesh, Matrix transform) {
    TriangleCollisionInfo result = {0};
    for (int i = 0; i < mesh.triangleCount; i++) {
        Vector3 a, b, c;
        Vector3* vertdata = (Vector3*)mesh.vertices;

        if (mesh.indices)
        {
            a = vertdata[mesh.indices[i*3 + 0]];
            b = vertdata[mesh.indices[i*3 + 1]];
            c = vertdata[mesh.indices[i*3 + 2]];
        }
        else
        {
            a = vertdata[i*3 + 0];
            b = vertdata[i*3 + 1];
            c = vertdata[i*3 + 2];
        }

        a = Vector3Transform(a, transform);
        b = Vector3Transform(b, transform);
        c = Vector3Transform(c, transform);

        Triangle test = {a, b, c};
        if (CheckCollisionBoxTriangle(box, test)) {
            // printf("Hit %d/%d\n", i, mesh.triangleCount);
            result.hit = true;
            result.triangle = test;
            return result;
        }
    }

    return result;
}

bool CheckCollisionLineRec(Vector2 startPos, Vector2 endPos, Rectangle rec) {
    // check if the line has hit any of the rectangle's sides
    // uses the Line/Line function below
    Vector2 collisionPoint;
    bool left   = CheckCollisionLines(startPos, endPos, (Vector2) { rec.x, rec.y }, (Vector2) { rec.x, rec.y + rec.height }, &collisionPoint);
    bool right  = CheckCollisionLines(startPos, endPos, (Vector2) { rec.x + rec.width, rec.y }, (Vector2) { rec.x + rec.width, rec.y + rec.height }, &collisionPoint);
    bool top    = CheckCollisionLines(startPos, endPos, (Vector2) { rec.x, rec.y }, (Vector2) { rec.x + rec.width, rec.y }, &collisionPoint);
    bool bottom = CheckCollisionLines(startPos, endPos, (Vector2) { rec.x, rec.y + rec.height }, (Vector2) { rec.x + rec.width, rec.y + rec.height }, &collisionPoint);

    // if ANY of the above are true, the line
    // has hit the rectangle
    if (left || right || top || bottom) { return true; }
    return false;
}
