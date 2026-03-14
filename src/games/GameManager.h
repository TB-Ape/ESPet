#pragma once
#include <Arduino.h>
#include "../pet/Pet.h"
#include "../display/Renderer.h"
#include "../input/Input.h"
#include "../audio/Audio.h"

// ═══════════════════════════════════════════════════════════════════════════════
// GameManager – Mini-game launcher
// Integrated into the main loop; runs on Core 1 (blocks renderTask)
// ═══════════════════════════════════════════════════════════════════════════════

enum class GameID : uint8_t {
    NONE   = 0,
    CATCH  = 1,
    RUNNER = 2,
};

struct GameResult {
    GameID  game;
    uint16_t score;
    bool    newHighscore;
};

class GameManager {
public:
    bool     active;
    GameID   currentGame;

    GameManager() : active(false), currentGame(GameID::NONE) {}

    // Launch a game (blocks until game is over)
    // Returns score; applies stat effects to pet
    GameResult runGame(GameID id, Pet& pet);

private:
    GameResult runCatchGame(Pet& pet);
    GameResult runRunnerGame(Pet& pet);
};

extern GameManager Games;
