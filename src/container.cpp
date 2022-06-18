#include "container.hpp"

#include <iostream>

namespace gathering {

Grid::Grid(const vec3i& resolution, const AABB& aabb) : resolution(resolution), aabb(aabb) {
    size = aabb.max - aabb.min;
    grid = IDXList3D(resolution.x, resolution.y, resolution.z, GridCell());

    cell_size = size / (glm::vec3)resolution;
    cell_radius = cell_size / 2.0f;

    // precompute cell center
    for (int x = 0; x < resolution.x; ++x) {
        for (int y = 0; y < resolution.y; ++y) {
            for (int z = 0; z < resolution.z; ++z) {
                grid.at(x, y, z).center =
                    aabb.min + cell_size * glm::vec3(x + 1, y + 1, z + 1) - cell_radius;
            }
        }
    }

    // compute neighbour indices
    for (int i = 0; i < 8; ++i) {
        int offset_x = (i & 1);
        int offset_y = (i >> 1) & 1;
        int offset_z = (i >> 2) & 1;

        for (int x = -1 + offset_x; x < 1 + offset_x; ++x) {
            for (int y = -1 + offset_y; y < 1 + offset_y; ++y) {
                for (int z = -1 + offset_z; z < 1 + offset_z; ++z) {
                    neighbours[i].push_back(glm::vec3(x, y, z));
                }
            }
        }
    }
}

vec3i Grid::coords(const glm::vec3& pos) const {
    // TODO point in AABB of vessel??
    return ((pos - aabb.min) * (glm::vec<3, float>)resolution) / size;
}

void Grid::insert(const glm::vec3& pos, size_t idx) {
    grid.at(coords(pos)).content.push_back(idx);
}

void Grid::insert(const vec3i& coords, size_t idx) { grid.at(coords).content.push_back(idx); }

void Grid::clear(const vec3i& coords) { grid.at(coords).content.clear(); }

const glm::vec3& Grid::cellCenter(const vec3i& coords) const { return grid.at(coords).center; }

void Grid::closeUniqueElements(const vec3i& coords,
                               const size_t& self_idx,
                               const int neighbours_idx,
                               std::vector<size_t>& output) {
    for (const vec3i& offset : neighbours[neighbours_idx]) {
        const GridCell& cell = grid.at(coords + offset);
        for (const size_t& e : cell.content) {
            if (e > self_idx)
                output.push_back(
                    e);  // ignore self-collision and avoid collisions reported twice
        }
    }
}

}  // namespace gathering
