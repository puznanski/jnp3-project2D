#include "game.h"

#include <random>
#include <algorithm>

Game::Game() :
    ball({
        BALL_RADIUS,
        {0, -BALL_INITIAL_SPEED},
        {GAME_CANVAS_SIZE / 2, GAME_CANVAS_SIZE - (PADDLE_GAP_BOTTOM + PADDLE_HEIGHT + BALL_GAP_BOTTOM)}}
    ), paddle({
        {PADDLE_WIDTH, PADDLE_HEIGHT},
        {(GAME_CANVAS_SIZE - PADDLE_WIDTH) / 2, GAME_CANVAS_SIZE - PADDLE_GAP_BOTTOM}}
    ) {
    std::random_device random_device;
    std::mt19937 engine(random_device());
    std::uniform_real_distribution<> dist(0, 1);

    std::int16_t y = BOARD_GAP_TOP;

    while (y < GAME_CANVAS_SIZE - BOARD_GAP_BOTTOM) {
        std::int16_t x = BOARD_GAP_SIDE;

        while (x < GAME_CANVAS_SIZE - BOARD_GAP_SIDE) {
            auto random = dist(engine);
            BrickType brick_type;

            if (random < UNBREAKABLE_BRICK_PROBABILITY) {
                brick_type = BrickType::Unbreakable;
            }
            else if (random < UNBREAKABLE_BRICK_PROBABILITY + AWARD_BRICK_PROBABILITY) {
                brick_type = BrickType::Award;
            }
            else {
                brick_type = BrickType::Normal;
            }

            bricks.push_back({{BRICK_WIDTH, BRICK_HEIGHT}, {x, y}, brick_type});
            x += BRICK_WIDTH + BRICK_GAP;
        }

        y += BRICK_HEIGHT + BRICK_GAP;
    }
}

bool Game::game_step(D2DSize mouse, bool mouse_pressed, D2DSize window_size, D2DSize board_start, D2DSize board_size, D2DSize board_scale) {
    if (!is_game_started) {
        is_game_started = mouse_pressed;
    }
    else {
        ball.move();
        paddle.move(window_coordinates_to_game_coordinates(window_size, board_start, board_size, board_scale, mouse));
    }

    return false;
}

std::vector<Brick> Game::get_bricks_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale) {
    std::vector<Brick> result;
    auto calculated_size = game_size_to_window_size(window_size, board_scale, {BRICK_WIDTH, BRICK_HEIGHT});

    for (const auto &brick : bricks) {
        auto calculated_position = game_coordinates_to_window_coordinates(window_size, board_start, board_scale, brick.position);
        result.emplace_back(calculated_size, calculated_position, brick.type);
    }

    return result;
}

Paddle Game::get_paddle_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale) {
    auto calculated_size = game_size_to_window_size(window_size, board_scale, {PADDLE_WIDTH, PADDLE_HEIGHT});
    auto calculated_position = game_coordinates_to_window_coordinates(window_size, board_start, board_scale, paddle.position);
    return {calculated_size, calculated_position};
}

Ball Game::get_ball_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale) {
    auto calculated_radius = game_size_to_window_size(window_size, board_scale, {BALL_RADIUS, BALL_RADIUS});
    auto calculated_position = game_coordinates_to_window_coordinates(window_size, board_start, board_scale, ball.position);
    return {calculated_radius.first, ball.speed, calculated_position};
}

Position Game::window_coordinates_to_game_coordinates(D2DSize window_size, D2DSize board_start, D2DSize board_size, D2DSize board_scale, D2DSize position) const {
    D2DSize temp;

    temp.first = std::max(position.first, board_start.first);
    temp.first = std::min(temp.first, board_start.first + board_size.first);
    temp.first -= board_start.first;
    temp.first /= board_scale.first;

    temp.second = std::max(position.second, board_start.second);
    temp.second = std::min(temp.second, board_start.second + board_size.second);
    temp.second -= board_start.first;
    temp.second /= board_scale.second;

    return {static_cast<std::int16_t>((temp.first * static_cast<float>(GAME_CANVAS_SIZE)) / window_size.first),
            static_cast<std::int16_t>((temp.second * static_cast<float>(GAME_CANVAS_SIZE)) / window_size.second)};
}

Position Game::game_size_to_window_size(D2DSize window_size, D2DSize board_scale, Position position) const {
    return {static_cast<std::int16_t>(((static_cast<float>(position.first) * window_size.first) / static_cast<float>(GAME_CANVAS_SIZE) * board_scale.first)),
            static_cast<std::int16_t>(((static_cast<float>(position.second) * window_size.second) / static_cast<float>(GAME_CANVAS_SIZE) * board_scale.second))};
}

Position Game::game_coordinates_to_window_coordinates(D2DSize window_size, D2DSize board_start, D2DSize board_scale, Position position) const {
    auto size_converted = game_size_to_window_size(window_size, board_scale, position);

    return {size_converted.first + static_cast<std::int16_t>(board_start.first),
            size_converted.second + static_cast<std::int16_t>(board_start.second)};
}
