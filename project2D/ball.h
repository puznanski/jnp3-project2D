#ifndef PROJECT2D_BALL_H
#define PROJECT2D_BALL_H

#include <cstdint>
#include <utility>

using Position = std::pair<std::int16_t, std::int16_t>;
using Speed = std::pair<std::int16_t, std::int16_t>;

class Ball {
public:
    Ball(std::int16_t radius, Speed initial_speed, Position position);
    ~Ball() = default;

    std::int16_t BALL_RADIUS;

    Position previous_position;
    Position position;
    Speed speed;

    void move();
};


#endif //PROJECT2D_BALL_H
