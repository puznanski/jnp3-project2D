#include "ball.h"

Ball::Ball(std::int16_t radius, Speed initial_speed, Position position) : BALL_RADIUS(radius), speed(initial_speed),
                                                                          previous_position(position),
                                                                          position(position) {}

void Ball::move() {
    previous_position = position;
    position.first += speed.first;
    position.second += speed.second;
}
