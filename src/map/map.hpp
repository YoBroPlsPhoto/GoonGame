#ifndef MAP_HPP
#define MAP_HPP

#include "raylib.h"
#include <vector>

enum class MapType { CITY, DESERT };

class Map {
public:
    MapType type = MapType::CITY;
    int worldDetail = 1; // 0 = low, 1 = high
    virtual ~Map() {}
    virtual void Draw(int detailLevel = 1, Vector3 viewPos = {0,0,0}) = 0;
    virtual float GetHeight(float x, float z) = 0;
    virtual const std::vector<BoundingBox>& GetObstacles() const = 0;
};

#endif
