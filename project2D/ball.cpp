#include "ball.h"

#include <cmath>

Ball::Ball(std::int16_t radius, Speed initial_speed, Position position) : radius(radius), speed(initial_speed),
                                                                          position(position) {}

void Ball::move() {
    position.first += speed.first;
    position.second += speed.second;
}

void Ball::reflect_ball_x() {
    speed.first *= -1;
}

void Ball::reflect_ball_y() {
    speed.second *= -1;
}

void Ball::reflect_ball() {
    if (speed.first == 0) {
        reflect_ball_y();
    }
    else if (speed.second == 0) {
        reflect_ball_x();
    }
    else {
        auto atan_val = atan(static_cast<double>(speed.first) / static_cast<double>(speed.second));
        auto angle = static_cast<std::int16_t>(round(atan_val * 180.0 / 3.1415));

        if (angle == -45 || angle == 45) {
            reflect_ball_x();
            reflect_ball_y();
        }
        else if (angle < -45 || angle > 45) {
            reflect_ball_x();
        }
        else {
            reflect_ball_y();
        }
    }
}
