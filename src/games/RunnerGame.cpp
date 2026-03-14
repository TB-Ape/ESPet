#include "RunnerGame.h"

void RunnerGame::init() {
    gs.playerY   = (float)RUNNER_FLOOR_Y - RUNNER_PLAYER_H;
    gs.playerVY  = 0.0f;
    gs.onGround  = true;
    gs.ducking   = false;
    gs.distance  = 0;
    gs.speed     = 2.0f;
    gs.spawnDist = 0;
    gs.nextSpawn = 80;
    gs.running   = true;
    gs.dead      = false;
    for (int i = 0; i < RUNNER_MAX_OBSTACLES; i++) gs.obstacles[i].active = false;
}

void RunnerGame::spawnObstacle() {
    for (int i = 0; i < RUNNER_MAX_OBSTACLES; i++) {
        if (!gs.obstacles[i].active) {
            gs.obstacles[i].x     = 128.0f;
            gs.obstacles[i].isTall = (esp_random() % 2) == 0;
            gs.obstacles[i].w     = 6 + (esp_random() % 4);
            gs.obstacles[i].h     = gs.obstacles[i].isTall ? 14 : 8;
            gs.obstacles[i].active = true;
            return;
        }
    }
}

void RunnerGame::update() {
    // Player physics
    if (!gs.onGround || gs.playerVY != 0.0f) {
        gs.playerVY += RUNNER_GRAVITY;
        gs.playerY  += gs.playerVY;

        float floor = (float)RUNNER_FLOOR_Y - RUNNER_PLAYER_H;
        if (gs.playerY >= floor) {
            gs.playerY  = floor;
            gs.playerVY = 0.0f;
            gs.onGround = true;
            gs.ducking  = false;
        }
    }

    // Move obstacles
    for (int i = 0; i < RUNNER_MAX_OBSTACLES; i++) {
        if (!gs.obstacles[i].active) continue;
        gs.obstacles[i].x -= gs.speed;
        if (gs.obstacles[i].x + gs.obstacles[i].w < 0) {
            gs.obstacles[i].active = false;
        }
    }

    // Spawn
    gs.spawnDist++;
    if (gs.spawnDist >= gs.nextSpawn) {
        gs.spawnDist = 0;
        gs.nextSpawn = 60 + (esp_random() % 60);
        spawnObstacle();
    }

    // Distance counter + speed increase
    gs.distance++;
    gs.speed = 2.0f + gs.distance * 0.002f;
    if (gs.speed > 6.0f) gs.speed = 6.0f;

    // Collision
    if (checkCollision()) {
        gs.dead    = false;  // handled in run()
        gs.running = false;
    }
}

bool RunnerGame::checkCollision() {
    uint8_t ph = gs.ducking ? RUNNER_PLAYER_H / 2 : RUNNER_PLAYER_H;
    float py = gs.ducking
        ? gs.playerY + RUNNER_PLAYER_H / 2
        : gs.playerY;

    for (int i = 0; i < RUNNER_MAX_OBSTACLES; i++) {
        if (!gs.obstacles[i].active) continue;
        Obstacle& ob = gs.obstacles[i];

        float obY = (float)RUNNER_FLOOR_Y - ob.h;

        bool xOverlap = (RUNNER_PLAYER_X + RUNNER_PLAYER_W > ob.x) &&
                        (RUNNER_PLAYER_X < ob.x + ob.w);
        bool yOverlap = (py + ph > obY) && (py < (float)RUNNER_FLOOR_Y);

        if (xOverlap && yOverlap) return true;
    }
    return false;
}

