//
// Snake Game for Picoware
// Modern Snake implementation with levels and power-ups
//

#pragma once
#include "../../../system/colors.hpp"
#include "../../../system/view.hpp"
#include "../../../system/view_manager.hpp"
#include "../../../applications/games/snake/game.hpp"

static GameEngine *snakeEngine = nullptr;

static bool snakeStart(ViewManager *viewManager)
{
    // Create the game instance with its name, start/stop callbacks, and colors.
    Game *game = new Game(
        "Snake",                        // Game name
        Vector(320, 240),               // Game size
        viewManager->getDraw(),         // Draw object
        viewManager->getInputManager(), // Input manager
        TFT_GREEN,                      // Foreground color
        TFT_BLACK,                      // Background color
        CAMERA_FIRST_PERSON,            // Camera perspective
        nullptr,                        // Game start callback
        Snake::game_stop                // Game stop callback
    );

    // Create and add a level to the game.
    Level *level = new Level("Snake Level", Vector(320, 240), game);
    game->level_add(level);

    // Add the player entity (snake) to the level
    Snake::player_spawn(level, game);

    // Create the game engine (with 15 frames per second for classic snake feel).
    snakeEngine = new GameEngine(game, 15);

    return true;
}

static void snakeRun(ViewManager *viewManager)
{
    // Run the game engine
    if (snakeEngine != nullptr)
    {
        snakeEngine->runAsync(false);
    }
    auto input = viewManager->getInputManager()->getLastButton();
    if (input == BUTTON_BACK)
    {
        viewManager->back();
        viewManager->getInputManager()->reset(true);
        return;
    }
}

static void snakeStop(ViewManager *viewManager)
{
    // Stop the game engine
    if (snakeEngine != nullptr)
    {
        snakeEngine->stop();
        delete snakeEngine;
        snakeEngine = nullptr;
    }
}

static const View snakeView = View("Snake", snakeRun, snakeStart, snakeStop);
