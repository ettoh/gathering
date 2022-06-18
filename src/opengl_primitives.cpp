#include "opengl_primitives.hpp"

namespace gathering {
namespace OpenGLPrimitives {

/////////////////////////////////////
/// Object
/////////////////////////////////////

size_t Object::totalVertexSize() const {
    if (vertices.size() != 0) return sizeof(vertices[0]) * vertices.size();
    return 0;
}

// ------------------------------------------------------------------------------------------------

size_t Object::totalElementSize() const {
    if (elements.size() != 0) {
        switch (gl_element_type) {
            case GL_UNSIGNED_SHORT:
                return sizeof(GLushort) * elements.size();
            case GL_UNSIGNED_BYTE:
                return sizeof(GLubyte) * elements.size();
            default:
                return sizeof(elements[0]) * elements.size();
        }
    }
    return 0;
}

// ------------------------------------------------------------------------------------------------

std::vector<GLushort> Object::elements_16() const {
    std::vector<GLushort> result;
    for (const auto& e : elements) {
        result.push_back(static_cast<GLushort>(e));
    }
    return result;
}

// ------------------------------------------------------------------------------------------------

std::vector<GLubyte> Object::elements_8() const {
    std::vector<GLubyte> result;
    for (const auto& e : elements) {
        result.push_back(static_cast<GLubyte>(e));
    }
    return result;
}

/////////////////////////////////////
// Object meshes
/////////////////////////////////////

Object createSphere(const float radius,
                    const glm::vec3 center,
                    const unsigned short accuracy,
                    const glm::vec4 color) {
    unsigned short number_of_stacks = accuracy;
    unsigned short number_of_sectors = accuracy * 2;
    float stack_step = static_cast<float>(M_PI) / number_of_stacks;
    float sector_step = 2.f * static_cast<float>(M_PI) / number_of_sectors;

    Object model = Object();
    model.gl_draw_mode = GL_TRIANGLES;

    /**
     * 1.   Rasterize the sphere with the analytic equation for spheres.
     *      Stacks move along the z-axis. Sectors move around the z-axis.
     * 2.   All vertices are stored contiguous in an array where every 3 elements form one
     * vertex. I.e. vertex id "0" refers to the vertex ranged from index 0 to index 2 in the
     * vericies array.
     * 3.   3 contiguous id's in the element array form a triangle (counterclockwise).
     * 4.   For every sector (x) on a stack we define two triangles on the right side. n is the
     * number of sectors in a stack:
     *
     *      stack i+1    x+n-------x+n+1
     *                    |       / |
     *                    |     /   |
     *                    |   /     |
     *                    | /       |
     *      stack i       x---------x+1
     *                sector x    sector x+1
     */

    // rasterize a sphere with the analytic equation for spheres
    for (int i = 0; i <= number_of_stacks; ++i) {       // stacks
        for (int j = 0; j <= number_of_sectors; ++j) {  // sectors
            VertexData vertex = VertexData();

            // Vertices
            float stack_angle = static_cast<float>(M_PI) / 2.f - (i * stack_step);
            float sector_angle = (j * sector_step);

            vertex.position.x = center.y + radius * cos(stack_angle) * sin(sector_angle);  // y
            vertex.position.y = -center.z - radius * sin(stack_angle);                     // z
            vertex.position.z = center.x + radius * cos(stack_angle) * cos(sector_angle);  // x

            // texcoords
            vertex.texture.x = (float)j / number_of_sectors;
            vertex.texture.y = (float)i / number_of_stacks;

            // normal
            glm::vec3 position = vertex.position - center;
            vertex.normal = glm::normalize(position);

            // colors
            vertex.color = color;

            model.vertices.push_back(vertex);
        }
    }

    // define the triangles between the vertices
    for (unsigned short i = 0; i < number_of_stacks; ++i) {       // stacks
        for (unsigned short j = 0; j < number_of_sectors; ++j) {  // sectors
            // calculate id's from the previously defined vertices
            unsigned short base_point = j + i * number_of_sectors + i;  // x
            unsigned short right = (base_point + 1);                    // x+1
            unsigned short top = base_point + number_of_sectors + 1;    // x+n
            unsigned short top_right = (top + 1);                       // x+n+1

            // Triangle 1 ignoring last stack
            if (i != (number_of_stacks - 1)) {
                model.elements.push_back(base_point);
                model.elements.push_back(top_right);
                model.elements.push_back(top);
            }

            // Triangle 2 ignoring first stack
            if (i != 0) {
                model.elements.push_back(base_point);
                model.elements.push_back(right);
                model.elements.push_back(top_right);
            }
        }
    }

    return model;
}

// ------------------------------------------------------------------------------------------------

Object createCube(const float size, const glm::vec3 center, const glm::vec4 color) {
    return createCuboid(glm::vec3(size), center, color);
}

// ------------------------------------------------------------------------------------------------

Object createCuboid(const glm::vec3 size, const glm::vec3 center, const glm::vec4 color) {
    Object m = Object();
    m.gl_draw_mode = GL_TRIANGLES;

    VertexData v;
    v.color = color;
    glm::vec3 half_size = size / 2.f;
    v.position = center + glm::vec3(1, -1, 1) * half_size;  // front right down 0
    v.normal = glm::normalize(v.position - center);
    m.vertices.push_back(v);
    v.position = center + glm::vec3(1, 1, 1) * half_size;  // front right up 1
    v.normal = glm::normalize(v.position - center);
    m.vertices.push_back(v);
    v.position = center + glm::vec3(-1, 1, 1) * half_size;  // front left up 2
    v.normal = glm::normalize(v.position - center);
    m.vertices.push_back(v);
    v.position = center + glm::vec3(-1, -1, 1) * half_size;  // front left down 3
    v.normal = glm::normalize(v.position - center);
    m.vertices.push_back(v);
    v.position = center + glm::vec3(1, -1, -1) * half_size;  // back right down 4
    v.normal = glm::normalize(v.position - center);
    m.vertices.push_back(v);
    v.position = center + glm::vec3(1, 1, -1) * half_size;  // back right up 5
    v.normal = glm::normalize(v.position - center);
    m.vertices.push_back(v);
    v.position = center + glm::vec3(-1, 1, -1) * half_size;  // back left up 6
    v.normal = glm::normalize(v.position - center);
    m.vertices.push_back(v);
    v.position = center + glm::vec3(-1, -1, -1) * half_size;  // back left down 7
    v.normal = glm::normalize(v.position - center);
    m.vertices.push_back(v);

    m.elements = {0, 1, 3, 3, 1, 2, 4, 5, 0, 0, 5, 1, 7, 6, 4, 4, 6, 5,
                  3, 2, 7, 7, 2, 6, 2, 1, 5, 2, 5, 6, 0, 4, 3, 3, 4, 7};

    return m;
}

// ------------------------------------------------------------------------------------------------

Object createLine(const glm::vec3& p1,
                  const glm::vec3& p2,
                  const glm::vec4& color,
                  bool dashed) {
    Object m = Object();
    m.gl_draw_mode = GL_LINES;

    // for every colored segment we need a transparent counterpart - except the last segment
    int colored_segments = dashed ? 15 : 1;
    int segments = 2 * colored_segments - 1;
    glm::vec3 distance_vector = p2 - p1;

    for (float i = 0; i < colored_segments; i++) {
        VertexData v1;
        v1.color = color;
        v1.position = p1 + distance_vector * (i * 2 / segments);
        m.vertices.push_back(v1);

        VertexData v2;
        v2.color = color;
        v2.position = p1 + distance_vector * ((i * 2 + 1) / segments);
        m.vertices.push_back(v2);
    }

    return m;
}

// ------------------------------------------------------------------------------------------------

Object createPipe(const float radius,
                  const float height,
                  const glm::vec4 color,
                  const unsigned int sector_count) {
    Object m = Object();
    m.gl_draw_mode = GL_TRIANGLE_STRIP;
    if (sector_count < 3) return m;

    // vertices
    const float PI = 3.1415926f;
    float sector_step = 2 * PI / sector_count;
    for (unsigned int i = 0; i <= sector_count; i++) {
        float angle = i * sector_step;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.f, z));

