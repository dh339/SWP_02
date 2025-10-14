/*
 * 도전과제1: 주차장 차단기 (Sigmoid Easing)
 * - 초음파로 차량 접근 감지 -> 서보 차단기 부드럽게 상승/하강
 * - 시그모이드(로지스틱) 이징으로 가감속
 * - 비블로킹 millis(), EMA, 히스테리시스 적용
 */

#include <Servo.h>
#include <math.h>

// ====== 핀 설정 ======
#define PIN_SERVO 10
#define PIN_TRIG  12
#define PIN_ECHO  13

// ====== 서보/동작 파라미터 ======
static const int   ANGLE_CLOSED = 0;
static const int   ANGLE_OPENED = 90;
static const unsigned long MOVE_MS = 1200;

// ====== 거리/센서 파라미터 ======
static const float SND_VEL  = 346.0f;
static const int   INTERVAL = 25;
static const int   PULSE_US = 10;
static const float EMA_ALPHA = 0.3f;
static const int   OPEN_DIST_MM  = 220;
static const int   CLOSE_DIST_MM = 260;
static const unsigned long ECHO_TIMEOUT_US = (INTERVAL / 2) * 1000UL;

// ====== 전역 ======
Servo gate;
unsigned long lastSample = 0;
bool   emaInited = false;
float  distEma   = 9999.0f;

enum GateState { IDLE, OPENING, CLOSING };
GateState state = IDLE;
unsigned long moveStart = 0;
int startAngle  = ANGLE_CLOSED;
int targetAngle = ANGLE_CLOSED;

// ====== 유틸 ======
float measureDistanceMM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(PULSE_US);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long duration = pulseIn(PIN_ECHO, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return NAN;
  return 0.173f * (float)duration;
}

float emaUpdate(float prev, float sample) {
  if (isnan(sample)) return prev;
  if (!emaInited) { emaInited = true; return sample; }
  return EMA_ALPHA * sample + (1.0f - EMA_ALPHA) * prev;
}

// Sigmoid easing: u∈[0,1] -> [0,1]
float easeSigmoid(float u) {
  const float k = 8.0f;                  // 6~10 권장
  float L0 = 1.0f / (1.0f + (float)exp(+k * 0.5f));
  float L1 = 1.0f / (1.0f + (float)exp(-k * 0.5f));
  float Lu = 1.0f / (1.0f + (float)exp(-k * (u - 0.5f)));
  return (Lu - L0) / (L1 - L0);
}

void startMove(int newTarget) {
  startAngle  = gate.read();
  targetAngle = newTarget;
  moveStart   = millis();
  state = (targetAngle > startAngle) ? OPENING : CLOSING;
}

void updateMove() {
  if (state == IDLE) return;

  unsigned long elapsed = millis() - moveStart;
  if (elapsed >= MOVE_MS) {
    gate.write(targetAngle);
    state = IDLE;
    return;
  }
  float u = (float)elapsed / (float)MOVE_MS;   // 0..1
  float e = easeSigmoid(u);
  float angle = startAngle + (targetAngle - startAngle) * e;
  gate.write((int)(angle + 0.5f));
}

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW); // 안정화
  gate.attach(PIN_SERVO);
  gate.write(ANGLE_CLOSED);
  delay(300);
}

void loop() {
  unsigned long now = millis();
  if (now - lastSample >= INTERVAL) {
    lastSample = now;

    float d = measureDistanceMM();
    distEma = emaUpdate(distEma, d);

    if (state == IDLE) {
      if (distEma <= OPEN_DIST_MM) {
        if (gate.read() != ANGLE_OPENED) startMove(ANGLE_OPENED);
      } else if (distEma >= CLOSE_DIST_MM) {
        if (gate.read() != ANGLE_CLOSED) startMove(ANGLE_CLOSED);
      }
    }
  }
  updateMove();
}
