#ifndef PROJECT2D_DROP_H
#define PROJECT2D_DROP_H

#include <cstdint>
#include <utility>
#include "drop_type.h"

using Position = std::pair<std::int16_t, std::int16_t>;
using Size = std::pair<std::int16_t, std::int16_t>;
using Speed = std::pair<std::int16_t, std::int16_t>;

class Drop {
public:
    Drop(Size size, Position position, DropType type, Speed speed);
    ~Drop() = default;

    std::int16_t width;
    std::int16_t height;
    Position position;
    DropType type;
    Speed speed;

    void move();
};


#endif //PROJECT2D_DROP_H
