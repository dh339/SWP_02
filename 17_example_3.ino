#include <Servo.h>

// Arduino pin assignment
#define PIN_IR    0         // IR sensor at Pin A0
#define PIN_LED   9
#define PIN_SERVO 10

#define _DUTY_MIN 400    // servo full clock-wise position (0 degree)
#define _DUTY_NEU 1400   // servo neutral position (90 degree)
#define _DUTY_MAX 2606   // servo full counter-clockwise position (180 degree)
// ^^^ _DUTY_MAX를 2600 -> 2606으로 미세 보정(정수 나눗셈 오차 방지용, 체감 동일)

#define _DIST_MIN  100.0   // minimum distance 100mm (10 cm)
#define _DIST_MAX  250.0   // maximum distance 250mm (25 cm)

// ===== EMA 필터 계수 (INTERVAL=30ms 기준 권장: 0.2~0.3) =====
#define EMA_ALPHA  0.25

#define LOOP_INTERVAL 30   // Loop Interval (unit: msec)  >= 20ms 요구 충족

Servo myservo;
unsigned long last_loop_time = 0;   // unit: msec

float dist_prev = _DIST_MIN;
float dist_ema  = _DIST_MIN;

static inline float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

void setup()
{
  pinMode(PIN_LED, OUTPUT);

  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(_DUTY_NEU);

  Serial.begin(1000000);    // 1,000,000 bps
  last_loop_time = millis(); // ★ 주기 제어용 기준시간 초기화 (추가)
}

void loop()
{
  unsigned long time_curr = millis();
  int duty;
  float a_value, dist_raw;

  // wait until next event time
  if (time_curr < (last_loop_time + LOOP_INTERVAL))
    return;
  last_loop_time += LOOP_INTERVAL;

  a_value = analogRead(PIN_IR);

  // IR → 거리(mm) 변환 (안전 가드: a_value<=10이면 분모 보호)
  if (a_value <= 10) {
    dist_raw = 10000.0; // 터무니없이 큰 값으로 만들어 범위 밖 처리
  } else {
    dist_raw = ((6762.0 / (a_value - 9.0)) - 4.0) * 10.0 - 60.0;
  }

  // ===== 범위 필터 (10cm~25cm만 유효) + LED 제어 =====
  bool in_range = (dist_raw >= _DIST_MIN) && (dist_raw <= _DIST_MAX);
  digitalWrite(PIN_LED, in_range ? LOW : HIGH);

  float dist_meas;
  if (in_range) {
    dist_meas = dist_raw;       // 범위 안: 갱신 허용
    dist_prev = dist_raw;       // 최근 유효값 갱신
  } else {
    dist_meas = dist_prev;      // 범위 밖: 이전 유효값 유지(서보 안정)
  }

  // ===== EMA 필터 =====
  // dist_ema(k) = alpha * dist_meas(k) + (1-alpha) * dist_ema(k-1)
  dist_ema = (EMA_ALPHA * dist_meas) + ((1.0f - EMA_ALPHA) * dist_ema);

  // ===== map() 미사용 선형 매핑 (10~25cm -> 0~180deg -> PWM us) =====
  // duty = _DUTY_MIN + (dist_ema - _DIST_MIN) * (_DUTY_MAX - _DUTY_MIN) / (_DIST_MAX - _DIST_MIN)
  float t = (dist_ema - _DIST_MIN) / (_DIST_MAX - _DIST_MIN); // 0.0~1.0
  t = clampf(t, 0.0f, 1.0f); // 범위 클램프
  duty = (int)(_DUTY_MIN + t * (float)(_DUTY_MAX - _DUTY_MIN));

  myservo.writeMicroseconds(duty);

  // ===== 시리얼 플로터용 출력 포맷 유지 =====
  Serial.print("_DUTY_MIN:");  Serial.print(_DUTY_MIN);
  Serial.print("_DIST_MIN:");  Serial.print(_DIST_MIN);
  Serial.print(",IR:");        Serial.print(a_value);
  Serial.print(",dist_raw:");  Serial.print(dist_raw);
  Serial.print(",ema:");       Serial.print(dist_ema);
  Serial.print(",servo:");     Serial.print(duty);
  Serial.print(",_DIST_MAX:"); Serial.print(_DIST_MAX);
  Serial.print(",_DUTY_MAX:"); Serial.print(_DUTY_MAX);
  Serial.println("");
}
