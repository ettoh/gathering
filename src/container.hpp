#ifndef GATHERING_CONTAINER_H
#define GATHERING_CONTAINER_H

#include <memory>
#include <vector>

#include "gathering/glm_include.hpp"
#include "particle.hpp"  // AABB

namespace gathering {

template <typename T>
class Array3D {
   public:
    Array3D(size_t size_x, size_t size_y, size_t size_z, const T& error_element)
        : sx(size_x), sy(size_y), sz(size_z), error_element(error_element) {
        b_x = size_x;
        b_xy = size_x * size_y;
        data = std::shared_ptr<T[]>(new T[size_x * size_y * size_z]);
    }

    Array3D(size_t size_x,
            size_t size_y,
            size_t size_z,
            const T& default_element,
            const T& error_element)
        : sx(size_x), sy(size_y), sz(size_z), error_element(error_element) {
        b_x = size_x;
        b_xy = size_x * size_y;
        data = std::shared_ptr<T[]>(new T[size_x * size_y * size_z]{default_element});
    }

    Array3D(const Array3D&) = delete;
    T& at(size_t x, size_t y, size_t z) {
        if (x < 0 || x >= sx || y < 0 || y >= sy || z < 0 || z >= sz) return error_element;
        return data[x + y * b_x + z * b_xy];
    }
    T& at(const vec3i& i) {
        if (i.x < 0 || i.x >= sx || i.y < 0 || i.y >= sy || i.z < 0 || i.z >= sz)
            return error_element;
        return data[i.x + i.y * b_x + i.z * b_xy];
    }
    const T& at(size_t x, size_t y, size_t z) const {
        if (x < 0 || x >= sx || y < 0 || y >= sy || z < 0 || z >= sz) return error_element;
        return data[i.x + i.y * b_x + i.z * b_xy];
    }
    const T& at(const vec3i& i) const {
        if (i.x < 0 || i.x >= sx || i.y < 0 || i.y >= sy || i.z < 0 || i.z >= sz)
            return error_element;
        return data[i.x + i.y * b_x + i.z * b_xy];
    }

   private:
    std::shared_ptr<T[]> data;
    size_t b_xy, b_x;
    size_t sx, sy, sz;
    T error_element;
};

struct GridCell {
    glm::vec3 center = glm::vec3(0);
    std::vector<size_t> content;
};

typedef Array3D<GridCell> IDXList3D;

class Grid {
   public:
    Grid() = default;
    Grid(const vec3i& resolution, const AABB& aabb);

    Grid(const Grid&) = delete;
    vec3i coords(const glm::vec3& pos) const;
    void insert(const glm::vec3& pos, size_t idx);
    void insert(const vec3i& coords, size_t idx);
    void clear(const vec3i& coords);
    const glm::vec3& cellCenter(const vec3i& coords) const;
    void closeUniqueElements(const vec3i& coords,
                             const size_t& self_idx,
                             const int neighbours_idx,
                             std::vector<size_t>& output);

   private:
    std::vector<glm::vec3> neighbours[8];
    IDXList3D grid = IDXList3D(0, 0, 0, GridCell());
    AABB aabb;
    glm::vec3 cell_size = glm::vec3(0);
    glm::vec3 cell_radius = glm::vec3(0);
    glm::vec3 size = glm::vec3(1, 1, 1);
    vec3i resolution = vec3i(0, 0, 0);
};
}  // namespace gathering

#endif