#include "GameManager.h"
#include "CatchGame.h"
#include "RunnerGame.h"

GameManager Games;

static CatchGame  catchGame;
static RunnerGame runnerGame;

GameResult GameManager::runGame(GameID id, Pet& pet) {
    active      = true;
    currentGame = id;

    GameResult result = { id, 0, false };

    switch (id) {
    case GameID::CATCH:
        result = runCatchGame(pet);
        break;
    case GameID::RUNNER:
        result = runRunnerGame(pet);
        break;
    default:
        break;
    }

    active      = false;
    currentGame = GameID::NONE;
    return result;
}

GameResult GameManager::runCatchGame(Pet& pet) {
    GameResult result = { GameID::CATCH, 0, false };
    int16_t score = catchGame.run(Display.u8g2);
    if (score < 0) return result;  // quit

    result.score = (uint16_t)score;

    // Apply to pet stats
    int16_t happyBonus = min(30, score / 2);
    pet.state.happiness = min(100, pet.state.happiness + happyBonus);
    pet.state.energy    = max(0,   pet.state.energy    - 15);
    pet.state.totalInteractions++;

    // Highscore
    if (result.score > pet.state.highscoreCatch) {
        pet.state.highscoreCatch = result.score;
        result.newHighscore = true;
        Audio.play(MelodyID::LEVEL_UP);
    } else {
        Audio.play(MelodyID::GAME_OVER);
    }

    return result;
}

GameResult GameManager::runRunnerGame(Pet& pet) {
    GameResult result = { GameID::RUNNER, 0, false };
    int16_t score = runnerGame.run(Display.u8g2);
    if (score < 0) return result;

    result.score = (uint16_t)score;

    int16_t happyBonus = min(25, score / 5);
    pet.state.happiness = min(100, pet.state.happiness + happyBonus);
    pet.state.energy    = max(0,   pet.state.energy    - 20);
    pet.state.totalInteractions++;

    if (result.score > pet.state.highscoreRunner) {
        pet.state.highscoreRunner = result.score;
        result.newHighscore = true;
        Audio.play(MelodyID::LEVEL_UP);
    } else {
        Audio.play(MelodyID::GAME_OVER);
    }

    return result;
}
