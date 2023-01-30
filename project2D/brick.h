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

    std::int16_t width;
    std::int16_t height;
    Position position;
    BrickType type;

    [[nodiscard]] bool check_collision(Position ball_position, std::int16_t ball_radius) const;
};


#endif //PROJECT2D_BRICK_H
