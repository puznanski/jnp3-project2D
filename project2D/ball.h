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

    std::int16_t radius;
    Position position;
    Speed speed;

    void move();

    void reflect_ball_x();

    void reflect_ball_y();

    void reflect_ball();
};


#endif //PROJECT2D_BALL_H
