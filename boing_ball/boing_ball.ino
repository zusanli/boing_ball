/*
 * ============================================================
 *  Amiga Boing Ball  –  TTGO T-Display (ST7789, 135 × 240)
 *  Double-Buffering via Full-Screen-Sprite → kein Flackern
 * ============================================================
 */

#include <TFT_eSPI.h>
#include <math.h>

TFT_eSPI    tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);   // Vollbild-Framebuffer

static const int16_t SCR_W  = 135;
static const int16_t SCR_H  = 240;
static const int16_t BALL_R = 40;
static const int16_t CELL   = 15;      // 9×16 Kacheln, teilt 135 und 240 exakt

static const uint16_t BG_DARK  = 0x1082;
static const uint16_t BG_LIGHT = 0x2104;
static const uint16_t SHADOW_C = 0x2965;

float bx  = SCR_W / 2.0f;
float by  = SCR_H / 2.0f;
float vx  = 2.5f;
float vy  = 2.0f;
float rot = 0.0f;

// ----------------------------------------------------------------
void drawBackground() {
  for (int y = 0; y < SCR_H; y += CELL)
    for (int x = 0; x < SCR_W; x += CELL)
      spr.fillRect(x, y, CELL, CELL,
        ((x / CELL + y / CELL) & 1) ? BG_LIGHT : BG_DARK);
}

// ----------------------------------------------------------------
void renderBall(int cx, int cy, float angle) {
  const float ca  = cosf(angle);
  const float sa  = sinf(angle);
  const float inv = 1.0f / BALL_R;

  for (int dy = -BALL_R; dy <= BALL_R; dy++) {
    float ny  = dy * inv;
    float ny2 = ny * ny;
    if (ny2 >= 1.0f) continue;
    int hw = (int)(sqrtf(1.0f - ny2) * BALL_R);

    for (int dx = -hw; dx <= hw; dx++) {
      float nx = dx * inv;
      float nz = sqrtf(fmaxf(0.0f, 1.0f - nx*nx - ny2));

      // Y-Achsen-Rotation
      float rx = nx * ca + nz * sa;
      float ry = ny;
      float rz = -nx * sa + nz * ca;

      // Kugelkoordinaten → Schachbrettmuster
      float theta = acosf(fmaxf(-1.0f, fminf(1.0f, ry)));
      float phi   = atan2f(rx, rz) + (float)M_PI;

      int ti  = (int)(theta * 8.0f / (float)M_PI);
      int pi_ = (int)(phi   * 4.0f / (float)M_PI);
      bool isRed = (ti + pi_) & 1;

      // Diffuses Licht von vorne
      float light = 0.45f + 0.55f * nz;

      uint16_t col;
      if (isRed)
        col = tft.color565((uint8_t)(210.0f * light), 0, 0);
      else {
        uint8_t w = (uint8_t)(255.0f * light);
        col = tft.color565(w, w, w);
      }
      // Direkt in den Framebuffer-Sprite zeichnen
      spr.drawPixel(cx + dx, cy + dy, col);
    }
  }
}

// ----------------------------------------------------------------
void setup() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  spr.setColorDepth(16);
  spr.createSprite(SCR_W, SCR_H);
}

// ----------------------------------------------------------------
void loop() {
  bx += vx;
  by += vy;
  rot += vx * 0.023f;

  // Wandkollision
  if (bx - BALL_R < 0)        { bx = BALL_R;         vx =  fabsf(vx); }
  if (bx + BALL_R >= SCR_W)   { bx = SCR_W - BALL_R; vx = -fabsf(vx); }
  if (by - BALL_R < 0)        { by = BALL_R;         vy =  fabsf(vy); }
  if (by + BALL_R >= SCR_H)   { by = SCR_H - BALL_R; vy = -fabsf(vy); }

  // --- Alles in den Sprite zeichnen (unsichtbar) ---
  drawBackground();

  // Schatten
  int sy = (int)(by + BALL_R - 4);
  if (sy < SCR_H - 2)
    spr.fillEllipse((int)bx, sy,
                    (int)(BALL_R * 0.75f),
                    (int)(BALL_R * 0.18f),
                    SHADOW_C);

  // Ball
  renderBall((int)bx, (int)by, rot);

  // --- Kompletten Frame in einem SPI-Burst ausgeben ---
  spr.pushSprite(0, 0);
}
