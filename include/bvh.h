#include "raylib.h"

// Adapted from
// https://github.com/jbikker/bvh_article/blob/main/quickbuild.cpp

typedef struct RayCollisionUV
{
    bool hit;            
    float distance;
    Vector3 point;
    Vector3 normal;
    Vector3 barrycenter;
    Vector2 u1, u2, u3;

    Vector2 getUV()
    {
        Vector2 uv;
        uv.x = barrycenter.x * u1.x +
               barrycenter.y * u2.x +
               barrycenter.z * u3.x;

        uv.y = barrycenter.x * u1.y +
               barrycenter.y * u2.y +
               barrycenter.z * u3.y;

        return uv;
    }
    
} RayCollisionUV;

typedef struct Triangle
{
    Vector3 a, b, c;
    Vector2 u1, u2, u3;
    Vector3 centroid;
    BoundingBox bounding_box;
} Triangle;

// Header Functions
Triangle GetTriangle(Mesh& mesh, unsigned int id, Matrix transform);
BoundingBox GetTriangleBoundingBox(Triangle& tri);

Triangle GetTriangle(Mesh& mesh, unsigned int id, Matrix transform)
{
    Triangle tri;
    Vector3 a, b, c;
    Vector2 u1, u2, u3;

    if (mesh.indices)
    {   
        unsigned short ida = mesh.indices[id*3 + 0];
        unsigned short idb = mesh.indices[id*3 + 1];
        unsigned short idc = mesh.indices[id*3 + 2];
        a = { mesh.vertices[ida*3 + 0], mesh.vertices[ida*3 + 1], mesh.vertices[ida*3 + 2] };
        b = { mesh.vertices[idb*3 + 0], mesh.vertices[idb*3 + 1], mesh.vertices[idb*3 + 2] };
        c = { mesh.vertices[idc*3 + 0], mesh.vertices[idc*3 + 1], mesh.vertices[idc*3 + 2] };

        u1 = { mesh.texcoords[ida*3 + 0], mesh.texcoords[ida*3 + 1]};
        u2 = { mesh.texcoords[idb*3 + 0], mesh.texcoords[idb*3 + 1]};
        u3 = { mesh.texcoords[idc*3 + 0], mesh.texcoords[idc*3 + 1]};
    }
    else
    {
        a = { mesh.vertices[(id*3 + 0)*3 + 0], mesh.vertices[(id*3 + 0)*3 + 1], mesh.vertices[(id*3 + 0)*3 + 2] };
        b = { mesh.vertices[(id*3 + 1)*3 + 0], mesh.vertices[(id*3 + 1)*3 + 1], mesh.vertices[(id*3 + 1)*3 + 2] };
        c = { mesh.vertices[(id*3 + 2)*3 + 0], mesh.vertices[(id*3 + 2)*3 + 1], mesh.vertices[(id*3 + 2)*3 + 2] };

        u1 = { mesh.texcoords[(id*3 + 0)*2 + 0], mesh.texcoords[(id*3 + 0)*2 + 1]};
        u2 = { mesh.texcoords[(id*3 + 1)*2 + 0], mesh.texcoords[(id*3 + 1)*2 + 1]};
        u3 = { mesh.texcoords[(id*3 + 2)*2 + 0], mesh.texcoords[(id*3 + 2)*2 + 1]};
    }

    tri.a = Vector3Transform(a, transform);
    tri.b = Vector3Transform(b, transform);
    tri.c = Vector3Transform(c, transform);
    
    tri.u1 = u1;
    tri.u2 = u2;
    tri.u3 = u3;

    tri.centroid.x = (a.x + b.x + c.x)/3;
    tri.centroid.y = (a.y + b.y + c.y)/3;
    tri.centroid.z = (a.z + b.z + c.z)/3;

    tri.bounding_box = GetTriangleBoundingBox(tri);

    return tri;
}

