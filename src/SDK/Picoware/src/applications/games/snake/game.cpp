#include "../../../applications/games/snake/game.hpp"
#include "../../../system/buttons.hpp"
#include "pico/stdlib.h"
#include <cstdio>
#include <cstring>

namespace Snake
{
    // Game constants
    #define SCREEN_SIZE_X 320
    #define SCREEN_SIZE_Y 240
    #define SNAKE_SEGMENT_SIZE 8
    #define MAX_SNAKE_LENGTH 100
    #define FOOD_SIZE 8
    #define INITIAL_SPEED 200      // milliseconds between moves
    #define SPEED_INCREASE 10      // speed increase per food eaten
    #define MIN_SPEED 50           // minimum speed (fastest)

    enum Direction {
        DIR_UP = 0,
        DIR_DOWN = 1,
        DIR_LEFT = 2,
        DIR_RIGHT = 3
    };

    typedef struct {
        Vector position;
        bool active;
    } SnakeSegment;

    typedef struct {
        Vector position;
        bool active;
        uint16_t color;
    } Food;

    typedef struct {
        int score;
        int lives;
        bool gameOver;
        bool gameWon;
        Direction direction;
        Direction nextDirection;
        SnakeSegment segments[MAX_SNAKE_LENGTH];
        int length;
        Food food;
        uint32_t lastMove;
        uint32_t moveSpeed;
        Entity *player;
        int level;
        int foodEaten;
    } GameState;

    GameState *gameState = nullptr;

    void game_stop()
    {
        if (gameState != nullptr)
        {
            delete gameState;
            gameState = nullptr;
        }
    }

    void spawn_food()
    {
        if (!gameState) return;
        
        // Generate random position for food
        bool validPosition = false;
        Vector newPos;
        int attempts = 0;
        
        while (!validPosition && attempts < 100) {
            newPos.x = (rand() % (SCREEN_SIZE_X / SNAKE_SEGMENT_SIZE)) * SNAKE_SEGMENT_SIZE;
            newPos.y = (rand() % (SCREEN_SIZE_Y / SNAKE_SEGMENT_SIZE)) * SNAKE_SEGMENT_SIZE;
            
            // Check if position collides with snake
            validPosition = true;
            for (int i = 0; i < gameState->length; i++) {
                if (gameState->segments[i].position.x == newPos.x && 
                    gameState->segments[i].position.y == newPos.y) {
                    validPosition = false;
                    break;
                }
            }
            attempts++;
        }
        
        gameState->food.position = newPos;
        gameState->food.active = true;
        gameState->food.color = TFT_RED;
    }

    void init_snake()
    {
        if (!gameState) return;
        
        // Initialize snake in center of screen
        gameState->length = 3;
        gameState->direction = DIR_RIGHT;
        gameState->nextDirection = DIR_RIGHT;
        
        int startX = (SCREEN_SIZE_X / 2) - (SNAKE_SEGMENT_SIZE * 2);
        int startY = SCREEN_SIZE_Y / 2;
        
        for (int i = 0; i < gameState->length; i++) {
            gameState->segments[i].position.x = startX - (i * SNAKE_SEGMENT_SIZE);
            gameState->segments[i].position.y = startY;
            gameState->segments[i].active = true;
        }
        
        // Clear remaining segments
        for (int i = gameState->length; i < MAX_SNAKE_LENGTH; i++) {
            gameState->segments[i].active = false;
        }
    }

    void reset_game()
    {
        if (!gameState) return;
        
        gameState->score = 0;
        gameState->lives = 3;
        gameState->gameOver = false;
        gameState->gameWon = false;
        gameState->lastMove = 0;
        gameState->moveSpeed = INITIAL_SPEED;
        gameState->level = 1;
        gameState->foodEaten = 0;
        
        init_snake();
        spawn_food();
    }

    bool check_collision_with_walls(Vector pos)
    {
        return (pos.x < 0 || pos.x >= SCREEN_SIZE_X || 
                pos.y < 0 || pos.y >= SCREEN_SIZE_Y);
    }

    bool check_collision_with_self(Vector pos)
    {
        if (!gameState) return false;
        
        for (int i = 1; i < gameState->length; i++) {
            if (gameState->segments[i].position.x == pos.x && 
                gameState->segments[i].position.y == pos.y) {
                return true;
            }
        }
        return false;
    }

