#ifndef PROJECT2D_GAME_H
#define PROJECT2D_GAME_H

#include <cstdint>
#include <vector>
#include <utility>

#include "brick.h"
#include "paddle.h"
#include "ball.h"

using Position = std::pair<std::int16_t, std::int16_t>;
using D2DSize = std::pair<float, float>;

class Game {
public:
    Game();

    ~Game() = default;

    bool game_step(
            D2DSize mouse,
            bool mouse_pressed,
            D2DSize window_size,
            D2DSize board_start,
            D2DSize board_size,
            D2DSize board_scale
    );

    std::vector<Brick> get_bricks_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale);

    Paddle get_paddle_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale);

    Ball get_ball_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale);

private:
    const std::int16_t GAME_CANVAS_SIZE = 1000;
    const std::int16_t BALL_RADIUS = 8;
    const std::int16_t BALL_INITIAL_SPEED = 10;
    const std::int16_t BALL_GAP_BOTTOM = 50;
    const std::int16_t PADDLE_WIDTH = 100;
    const std::int16_t PADDLE_HEIGHT = 15;
    const std::int16_t PADDLE_GAP_BOTTOM = 50;
    const std::int16_t BRICK_WIDTH = 40;
    const std::int16_t BRICK_HEIGHT = 20;
    const std::int16_t BRICK_GAP = 4;
    const std::int16_t BOARD_GAP_SIDE = 18;
    const std::int16_t BOARD_GAP_TOP = 100;
    const std::int16_t BOARD_GAP_BOTTOM = 424;
    const float UNBREAKABLE_BRICK_PROBABILITY = 0.02;
    const float AWARD_BRICK_PROBABILITY = 0.06;

    std::vector<Brick> bricks;
    Paddle paddle;
    Ball ball;
    bool is_game_started = false;

    [[nodiscard]] Position window_coordinates_to_game_coordinates(D2DSize window_size, D2DSize board_start, D2DSize board_size, D2DSize board_scale, D2DSize position) const;

    [[nodiscard]] Position game_size_to_window_size(D2DSize window_size, D2DSize board_scale, Position position) const;

    [[nodiscard]] Position game_coordinates_to_window_coordinates(D2DSize window_size, D2DSize board_start, D2DSize board_scale, Position position) const;
};


#endif //PROJECT2D_GAME_H
