#include "paddle.h"

#include <algorithm>

Paddle::Paddle(Size size, Position position) : WIDTH(size.first), HEIGHT(size.second), position(position) {}

void Paddle::move(Position mouse_position, std::int16_t board_size) {
    position.first = mouse_position.first - WIDTH / 2;
    position.first = std::max(position.first, static_cast<std::int16_t>(0));
    position.first = std::min(position.first, static_cast<std::int16_t>(board_size - WIDTH));

}