BoundingBox GetTriangleBoundingBox(Triangle& tri)
{
    BoundingBox bounds = {0};
    bounds.min = {1e30f};
    bounds.max = {-1e30f};
    bounds.min.x = fminf(bounds.min.x, tri.a.x);
    bounds.min.x = fminf(bounds.min.x, tri.b.x);
    bounds.min.x = fminf(bounds.min.x, tri.c.x);
    bounds.min.y = fminf(bounds.min.y, tri.a.y);
    bounds.min.y = fminf(bounds.min.y, tri.b.y);
    bounds.min.y = fminf(bounds.min.y, tri.c.y);
    bounds.min.z = fminf(bounds.min.z, tri.a.z);
    bounds.min.z = fminf(bounds.min.z, tri.b.z);
    bounds.min.z = fminf(bounds.min.z, tri.c.z);
    bounds.max.x = fmaxf(bounds.max.x, tri.a.x);
    bounds.max.x = fmaxf(bounds.max.x, tri.b.x);
    bounds.max.x = fmaxf(bounds.max.x, tri.c.x);
    bounds.max.y = fmaxf(bounds.max.y, tri.a.y);
    bounds.max.y = fmaxf(bounds.max.y, tri.b.y);
    bounds.max.y = fmaxf(bounds.max.y, tri.c.y);
    bounds.max.z = fmaxf(bounds.max.z, tri.a.z);
    bounds.max.z = fmaxf(bounds.max.z, tri.b.z);
    bounds.max.z = fmaxf(bounds.max.z, tri.c.z);

    return bounds;
}

typedef struct Node{
    BoundingBox bounding_box;
    unsigned int left_first, triangle_count;
    bool isLeaf() {return triangle_count > 0;}
} Node;

class BVH 
{
    private:
        Node *tree_model;
        Triangle *triangles;
        unsigned int *triangles_id;
        unsigned int nodes_used;
        Matrix transform_model;
        unsigned int triangle_count;

        void UpdateNodeBounds(unsigned int node_id);
        void Subdivide(unsigned int node_id);
        void RecursiveSearch(Ray &ray, unsigned int id, RayCollisionUV &results);

    public:
        bool is_built = false;

        RayCollision BruteSearch(Ray& ray);
        RayCollisionUV Search(Ray& ray);
        BVH(Mesh& mesh, Matrix transform);
        ~BVH();
};

BVH::BVH(Mesh& mesh, Matrix transform)
{
    if (mesh.vertices != NULL)
    {
        const unsigned int triangle_count = mesh.triangleCount;
        this->tree_model = new Node[triangle_count*2];
        this->triangles = new Triangle[triangle_count];
        this->triangles_id = new unsigned int[triangle_count];
        this->triangle_count = triangle_count;

        for (unsigned int i = 0; i < triangle_count; i++)
        {
            this->triangles_id[i] = i;
            this->triangles[i] = GetTriangle(mesh, i, transform);
        }

        this->tree_model[0].triangle_count = triangle_count;
        this->tree_model[0].bounding_box = GetMeshBoundingBox(mesh);
        this->tree_model[0].left_first = 0;
        this->nodes_used = 1;

        this->Subdivide(0);
        is_built = true;

        this->transform_model = transform;
    }
}

void BVH::Subdivide(unsigned int node_id)
{
    // terminate recursion
	Node& node = this->tree_model[node_id];
	if (node.triangle_count <= 2) return;

    // Determine which axis to perform the triangle division
    float delta_axis[3] = {node.bounding_box.max.x - node.bounding_box.min.x,
                           node.bounding_box.max.y - node.bounding_box.min.y,
                           node.bounding_box.max.z - node.bounding_box.min.z};
    int axis = 0;
    if (delta_axis[1] > delta_axis[0]) axis = 1;
    if (delta_axis[2] > delta_axis[axis]) axis = 2;
    float bounds_min[3] = {node.bounding_box.min.x,
                           node.bounding_box.min.y,
                           node.bounding_box.min.z};
    float split_pos = bounds_min[axis] + delta_axis[axis] * 0.5f;

	// in-place partition
    int i = node.left_first;
	int j = i + node.triangle_count - 1;
	while (i <= j)
	{
        float centroid[3] = {this->triangles[this->triangles_id[i]].centroid.x,
                             this->triangles[this->triangles_id[i]].centroid.y,
                             this->triangles[this->triangles_id[i]].centroid.z};
        if (centroid[axis]< split_pos) i++;
        else
        {
            unsigned int swap_idx = this->triangles_id[j];
            this->triangles_id[j] = this->triangles_id[i];
            this->triangles_id[i] = swap_idx;
            j--;
        }
	}

	// abort split if one of the sides is empty
	int left_count = i - node.left_first;
	if (left_count == 0 || left_count == node.triangle_count) return;
	// create child nodes
	int left_child_idx = this->nodes_used++;
	int right_child_idx = this->nodes_used++;
	this->tree_model[left_child_idx].left_first = node.left_first;
	this->tree_model[left_child_idx].triangle_count = left_count;
	this->tree_model[right_child_idx].left_first = i;
	this->tree_model[right_child_idx].triangle_count = node.triangle_count - left_count;
	node.left_first = left_child_idx;
	node.triangle_count = 0;
	this->UpdateNodeBounds(left_child_idx);
	this->UpdateNodeBounds(right_child_idx);
	// recurse
	this->Subdivide( left_child_idx );
	this->Subdivide( right_child_idx );
}

