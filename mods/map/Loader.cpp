#include "Map.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace Map;

void Level::loadLevel(std::string_view path2level) {
    std::string path(path2level.data(), path2level.size());

    std::ifstream file(path.c_str());

    if (!file) {
        throw std::runtime_error("Map loader: failed to open level file");
    }

    /* read a header */

    std::string mapName;
    std::getline(file, mapName);

    size_t width  {};
    size_t height {};

    if (!(file >> width >> height)) {
        throw std::runtime_error("Map loader: expected '<width> <height>'");
    }

    if (width == 0 || height == 0) {
        throw std::runtime_error("Map loader: map size must be non-zero");
    }

    /* read declarations */

    std::string word;

    if (!(file >> word) || word != "DECLARE") {
        throw std::runtime_error("Map loader: expected DECLARE section");
    }

    std::unordered_map<size_t, Tile::Type> declaredTypes;

    while (file >> word) {
        if (word == "END") {
            break;
        }

        size_t code = std::stoull(word);

        std::string typeName;

        if (!(file >> typeName)) {
            throw std::runtime_error("Map loader: invalid DECLARE entry");
        }

        declaredTypes.emplace(code, Tile::Type(typeName.c_str()));
    }

    /* read the map matrix */

    std::vector<std::vector<size_t>> matrix(height, std::vector<size_t>(width));

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            size_t code {};

            if (!(file >> code)) {
                throw std::runtime_error("Map loader: not enough tile ids in matrix");
            }

            if (declaredTypes.find(code) == declaredTypes.end()) {
                throw std::runtime_error("Map loader: matrix uses undeclared tile id");
            }

            matrix[y][x] = code;
        }
    }

    /* remove existing entities */

    std::vector<Entity::ID> entitiesToRemove;
    entitiesToRemove.reserve(m_entityList.size());

    for (const auto &[id, entity] : m_entityList) {
        entitiesToRemove.push_back(id);
    }

    for (Entity::ID id : entitiesToRemove) {
        removeEntity(id);
    }

    m_tileMap  .clear();
    m_tileTypes.clear();

    /* emplace tiles */

    m_tileMap.reserve(width);

    for (size_t x = 0; x < width; ++x) {
        m_tileMap.emplace_back();
        m_tileMap.back().reserve(height);

        for (size_t y = 0; y < height; ++y) {
            Tile::Type type = declaredTypes.at(matrix[y][x]);

            m_tileMap.back().emplace_back(*this, Vec2D<>(x, y), type);

            ++m_tileTypes[type];

            if (m_tileTypes[type] == 1) {
                EvTileTypeNew.emit(type);
            }
        }
    }

    EvLevelLoaded.emit();
}