    void move_snake()
    {
        if (!gameState || gameState->gameOver) return;
        
        uint32_t currentTime = to_ms_since_boot(get_absolute_time());
        if (currentTime - gameState->lastMove < gameState->moveSpeed) {
            return;
        }
        
        gameState->lastMove = currentTime;
        gameState->direction = gameState->nextDirection;
        
        // Calculate new head position
        Vector newHead = gameState->segments[0].position;
        
        switch (gameState->direction) {
            case DIR_UP:
                newHead.y -= SNAKE_SEGMENT_SIZE;
                break;
            case DIR_DOWN:
                newHead.y += SNAKE_SEGMENT_SIZE;
                break;
            case DIR_LEFT:
                newHead.x -= SNAKE_SEGMENT_SIZE;
                break;
            case DIR_RIGHT:
                newHead.x += SNAKE_SEGMENT_SIZE;
                break;
        }
        
        // Check collisions
        if (check_collision_with_walls(newHead) || check_collision_with_self(newHead)) {
            gameState->lives--;
            if (gameState->lives <= 0) {
                gameState->gameOver = true;
            } else {
                init_snake();
                spawn_food();
            }
            return;
        }
        
        // Check food collision
        bool ateFood = false;
        if (gameState->food.active && 
            newHead.x == gameState->food.position.x && 
            newHead.y == gameState->food.position.y) {
            ateFood = true;
            gameState->food.active = false;
            gameState->score += 10;
            gameState->foodEaten++;
            
            // Increase speed slightly
            if (gameState->moveSpeed > MIN_SPEED) {
                gameState->moveSpeed -= SPEED_INCREASE;
            }
            
            // Check level completion
            if (gameState->foodEaten >= 10) {
                gameState->level++;
                gameState->foodEaten = 0;
                if (gameState->level >= 5) {
                    gameState->gameWon = true;
                }
            }
        }
        
        // Move body segments
        if (!ateFood) {
            // Normal movement - remove tail
            for (int i = gameState->length - 1; i > 0; i--) {
                gameState->segments[i].position = gameState->segments[i - 1].position;
            }
        } else {
            // Growing - add new segment
            if (gameState->length < MAX_SNAKE_LENGTH) {
                for (int i = gameState->length; i > 0; i--) {
                    gameState->segments[i].position = gameState->segments[i - 1].position;
                    gameState->segments[i].active = true;
                }
                gameState->length++;
                spawn_food();
            }
        }
        
        // Set new head position
        gameState->segments[0].position = newHead;
    }

    // Player update callback
    void player_update(Entity *entity, Game *game)
    {
        if (!gameState || gameState->gameOver) return;
        
        // Handle input - prevent reverse direction
        if (game->input == BUTTON_UP && gameState->direction != DIR_DOWN) {
            gameState->nextDirection = DIR_UP;
        } else if (game->input == BUTTON_DOWN && gameState->direction != DIR_UP) {
            gameState->nextDirection = DIR_DOWN;
        } else if (game->input == BUTTON_LEFT && gameState->direction != DIR_RIGHT) {
            gameState->nextDirection = DIR_LEFT;
        } else if (game->input == BUTTON_RIGHT && gameState->direction != DIR_LEFT) {
            gameState->nextDirection = DIR_RIGHT;
        }
        
        // Reset game on any button press when game over
        if (gameState->gameOver && game->input != -1) {
            reset_game();
        }
        
        move_snake();
    }

    // Player render callback  
    void player_render(Entity *entity, Draw *draw, Game *game)
    {
        if (!gameState) return;
        
        // Clear screen
        draw->clearBuffer(0);
        
        if (gameState->gameOver) {
            // Game over screen
            draw->text(Vector(SCREEN_SIZE_X/2 - 50, SCREEN_SIZE_Y/2 - 20), "GAME OVER", TFT_RED);
            char scoreText[32];
            sprintf(scoreText, "Score: %d", gameState->score);
            draw->text(Vector(SCREEN_SIZE_X/2 - 40, SCREEN_SIZE_Y/2), scoreText, TFT_WHITE);
            draw->text(Vector(SCREEN_SIZE_X/2 - 60, SCREEN_SIZE_Y/2 + 20), "Press any key", TFT_WHITE);
            return;
        }
        
        if (gameState->gameWon) {
            // Victory screen
            draw->text(Vector(SCREEN_SIZE_X/2 - 40, SCREEN_SIZE_Y/2 - 20), "YOU WIN!", TFT_GREEN);
            char scoreText[32];
            sprintf(scoreText, "Score: %d", gameState->score);
            draw->text(Vector(SCREEN_SIZE_X/2 - 40, SCREEN_SIZE_Y/2), scoreText, TFT_WHITE);
            return;
        }
        
        // Draw snake
        for (int i = 0; i < gameState->length; i++) {
            if (gameState->segments[i].active) {
                uint16_t color = (i == 0) ? TFT_GREEN : TFT_DARKGREEN; // Head is brighter
                draw->fillRect(gameState->segments[i].position, 
                             Vector(SNAKE_SEGMENT_SIZE, SNAKE_SEGMENT_SIZE), color);
            }
        }
        
        // Draw food
        if (gameState->food.active) {
            draw->fillRect(gameState->food.position, 
                         Vector(FOOD_SIZE, FOOD_SIZE), gameState->food.color);
        }
        
        // Draw UI
        char scoreText[32];
        sprintf(scoreText, "Score: %d", gameState->score);
        draw->text(Vector(5, 5), scoreText, TFT_WHITE);
        
        char livesText[32];
        sprintf(livesText, "Lives: %d", gameState->lives);
        draw->text(Vector(5, 20), livesText, TFT_WHITE);
        
        char levelText[32];
        sprintf(levelText, "Level: %d", gameState->level);
        draw->text(Vector(5, 35), levelText, TFT_WHITE);
        
        char foodText[32];
        sprintf(foodText, "Food: %d/10", gameState->foodEaten);
        draw->text(Vector(5, 50), foodText, TFT_WHITE);
    }

    void player_spawn(Level *level, Game *game)
    {
        // Initialize game state
        if (gameState == nullptr) {
            gameState = new GameState();
        }
        
        // Create player entity
        Entity *player = new Entity();
        player->position = Vector(SCREEN_SIZE_X / 2, SCREEN_SIZE_Y / 2);
        player->size = Vector(SNAKE_SEGMENT_SIZE, SNAKE_SEGMENT_SIZE);
        player->visible = false; // Snake renders itself
        player->update = player_update;
        player->render = player_render;
        
        gameState->player = player;
        level->entity_add(player);
        
        // Initialize game
        reset_game();
    }
}
