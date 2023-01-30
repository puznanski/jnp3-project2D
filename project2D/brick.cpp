#include "brick.h"

Brick::Brick(Size size, Position position, BrickType type) : BRICK_WIDTH(size.first), BRICK_HEIGHT(size.second),
                                                             position(position), type(type) {}
