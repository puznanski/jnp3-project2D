#include "brick.h"

Brick::Brick(Size size, Position position, BrickType type) : width(size.first), height(size.second), position(position),
                                                             type(type) {}

bool Brick::check_collision(Position ball_position, std::int16_t ball_radius) const {
    return ((position.first < ball_position.first + ball_radius) &&
            (position.first + width > ball_position.first - ball_radius) &&
            (position.second < ball_position.second + ball_radius) &&
            (position.second + height > ball_position.second - ball_radius));
}