        VertexData v0(x, -height / 2, z);
        v0.color = color;
        v0.normal = normal;
        VertexData v1(x, height / 2, z);
        v1.color = color;
        v1.normal = normal;

        m.vertices.push_back(v0);
        m.vertices.push_back(v1);
    }

    // primitives by indices
    // first triangle
    m.elements.push_back(1);
    m.elements.push_back(0);
    m.elements.push_back(3);
    unsigned short last_vertex_idx = 3u;

    // for each additional triangle, add one index (TRIANGLE STRIP)
    for (unsigned int i = 0; i + 1 < 2 * sector_count; i++) {
        if (i % 2 == 0)
            last_vertex_idx -= 1;
        else
            last_vertex_idx += 3;

        m.elements.push_back(last_vertex_idx % (2 * sector_count));
    }
    return m;
}

// ------------------------------------------------------------------------------------------------

Object createCone(const float base_radius,
                  const float height,
                  const glm::vec4 color,
                  const unsigned short sector_count,
                  const bool centered) {
    Object m = Object();
    m.gl_draw_mode = GL_TRIANGLES;
    if (sector_count < 3) return m;

    const float half_height = height / 2;

    // top vertex (reused for different normals)
    VertexData top_vertex;
    top_vertex = centered ? VertexData(0.f, half_height, 0.f) : VertexData(0.f, 0.f, 0.f);
    top_vertex.color = color;

    // bottom vertex
    VertexData bottom_vertex;
    bottom_vertex =
        centered ? VertexData(0.f, -half_height, 0.f) : VertexData(0.f, -height, 0.f);
    bottom_vertex.color = color;
    bottom_vertex.normal = glm::vec3(0.f, -1.f, 0.f);
    m.vertices.push_back(bottom_vertex);  // index 0

    // parameter for normals
    const float side_length = sqrtf(height * height + base_radius * base_radius);
    const float nx = height / side_length;
    const float ny = base_radius / side_length;

    // vertices for the sides
    const float PI = 3.1415926f;
    float sector_step = 2 * PI / sector_count;
    VertexData v;
    v.color = color;
    for (unsigned int i = 0; i <= sector_count; i++) {
        float angle = i * sector_step;
        float x = base_radius * cos(angle);
        float z = base_radius * sin(angle);
        v.position = centered ? glm::vec3(x, -half_height, z) : glm::vec3(x, -height, z);

        glm::vec3 normal(nx * cos(angle), ny, nx * sin(angle));  // rotation of initial normal
        v.normal = glm::normalize(normal);
        top_vertex.normal = glm::normalize(normal + glm::vec3(0, -1, 0));  // bisector
        m.vertices.push_back(top_vertex);
        m.vertices.push_back(v);

        // vertex for bottom circle
        v.normal = glm::vec3(0.f, -1.f, 0.f);
        m.vertices.push_back(v);
    }

    // elements for sides
    for (unsigned short i = 1; i / 3 < sector_count; i += 3) {
        m.elements.push_back(i);
        m.elements.push_back(i + 1);
        m.elements.push_back((i + 4) % (3 * sector_count));
    }

    // elements for bottom circle
    for (unsigned short i = 1; i < sector_count; i++) {
        m.elements.push_back(0u);
        m.elements.push_back(i * 3u);
        m.elements.push_back(i * 3u + 3u);
    }
    m.elements.push_back(0u);
    m.elements.push_back(3u * sector_count);
    m.elements.push_back(3u);

    return m;
}

// ------------------------------------------------------------------------------------------------

Object createTriangle(const glm::vec3 a, const glm::vec3 b, const glm::vec3 c) {
    Object m = Object();
    m.gl_draw_mode = GL_TRIANGLES;
    VertexData v;
    v.color = glm::vec4(1.f, 0.f, 0.f, 1.f);
    v.position = a;
    m.vertices.push_back(v);
    v.position = b;
    m.vertices.push_back(v);
    v.position = c;
    m.vertices.push_back(v);

    return m;
}

}  // namespace OpenGLPrimitives
}  // namespace gathering
