#ifndef PROJECT2D_BRICK_H
#define PROJECT2D_BRICK_H

#include <cstdint>
#include <utility>

#include "brick_type.h"

using Position = std::pair<std::int16_t, std::int16_t>;
using Size = std::pair<std::int16_t, std::int16_t>;

class Brick {
public:
    Brick(Size size, Position position, BrickType type);
    ~Brick() = default;

    const std::int16_t BRICK_WIDTH;
    const std::int16_t BRICK_HEIGHT;

    Position position;
    BrickType type;
};


#endif //PROJECT2D_BRICK_H
