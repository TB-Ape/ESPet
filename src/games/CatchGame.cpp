#include "CatchGame.h"

void CatchGame::init() {
    gs.playerX       = 56.0f;
    gs.score         = 0;
    gs.lives         = 3;
    gs.spawnTimer    = 0;
    gs.spawnInterval = 1500;
    gs.running       = true;
    gs.paused        = false;
    for (int i = 0; i < CATCH_MAX_ITEMS; i++) gs.items[i].active = false;
}

void CatchGame::spawnItem() {
    for (int i = 0; i < CATCH_MAX_ITEMS; i++) {
        if (!gs.items[i].active) {
            gs.items[i].x = (float)(esp_random() % (128 - CATCH_ITEM_SIZE));
            gs.items[i].y = 0.0f;
            gs.items[i].speed = 0.8f + (float)(esp_random() % 100) / 100.0f
                                      + gs.score * 0.002f;

            // Item type distribution: 60% food, 25% star, 15% poison
            uint8_t r = esp_random() % 100;
            if (r < 60)      gs.items[i].type = ItemType::FOOD;
            else if (r < 85) gs.items[i].type = ItemType::STAR;
            else             gs.items[i].type = ItemType::POISON;

            gs.items[i].active = true;
            return;
        }
    }
}

void CatchGame::update(uint32_t nowMs) {
    // Spawn
    if (nowMs - gs.spawnTimer >= gs.spawnInterval) {
        gs.spawnTimer = nowMs;
        spawnItem();
        // Increase difficulty
        if (gs.spawnInterval > 600) gs.spawnInterval -= 10;
    }

    // Move items
    for (int i = 0; i < CATCH_MAX_ITEMS; i++) {
        if (!gs.items[i].active) continue;
        gs.items[i].y += gs.items[i].speed;

        if (gs.items[i].y >= CATCH_GROUND_Y) {
            if (checkCollision(gs.items[i])) {
                // Caught!
                switch (gs.items[i].type) {
                case ItemType::FOOD:   gs.score += 1; break;
                case ItemType::STAR:   gs.score += 3; break;
                case ItemType::POISON: gs.lives--;    break;
                }
            } else {
                // Missed food → lose life
                if (gs.items[i].type == ItemType::FOOD) {
                    gs.lives--;
                }
            }
            gs.items[i].active = false;

            if (gs.lives == 0) gs.running = false;
        }
    }

    // Player movement (handled in run() via buttons)
}

bool CatchGame::checkCollision(const FallingItem& item) {
    float px  = gs.playerX;
    float pw  = CATCH_PLAYER_W;
    float ix  = item.x;
    float iw  = CATCH_ITEM_SIZE;
    return (ix + iw >= px) && (ix <= px + pw);
}

int16_t CatchGame::run(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    init();

    // Intro screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.drawStr(18, 20, "CATCH GAME");
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(10, 35, "FEED=Left  PLAY=Right");
    u8g2.drawStr(10, 45, "SLEEP=Quit");
    u8g2.drawStr(20, 58, "Press any to start");
    u8g2.sendBuffer();

    // Wait for button
    ButtonMsg msg;
    while (!Input.receive(msg, 100 / portTICK_PERIOD_MS));

    uint32_t lastUpdate = millis();
    const float PLAYER_SPEED = 3.5f;

    while (gs.running) {
        uint32_t nowMs = millis();
        uint32_t dt = nowMs - lastUpdate;
        lastUpdate = nowMs;

        // Input
        while (Input.receive(msg, 0)) {
            if (msg.evt == ButtonEvent::PRESS) {
                if (msg.btn == ButtonID::FEED)  gs.playerX -= PLAYER_SPEED * 3;
                if (msg.btn == ButtonID::PLAY)  gs.playerX += PLAYER_SPEED * 3;
                if (msg.btn == ButtonID::SLEEP) { gs.running = false; return -1; }
            }
        }

        // Continuous movement (poll button state)
        if (digitalRead(PIN_BTN_FEED) == LOW) gs.playerX -= PLAYER_SPEED;
        if (digitalRead(PIN_BTN_PLAY) == LOW) gs.playerX += PLAYER_SPEED;

        // Clamp player
        gs.playerX = max(0.0f, min(gs.playerX, 128.0f - CATCH_PLAYER_W));

        if (dt >= 33) {  // ~30fps
            update(nowMs);
            draw(u8g2);
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // Game over screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.drawStr(22, 20, "GAME OVER");
    u8g2.setFont(u8g2_font_6x10_tr);
    char buf[20];
    snprintf(buf, sizeof(buf), "Score: %d", gs.score);
    u8g2.drawStr(36, 38, buf);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(20, 55, "Press any to exit");
    u8g2.sendBuffer();

    while (!Input.receive(msg, 200 / portTICK_PERIOD_MS));

    return (int16_t)gs.score;
}

void CatchGame::draw(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    u8g2.clearBuffer();
    drawHUD(u8g2);
    drawItems(u8g2);
    drawPlayer(u8g2);
    // Ground line
    u8g2.drawHLine(0, CATCH_GROUND_Y, 128);
    u8g2.sendBuffer();
}

void CatchGame::drawPlayer(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    int16_t px = (int16_t)gs.playerX;
    // Draw player as a small pet face
    u8g2.drawRBox(px, CATCH_PLAYER_Y, CATCH_PLAYER_W, CATCH_PLAYER_H, 2);
    u8g2.setDrawColor(0);
    // Eyes
    u8g2.drawPixel(px + 3, CATCH_PLAYER_Y + 2);
    u8g2.drawPixel(px + 8, CATCH_PLAYER_Y + 2);
    u8g2.setDrawColor(1);
}

void CatchGame::drawItems(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    for (int i = 0; i < CATCH_MAX_ITEMS; i++) {
        if (!gs.items[i].active) continue;
        int16_t ix = (int16_t)gs.items[i].x;
        int16_t iy = (int16_t)gs.items[i].y;

        switch (gs.items[i].type) {
        case ItemType::FOOD:
            // Apple-like circle
            u8g2.drawCircle(ix + 3, iy + 3, 3);
            u8g2.drawLine(ix + 3, iy, ix + 4, iy - 2);  // stem
            break;
        case ItemType::STAR:
            // Star (8x8 sprite)
            u8g2.drawXBMP(ix, iy - 1, 8, 8, spr_star);
            break;
        case ItemType::POISON:
            // Skull: circle with X
            u8g2.drawCircle(ix + 3, iy + 2, 3);
            u8g2.drawLine(ix + 1, iy + 4, ix + 5, iy + 6);  // bones
            u8g2.drawLine(ix + 5, iy + 4, ix + 1, iy + 6);
            break;
        }
    }
}

void CatchGame::drawHUD(U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2) {
    u8g2.setFont(u8g2_font_5x7_tr);
    char buf[16];
    snprintf(buf, sizeof(buf), "Score:%d", gs.score);
    u8g2.drawStr(2, 8, buf);

    // Lives as hearts
    for (int i = 0; i < gs.lives; i++) {
        u8g2.drawXBMP(90 + i * 10, 1, 8, 8, spr_heart);
    }
}
