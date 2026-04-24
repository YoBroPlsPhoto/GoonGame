#ifndef CITY_MAP_HPP
#define CITY_MAP_HPP

#include "map.hpp"

class CityMap : public Map {
public:
    CityMap();
    ~CityMap() override;
    
    void Draw(int detailLevel = 1, Vector3 viewPos = {0,0,0}) override;
    float GetHeight(float x, float z) override;
    const std::vector<BoundingBox>& GetObstacles() const override { return obstacles; }

private:
    struct Building {
        Vector3 pos;
        Vector3 size;
        Color color;
    };
    std::vector<Building> buildings;
    std::vector<BoundingBox> obstacles;
    
    Vector3 mainBasePos = { 0.0f, 0, 150.0f };
    Model trashBinModel;
    bool modelLoaded = false;
    void DrawMainBase();
    void DrawText3D(const char* text, Vector3 pos, float size, float spacing, Color col);
};

#endif
