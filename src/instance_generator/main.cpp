#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using std::to_string;

int main() {
    int object[5][5][5] = {
        {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {1, 1, 1, 1, 1}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}},
        {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {1, 0, 1, 0, 1}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}},
        {{1, 1, 1, 1, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1}, {1, 0, 1, 0, 1}, {1, 1, 1, 1, 1}},
        {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {1, 0, 1, 0, 1}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}},
        {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {1, 1, 1, 1, 1}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}};

    std::vector<std::array<int, 3>> normals = {
        {-1, 0, 0},  // x, back
        {1, 0, 0},   // x, forward
        {0, -1, 0},  // y, back
        {0, 1, 0},   // y, forward
        {0, 0, -1},  // z, back
        {0, 0, 1}    // z, forward
    };

    int object_padded[7][7][7] = {{{0}}};
    std::string vertices_matrix[7][7][7];
    std::vector<std::string> vertices;
    std::vector<std::string> faces;

    // padding to avoid "out of range"
    for (int x = 1; x < 6; ++x) {
        for (int y = 1; y < 6; ++y) {
            for (int z = 1; z < 6; ++z) {
                object_padded[x][y][z] = object[x - 1][y - 1][z - 1];
            }
        }
    }

    // finding outside
    for (int x = 1; x < 6; ++x) {
        for (int y = 1; y < 6; ++y) {
            for (int z = 1; z < 6; ++z) {
                if (object_padded[x][y][z] == 0) continue;  // outside of object

                // check if neighbouring cells are in- or outside
                for (int direction = 0; direction < 6; ++direction) {
                    int dim_prime = direction / 2;  // check in direction 1=x, 2=y or 3=z
                    int dim_left = (dim_prime - 1 + 3) % 3;
                    int dim_right = (dim_prime + 1) % 3;
                    int forward = direction % 2;  // go +1 or -1 in primary dimension

                    std::array<int, 3> d = {0, 0, 0}; // offset to [x, y, z] to find neighbouring cells
                    d[dim_prime] = forward * 2 - 1;  // map from {0,1} to {-1, 1}

                    // neighbouring cell is inside the object -> don't add a wall
                    if (object_padded[x + d[0]][y + d[1]][z + d[2]] == 1) continue;

                    // add vertices if neccessary
                    d[dim_prime] = forward;  // redo mapping from {0,1} to {-1, 1}
                    auto d_zero = d;
                    if (vertices_matrix[x + d[0]][y + d[1]][z + d[2]] == "") {
                        vertices.push_back("v " + to_string(x + d[0]) + " " +
                                           to_string(y + d[1]) + " " + to_string(z + d[2]));
                        vertices_matrix[x + d[0]][y + d[1]][z + d[2]] =
                            to_string(vertices.size());
                    }

                    d[dim_left] = 1;
                    auto d_left = d;
                    if (vertices_matrix[x + d[0]][y + d[1]][z + d[2]] == "") {
                        vertices.push_back("v " + to_string(x + d[0]) + " " +
                                           to_string(y + d[1]) + " " + to_string(z + d[2]));
                        vertices_matrix[x + d[0]][y + d[1]][z + d[2]] =
                            to_string(vertices.size());
                    }

                    d[dim_right] = 1;
                    auto d_both = d;
                    if (vertices_matrix[x + d[0]][y + d[1]][z + d[2]] == "") {
                        vertices.push_back("v " + to_string(x + d[0]) + " " +
                                           to_string(y + d[1]) + " " + to_string(z + d[2]));
                        vertices_matrix[x + d[0]][y + d[1]][z + d[2]] =
                            to_string(vertices.size());
                    }

                    d[dim_left] = 0;
                    auto d_right = d;
                    if (vertices_matrix[x + d[0]][y + d[1]][z + d[2]] == "") {
                        vertices.push_back("v " + to_string(x + d[0]) + " " +
                                           to_string(y + d[1]) + " " + to_string(z + d[2]));
                        vertices_matrix[x + d[0]][y + d[1]][z + d[2]] =
                            to_string(vertices.size());
                    }

                    faces.push_back(
                        "f " +
                        vertices_matrix[x + d_zero[0]][y + d_zero[1]][z + d_zero[2]] +
                        "//" + to_string(direction + 1) + " " +
                        vertices_matrix[x + d_left[0]][y + d_left[1]][z + d_left[2]] +
                        "//" + to_string(direction + 1) + " " +
                        vertices_matrix[x + d_both[0]][y + d_both[1]][z + d_both[2]] +
                        "//" + to_string(direction + 1));

                    faces.push_back(
                        "f " +
                        vertices_matrix[x + d_zero[0]][y + d_zero[1]][z + d_zero[2]] +
                        "//" + to_string(direction + 1) + " " +
                        vertices_matrix[x + d_both[0]][y + d_both[1]][z + d_both[2]] +
                        "//" + to_string(direction + 1) + " " +
                        vertices_matrix[x + d_right[0]][y + d_right[1]][z + d_right[2]] +
                        "//" + to_string(direction + 1));
                }
            }
        }
    }

    std::ofstream o;
    o.open("cube_cross.obj");

    // print vertices
    for (const auto& s : vertices) {
        o << s << "\n";
    }
    // print normals
    for (const auto& s : normals) {
        o << "vn " << std::to_string(s[0]) << " " << std::to_string(s[1]) << " "
          << std::to_string(s[2]) << "\n";
    }
    // print faces
    for (const auto& s : faces) {
        o << s << "\n";
    }

    o.close();
    return 0;
}
