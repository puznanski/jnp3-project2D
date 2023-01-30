#include "game.h"

#include <algorithm>

Game::Game() :
    balls({{
        BALL_RADIUS,
        {0, -BALL_INITIAL_SPEED},
        {GAME_CANVAS_SIZE / 2, GAME_CANVAS_SIZE - (PADDLE_GAP_BOTTOM + PADDLE_HEIGHT + BALL_GAP_BOTTOM)}}}
    ), paddle({
        {PADDLE_WIDTH, PADDLE_HEIGHT},
        {(GAME_CANVAS_SIZE - PADDLE_WIDTH) / 2, GAME_CANVAS_SIZE - PADDLE_GAP_BOTTOM}}
    ),
    engine(random_device()),
    speed_dist(static_cast<std::int16_t>(BALL_INITIAL_SPEED * 0.2), BALL_INITIAL_SPEED),
    dist(0, 1) {
    std::int16_t y = BOARD_GAP_TOP;

    while (y < GAME_CANVAS_SIZE - BOARD_GAP_BOTTOM) {
        std::int16_t x = BOARD_GAP_SIDE;

        while (x < GAME_CANVAS_SIZE - BOARD_GAP_SIDE) {
            auto random = dist(engine);
            BrickType brick_type;

            if (random < UPGRADE_BRICK_PROBABILITY) {
                brick_type = BrickType::Upgrade;
            }
            else if (random < UPGRADE_BRICK_PROBABILITY + AWARD_BRICK_PROBABILITY) {
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
        paddle.move(window_coordinates_to_game_coordinates(window_size, board_start, board_size, board_scale, mouse), GAME_CANVAS_SIZE);

        for (auto &drop : drops) {
            drop.move();
        }

        auto balls_iterator = balls.begin();

        while (balls_iterator != balls.end()) {
            balls_iterator->move();

            auto bricks_iterator = bricks.begin();
            bool collided = false;

            while (bricks_iterator != bricks.end()) {
                if (bricks_iterator->check_collision(balls_iterator->position, balls_iterator->radius)) {
                    collided = true;

                    if (bricks_iterator->type == BrickType::Upgrade || bricks_iterator->type == BrickType::Award) {
                        drops.push_back({
                                {DROP_SIZE, DROP_SIZE},
                                {bricks_iterator->position.first + bricks_iterator->width / 2, bricks_iterator->position.second + bricks_iterator->height},
                                bricks_iterator->type == BrickType::Upgrade ? DropType::Arrows : DropType::Star,
                                {0, DROP_SPEED}}
                        );
                    }

                    bricks_iterator = bricks.erase(bricks_iterator);
                } else {
                    bricks_iterator++;
                }
            }

            collided &= (!check_ball_sides_collision(*balls_iterator));
            collided &= (!check_ball_paddle_collision(*balls_iterator));

            if (balls_iterator->position.second + BALL_RADIUS > GAME_CANVAS_SIZE) {
                balls_iterator = balls.erase(balls_iterator);
            }
            else if (collided) {
                balls_iterator->reflect_ball();
            }
            else {
                balls_iterator++;
            }
        }

        if (balls.empty()) {
            lives -= 1;
            is_game_started = false;
            drops.erase(drops.begin(), drops.end());
            paddle.position = {(GAME_CANVAS_SIZE - PADDLE_WIDTH) / 2, GAME_CANVAS_SIZE - PADDLE_GAP_BOTTOM};
            balls.push_back({
                BALL_RADIUS,
                {0, -BALL_INITIAL_SPEED},
                {GAME_CANVAS_SIZE / 2, GAME_CANVAS_SIZE - (PADDLE_GAP_BOTTOM + PADDLE_HEIGHT + BALL_GAP_BOTTOM)}});
        }
        else {
            auto drops_iterator = drops.begin();

            while (drops_iterator != drops.end()) {
                if (check_drop_collection(*drops_iterator) || drops_iterator->position.second + DROP_SIZE > GAME_CANVAS_SIZE) {
                    drops_iterator = drops.erase(drops_iterator);
                }
                else {
                    drops_iterator++;
                }
            }
        }

        won = (lives > 0 && bricks.empty());
    }

    return (bricks.empty() || lives == 0);
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

std::vector<Ball> Game::get_balls_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale) {
    std::vector<Ball> result;
    auto calculated_radius = game_size_to_window_size(window_size, board_scale, {BALL_RADIUS, BALL_RADIUS});

    for (const auto &ball : balls) {
        auto calculated_position = game_coordinates_to_window_coordinates(window_size, board_start, board_scale, ball.position);
        result.emplace_back(calculated_radius.first, ball.speed, calculated_position);
    }

    return result;
}

std::vector<Drop> Game::get_drops_to_draw(D2DSize window_size, D2DSize board_start, D2DSize board_scale) {
    std::vector<Drop> result;
    auto calculated_size = game_size_to_window_size(window_size, board_scale, {DROP_SIZE, DROP_SIZE});

    for (const auto &drop : drops) {
        auto calculated_position = game_coordinates_to_window_coordinates(window_size, board_start, board_scale, drop.position);
        result.emplace_back(calculated_size, calculated_position, drop.type, drop.speed);
    }

    return result;
}

std::int16_t Game::get_lives() const {
    return lives;
}

std::int16_t Game::get_max_lives() const {
    return INITIAL_LIVES;
}

std::int32_t Game::get_points() const {
    return points;
}

bool Game::get_won() const {
    return won;
}

bool Game::check_ball_sides_collision(Ball &ball) const {
    if (ball.position.first - ball.radius < 0 || ball.position.first + ball.radius > GAME_CANVAS_SIZE) {
        ball.reflect_ball_x();
        return true;
    }
    else if (ball.position.second - ball.radius < 0) {
        ball.reflect_ball_y();
        return true;
    }

    return false;
}

bool Game::check_ball_paddle_collision(Ball &ball) const {
    if ((paddle.position.first < ball.position.first + ball.radius) &&
        (paddle.position.first + paddle.WIDTH > ball.position.first - ball.radius) &&
        (paddle.position.second < ball.position.second + ball.radius) &&
        (paddle.position.second + paddle.HEIGHT > ball.position.second - ball.radius)) {

        auto paddle_center = static_cast<float>(paddle.position.first) + static_cast<float>(paddle.WIDTH) / 2.0f;
        auto ball_distance_from_paddle_center = static_cast<float>(ball.position.first) - paddle_center;
        ball.speed.first = static_cast<std::int16_t>(ball_distance_from_paddle_center * HORIZONTAL_BALL_SPEED_MULTIPLIER);
        ball.speed.second *= -1;

        return true;
    }

    return false;
}

bool Game::check_drop_collection(Drop &drop) {
    if ((paddle.position.first < drop.position.first + drop.width / 2) &&
        (paddle.position.first + paddle.WIDTH > drop.position.first - drop.width / 2) &&
        (paddle.position.second < drop.position.second + drop.height / 2) &&
        (paddle.position.second + paddle.HEIGHT > drop.position.second - drop.height / 2)) {

        if (drop.type == DropType::Arrows && balls.size() < BALL_LIMIT) {
            std::vector<Ball> new_balls;

            for (auto ball : balls) {
                for (std::int16_t i = 1; i < DROP_BALL_MULTIPLIER; i++) {
                    new_balls.push_back({
                        BALL_RADIUS,
                        {speed_dist(engine) * (dist(engine) >= 0.5 ? 1 : -1), -speed_dist(engine)},
                        {ball.position.first, ball.position.second}});
                }
            }

            balls.reserve(balls.size() + new_balls.size());
            balls.insert(balls.end(), new_balls.begin(), new_balls.end());
        }
        else if (drop.type == DropType::Star) {
            points += POINTS_MULTIPLIER;
        }

        return true;
    }

    return false;
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