void BVH::UpdateNodeBounds(unsigned int node_id)
{
    Node& node = this->tree_model[node_id];
    node.bounding_box.min = {1e30f};
    node.bounding_box.max = {-1e30f};

    for (unsigned int i = 0; i < node.triangle_count; i++)
    {
        unsigned int triangle_id = this->triangles_id[node.left_first+i];
		Triangle& triangle = this->triangles[triangle_id];
        node.bounding_box.min.x = fminf(node.bounding_box.min.x, triangle.bounding_box.min.x);
        node.bounding_box.min.y = fminf(node.bounding_box.min.y, triangle.bounding_box.min.y);
        node.bounding_box.min.z = fminf(node.bounding_box.min.z, triangle.bounding_box.min.z);
        node.bounding_box.max.x = fmaxf(node.bounding_box.max.x, triangle.bounding_box.max.x);
        node.bounding_box.max.y = fmaxf(node.bounding_box.max.y, triangle.bounding_box.max.y);
        node.bounding_box.max.z = fmaxf(node.bounding_box.max.z, triangle.bounding_box.max.z);
    }
}

RayCollisionUV BVH::Search(Ray& ray)
{
    RayCollisionUV results = {0};
    unsigned int id = 0;
    this->RecursiveSearch(ray, id, results);
    return results;
}

void BVH::RecursiveSearch(Ray &ray, unsigned int id, RayCollisionUV &results)
{
    Node& node = this->tree_model[id];
    RayCollision bbox_results = GetRayCollisionBox(ray, node.bounding_box);
    if (bbox_results.hit)
    {
        if (node.isLeaf())
        {
            for (unsigned int i=0; i < node.triangle_count; i++)
            {
                Triangle& triangle = this->triangles[this->triangles_id[node.left_first+i]];
                RayCollision tri_hit = GetRayCollisionTriangle(ray, triangle.a, triangle.b, triangle.c);
                if (tri_hit.hit)
                {
                    if ((!results.hit) || (results.distance > tri_hit.distance))
                    {
                        results.hit = tri_hit.hit;
                        results.distance = tri_hit.distance;
                        results.normal = tri_hit.normal;
                        results.point = tri_hit.point;
                        results.u1 = triangle.u1;
                        results.u2 = triangle.u2;
                        results.u3 = triangle.u3;
                        results.barrycenter = Vector3Barycenter(tri_hit.point, triangle.a, triangle.b, triangle.c);
                    } 
                }
            }
        }
        else
        {
            this->RecursiveSearch(ray, node.left_first, results);
            this->RecursiveSearch(ray, (unsigned int)(node.left_first + 1), results);
        }
    }
}

RayCollision BVH::BruteSearch(Ray& ray)
{
    RayCollision results = {0};
    unsigned int id = 0;

    for (unsigned int i = 0; i < this->triangle_count; i++)
    {
        Triangle& triangle = this->triangles[i];
        RayCollision tri_hit = GetRayCollisionTriangle(ray, triangle.a, triangle.b, triangle.c);
        if (tri_hit.hit)
        {
            if ((!results.hit) || (results.distance > tri_hit.distance))
                results = tri_hit;
        }
    }
    return results;
}

BVH::~BVH()
{
    delete[] this->triangles;
    delete[] this->tree_model;
    delete[] this->triangles_id;
}