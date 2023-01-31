#ifndef PROJECT2D_GAME_H
#define PROJECT2D_GAME_H

#include <cstdint>
#include <vector>
#include <utility>
#include <random>

#include "brick.h"
#include "paddle.h"
#include "ball.h"
#include "drop.h"

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

    std::vector<Ball> get_balls_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale);

    std::vector<Drop> get_drops_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale);

    [[nodiscard]] std::int16_t get_lives() const;

    [[nodiscard]] std::int16_t get_max_lives() const;

    [[nodiscard]] std::int32_t get_points() const;

    [[nodiscard]] bool get_won() const;

private:
    const std::int16_t INITIAL_LIVES = 3;
    const std::int16_t GAME_CANVAS_SIZE = 1000;
    const std::int16_t BALL_RADIUS = 9;
    const std::int16_t BALL_INITIAL_SPEED = 10;
    const std::int16_t BALL_GAP_BOTTOM = 50;
    const std::int16_t PADDLE_WIDTH = 120;
    const std::int16_t PADDLE_HEIGHT = 15;
    const std::int16_t PADDLE_GAP_BOTTOM = 50;
    const std::int16_t BRICK_WIDTH = 40;
    const std::int16_t BRICK_HEIGHT = 20;
    const std::int16_t BRICK_GAP = 4;
    const std::int16_t BOARD_GAP_SIDE = 18;
    const std::int16_t BOARD_GAP_TOP = 100;
    const std::int16_t BOARD_GAP_BOTTOM = 424;
    const std::int16_t DROP_SIZE = 30;
    const std::int16_t DROP_SPEED = 5;
    const std::int16_t DROP_BALL_MULTIPLIER = 3;
    const std::int16_t BALL_LIMIT = 100;
    const std::int16_t POINTS_MULTIPLIER = 100;
    const float UPGRADE_BRICK_PROBABILITY = 0.06;
    const float AWARD_BRICK_PROBABILITY = 0.12;
    const float HORIZONTAL_BALL_SPEED_MULTIPLIER = 0.2;

    std::random_device random_device;
    std::mt19937 engine;
    std::uniform_int_distribution<> speed_dist;
    std::uniform_real_distribution<> dist;

    std::vector<Brick> bricks;
    Paddle paddle;
    std::vector<Ball> balls;
    std::vector<Drop> drops;
    bool is_game_started = false;
    bool won = false;
    std::int16_t lives = INITIAL_LIVES;
    std::int32_t points = 0;

    bool check_ball_sides_collision(Ball &ball) const;

    bool check_ball_paddle_collision(Ball &ball) const;

    bool check_drop_collection(Drop &drop);

    [[nodiscard]] Position window_coordinates_to_game_coordinates(D2DSize window_size, D2DSize board_start, D2DSize board_size, D2DSize board_scale, D2DSize position) const;

    [[nodiscard]] Position game_size_to_window_size(D2DSize window_size, D2DSize board_scale, Position position) const;

    [[nodiscard]] Position game_coordinates_to_window_coordinates(D2DSize window_size, D2DSize board_start, D2DSize board_scale, Position position) const;
};


#endif //PROJECT2D_GAME_H
