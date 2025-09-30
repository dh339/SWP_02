/*
 * 도전과제 2: PWM 함수 구현 (software PWM)
 * - 구현 함수:
 * void set_period(int period_us); // 100 ~ 10000 us
 * void set_duty(int duty_pct);    // 0 ~ 100 %
 * - 밝기 제어: 1초 동안 최소→최대→최소 (triangle), 101단계(0~100)
 * - period는 코드 상단 PWM_PERIOD_US로 직접 수정(예: 10000, 1000, 100)
 * 20252420 인공지능학부 한덕현
 */

const int LED_PIN = 7;
#define PWM_PERIOD_US 10000 // 바꾸기: 100(0.1ms), 1000(1ms), 10000(10ms)

volatile unsigned int pwm_period_us = PWM_PERIOD_US; // 100~10000
volatile int pwm_duty_pct = 0;                       // 0~100

// 값 제한 함수
static inline int clamp_value(int value, int min_val, int max_val) {
  return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

// 주기 설정
void set_period(int period_us) {
  pwm_period_us = (unsigned int)clamp_value(period_us, 100, 10000);
}

// 듀티 설정
void set_duty(int duty_pct) {
  pwm_duty_pct = clamp_value(duty_pct, 0, 100);
}

// 현재 pwm_period_us로 PWM 1주기 출력
void pwm_one_cycle(int duty_pct) {
  // 0% / 100%는 분기 (토글 최소화)
  if (duty_pct <= 0) {
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(pwm_period_us);
    return;
  }
  if (duty_pct >= 100) {
    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(pwm_period_us);
    return;
  }

  unsigned int on_time_us  = (unsigned long)pwm_period_us * (unsigned int)duty_pct / 100UL;
  unsigned int off_time_us = pwm_period_us - on_time_us;

  if (on_time_us)  { digitalWrite(LED_PIN, HIGH); delayMicroseconds(on_time_us); }
  if (off_time_us) { digitalWrite(LED_PIN, LOW ); delayMicroseconds(off_time_us); }
}

// 1초 동안 0→100→0 트라이앵글 (101단계 기준)
// 단계 시퀀스: 0..100 (101단계) + 99..1 (99단계) = 총 200스텝
void pwm_run_triangle_1s() {
  const int total_steps = 200; // 0..100 + 99..1
  unsigned long total_cycles = 1000000UL / pwm_period_us; // 1초 동안 총 사이클 수
  if (total_cycles == 0) total_cycles = 1;

  unsigned long cycles_per_step = total_cycles / total_steps; // 스텝당 기본 사이클 수
  unsigned long remainder       = total_cycles % total_steps; // 앞쪽 remainder 스텝에 +1

  int step_index = 0;

  // 상승: 0 → 100
  for (int d = 0; d <= 100; ++d) {
    set_duty(d);
    unsigned long do_cycles = cycles_per_step + ((step_index < (int)remainder) ? 1 : 0);
    if (do_cycles == 0) do_cycles = 1;
    for (unsigned long i = 0; i < do_cycles; ++i) pwm_one_cycle(pwm_duty_pct);
    step_index++;
  }

  // 하강: 99 → 1 (0과 100은 중복 방지)
  for (int d = 99; d >= 1; --d) {
    set_duty(d);
    unsigned long do_cycles = cycles_per_step + ((step_index < (int)remainder) ? 1 : 0);
    if (do_cycles == 0) do_cycles = 1;
    for (unsigned long i = 0; i < do_cycles; ++i) pwm_one_cycle(pwm_duty_pct);
    step_index++;
  }

  digitalWrite(LED_PIN, LOW); // 마무리 OFF
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  set_period(PWM_PERIOD_US);
}

void loop() {
  pwm_run_triangle_1s();
}
