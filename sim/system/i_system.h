#pragma once

struct WorldState;

class ISystem
{
public:
    virtual ~ISystem() = default;
    virtual void Update(WorldState& world) = 0;
};
