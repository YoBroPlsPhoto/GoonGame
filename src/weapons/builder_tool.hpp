#ifndef BUILDER_TOOL_HPP
#define BUILDER_TOOL_HPP

#include "weapon.hpp"

class BuilderTool : public Weapon {
public:
    int buildType = 0; // 0=Wall, 1=Turret
    BuilderTool(int t);
    void Update(float dt, bool isGrounded, Vector2 mouseDelta, bool isSprinting, bool isMoving) override;
    void DrawViewModel(Camera3D c) override;
};

#endif
