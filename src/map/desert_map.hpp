#ifndef DESERT_MAP_HPP
#define DESERT_MAP_HPP

#include "map.hpp"

class DesertMap : public Map {
public:
    DesertMap();
    ~DesertMap() override;
    
    void Draw(int detailLevel = 1, Vector3 viewPos = {0,0,0}, int currentWave = 1) override;
    float GetHeight(float x, float z) override;
    const std::vector<BoundingBox>& GetObstacles() const override { return obstacles; }

private:
    static const int terrainSize = 100;
    float terrainScale = 4.0f;
    float terrainHeights[101][101];
    std::vector<BoundingBox> obstacles;
};

#endif
