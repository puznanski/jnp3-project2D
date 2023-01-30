#include "paddle.h"


Paddle::Paddle(Size size, Position position) : WIDTH(size.first), HEIGHT(size.second), position(position) {}

void Paddle::move(Position mouse_position) {
    position.first = mouse_position.first - WIDTH / 2;
}
