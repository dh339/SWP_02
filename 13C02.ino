#include <Servo.h>

#define PIN_SERVO 10
#define INTERVAL_MS 30

int US_0   = 400;   
int US_90  = 1400;  
int US_180 = 2600; 

float _SERVO_SPEED = 0.3;   
float START_ANGLE = 0;
float TARGET_ANGLE = 90;
float currAngle = 0;

unsigned long lastUpdate = 0;
Servo myservo;

// ★ 0°, 90°, 180° 펄스 값 기준으로 정확하게 보정해서 계산하는 함수
int angleToUs(float ang) {
  if (ang <= 90) {
    // 0°~90° 구간 : US_0 → US_90 선형 보간
    return US_0 + (US_90 - US_0) * (ang / 90.0);
  } else {
    // 90°~180° 구간 : US_90 → US_180 선형 보간
    return US_90 + (US_180 - US_90) * ((ang - 90.0) / 90.0);
  }
}

void setup() {
  myservo.attach(PIN_SERVO);
  currAngle = START_ANGLE;
  myservo.writeMicroseconds(angleToUs(currAngle));
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate < INTERVAL_MS) return;
  lastUpdate = now;

  float step = _SERVO_SPEED * (INTERVAL_MS / 1000.0);

  if (currAngle < TARGET_ANGLE) currAngle += step;
  else if (currAngle > TARGET_ANGLE) currAngle -= step;

  // 목표 도착 시 정확히 값 고정
  if ((TARGET_ANGLE - currAngle) * (TARGET_ANGLE - START_ANGLE) <= 0) {
    currAngle = TARGET_ANGLE;
  }

  myservo.writeMicroseconds(angleToUs(currAngle));
}