int16_t RunnerGame::run(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    init();

    // Intro
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.drawStr(10, 20, "RUNNER GAME");
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(8, 33, "PLAY=Jump  FEED=Duck");
    u8g2.drawStr(8, 43, "SLEEP=Quit");
    u8g2.drawStr(20, 58, "Press any to start");
    u8g2.sendBuffer();

    ButtonMsg msg;
    while (!Input.receive(msg, 100 / portTICK_PERIOD_MS));

    uint32_t lastFrame = millis();

    while (gs.running) {
        uint32_t nowMs = millis();

        // Handle buttons
        while (Input.receive(msg, 0)) {
            if (msg.btn == ButtonID::SLEEP) { return -1; }
            if (msg.btn == ButtonID::PLAY && gs.onGround) {
                gs.playerVY = RUNNER_JUMP_VEL;
                gs.onGround = false;
            }
        }

        // Duck: hold FEED
        gs.ducking = (digitalRead(PIN_BTN_FEED) == LOW) && gs.onGround;

        if (nowMs - lastFrame >= 33) {
            lastFrame = nowMs;
            update();
            draw(u8g2);
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // Death screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.drawStr(22, 20, "GAME OVER");
    u8g2.setFont(u8g2_font_6x10_tr);
    char buf[24];
    snprintf(buf, sizeof(buf), "Dist: %lum", gs.distance / 10);
    u8g2.drawStr(30, 38, buf);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(20, 55, "Press any to exit");
    u8g2.sendBuffer();

    while (!Input.receive(msg, 200 / portTICK_PERIOD_MS));
    return (int16_t)min((uint32_t)32767, gs.distance / 10);
}

void RunnerGame::draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    u8g2.clearBuffer();
    drawHUD(u8g2);
    drawBackground(u8g2);
    drawObstacles(u8g2);
    drawPlayer(u8g2);
    u8g2.sendBuffer();
}

void RunnerGame::drawPlayer(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    uint8_t ph = gs.ducking ? RUNNER_PLAYER_H / 2 : RUNNER_PLAYER_H;
    float   py = gs.ducking ? gs.playerY + RUNNER_PLAYER_H / 2 : gs.playerY;

    u8g2.drawRBox(RUNNER_PLAYER_X, (int16_t)py, RUNNER_PLAYER_W, ph, 1);
    // Eye
    u8g2.setDrawColor(0);
    u8g2.drawPixel(RUNNER_PLAYER_X + 6, (int16_t)py + 2);
    u8g2.setDrawColor(1);

    // Legs animation (alternate based on distance)
    if (gs.onGround) {
        uint8_t legPhase = (uint8_t)(gs.distance % 8);
        if (legPhase < 4) {
            u8g2.drawLine(RUNNER_PLAYER_X + 2, (int16_t)py + ph,
                          RUNNER_PLAYER_X + 1, (int16_t)py + ph + 3);
            u8g2.drawLine(RUNNER_PLAYER_X + 5, (int16_t)py + ph,
                          RUNNER_PLAYER_X + 6, (int16_t)py + ph + 3);
        } else {
            u8g2.drawLine(RUNNER_PLAYER_X + 2, (int16_t)py + ph,
                          RUNNER_PLAYER_X + 3, (int16_t)py + ph + 3);
            u8g2.drawLine(RUNNER_PLAYER_X + 5, (int16_t)py + ph,
                          RUNNER_PLAYER_X + 4, (int16_t)py + ph + 3);
        }
    }
}

void RunnerGame::drawObstacles(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    for (int i = 0; i < RUNNER_MAX_OBSTACLES; i++) {
        if (!gs.obstacles[i].active) continue;
        Obstacle& ob = gs.obstacles[i];
        int16_t ox = (int16_t)ob.x;
        int16_t oy = (int16_t)RUNNER_FLOOR_Y - ob.h;

        if (ob.isTall) {
            // Cactus-like: main stem + arms
            u8g2.drawBox(ox + ob.w / 2 - 1, oy, 3, ob.h);
            u8g2.drawBox(ox, oy + 3, ob.w, 3);
        } else {
            // Short rock
            u8g2.drawBox(ox, oy, ob.w, ob.h);
            // Highlight top-left
            u8g2.setDrawColor(0);
            u8g2.drawPixel(ox, oy);
            u8g2.setDrawColor(1);
        }
    }
}

void RunnerGame::drawBackground(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    // Ground
    u8g2.drawHLine(0, RUNNER_FLOOR_Y, 128);
    // Scrolling ground dots
    for (int x = 0; x < 128; x += 8) {
        int8_t ox = (int8_t)((x - (gs.distance % 8)) & 0x7F);
        u8g2.drawPixel(ox, RUNNER_FLOOR_Y + 1);
    }
    // Clouds
    uint32_t cloudOff = gs.distance / 2;
    for (int c = 0; c < 3; c++) {
        int cx = (int)((120 - (cloudOff + c * 40) % 140));
        if (cx > 0 && cx < 120) {
            u8g2.drawCircle(cx,     12 + c * 5, 5);
            u8g2.drawCircle(cx + 7, 10 + c * 5, 6);
            u8g2.drawCircle(cx + 14, 12 + c * 5, 4);
        }
    }
}

void RunnerGame::drawHUD(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    u8g2.setFont(u8g2_font_5x7_tr);
    char buf[16];
    snprintf(buf, sizeof(buf), "%lum", gs.distance / 10);
    u8g2.drawStr(90, 8, buf);
}
