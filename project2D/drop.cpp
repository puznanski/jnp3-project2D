#include "drop.h"

Drop::Drop(Size size, Position position, DropType type, Speed speed) : width(size.first), height(size.second),
                                                                       position(position), type(type), speed(speed) {}

void Drop::move() {
    position.first += speed.first;
    position.second += speed.second;
}
