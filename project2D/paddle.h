#ifndef PROJECT2D_PADDLE_H
#define PROJECT2D_PADDLE_H

#include <cstdint>
#include <utility>

using Position = std::pair<std::int16_t, std::int16_t>;
using Size = std::pair<std::int16_t, std::int16_t>;

class Paddle {
public:
    Paddle(Size size, Position position);
    ~Paddle() = default;

    const std::int16_t WIDTH;
    const std::int16_t HEIGHT;

    Position position;

    void move(Position mouse_position);
};


#endif //PROJECT2D_PADDLE_H
