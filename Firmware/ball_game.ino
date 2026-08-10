#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

//  Display
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128
#define OLED_RESET    -1
#define OLED_I2C_ADDR 0x3D
Adafruit_SH1107 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// accelerometer
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Axis remap since on PCB, accelerometer is tilted 90 degrees left
#define SWAP_XY      true  // sensor X/Y are swapped
#define TILT_X_SIGN  -1    // flip to +1 if the ball rolls left/right backwards
#define TILT_Y_SIGN   1    // flip to -1 if the ball rolls up/down backwards

// Ball physics 
float ballX = SCREEN_WIDTH  / 2.0;
float ballY = SCREEN_HEIGHT / 2.0;
float velX = 0, velY = 0;

const float BALL_RADIUS = 4.0;
const float MARGIN      = 2.0;   // gap between the arena wall and the screen edge
const float ACCEL_SCALE = 40.0;  // higher = ball reacts to tilt more strongly
const float DAMPING     = 0.985; // rolling friction; closer to 1.0 = less friction
const float RESTITUTION = 0.55;  // bounciness off the walls, 0 = stops, 1 = perfect bounce

unsigned long lastFrame = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(D4, D5); // SDA = D4, SCL = D5

  if (!display.begin(OLED_I2C_ADDR, true)) {
    Serial.println("SH1107 not found — check wiring/address.");
    while (1) delay(10);
  }
  display.clearDisplay();
  display.display();

  if (!accel.begin()) {
    Serial.println("ADXL345 not found — check wiring.");
    while (1) delay(10);
  }
  accel.setRange(ADXL345_RANGE_4_G);

  lastFrame = millis();
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastFrame) / 1000.0;
  if (dt <= 0) return;
  if (dt > 0.05) dt = 0.05; // clamp so a stall doesn't fling the ball
  lastFrame = now;

  // Read tilt
  sensors_event_t accelEvent;
  accel.getEvent(&accelEvent);
  float rawX = accelEvent.acceleration.x;
  float rawY = accelEvent.acceleration.y;

  float tiltX, tiltY;
  if (SWAP_XY) {
    tiltX = TILT_X_SIGN * rawY;
    tiltY = TILT_Y_SIGN * rawX;
  } else {
    tiltX = TILT_X_SIGN * rawX;
    tiltY = TILT_Y_SIGN * rawY;
  }

  // Physics
  velX += tiltX * ACCEL_SCALE * dt;
  velY += tiltY * ACCEL_SCALE * dt;
  velX *= DAMPING;
  velY *= DAMPING;
  ballX += velX * dt;
  ballY += velY * dt;

  // Wall collision
  float minX = MARGIN + BALL_RADIUS;
  float maxX = SCREEN_WIDTH  - MARGIN - BALL_RADIUS;
  float minY = MARGIN + BALL_RADIUS;
  float maxY = SCREEN_HEIGHT - MARGIN - BALL_RADIUS;

  if (ballX < minX) { ballX = minX; velX = -velX * RESTITUTION; }
  if (ballX > maxX) { ballX = maxX; velX = -velX * RESTITUTION; }
  if (ballY < minY) { ballY = minY; velY = -velY * RESTITUTION; }
  if (ballY > maxY) { ballY = maxY; velY = -velY * RESTITUTION; }

  // Draw 
  display.clearDisplay();
  display.drawRoundRect((int)MARGIN, (int)MARGIN,
                         SCREEN_WIDTH  - (int)(2 * MARGIN),
                         SCREEN_HEIGHT - (int)(2 * MARGIN),
                         6, SH110X_WHITE);
  display.fillCircle((int)ballX, (int)ballY, (int)BALL_RADIUS, SH110X_WHITE);
  display.display();
}