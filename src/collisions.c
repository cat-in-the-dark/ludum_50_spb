#include "collisions.h"

#include "raymath.h"

#include <stdio.h>

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
            printf("Hit %d/%d\n", i, mesh.triangleCount);
            result.hit = true;
            result.triangle = test;
            return result;
        }
    }

    return result;
}
