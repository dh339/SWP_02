// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 13

// configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 100     // minimum distance to be measured (unit: mm)
#define _DIST_MAX 300     // maximum distance to be measured (unit: mm)

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL)     // coefficent to convert duration to distance

#define _EMA_ALPHA 1  // EMA weight of new sample (range: 0 to 1)
                          // Setting EMA to 1 effectively disables EMA filter.

#define MEDIAN_WIN 30  // N=3,10,30으로 바꿔가며 캡처

// global variables
unsigned long last_sampling_time;   // unit: msec
float dist_prev = _DIST_MAX;        // Distance last-measured
float dist_ema;                     // EMA distance

float samples[MEDIAN_WIN];
int   samp_count = 0;
int   samp_head  = 0;

float median_of_buffer();
bool  is_valid_distance(float d);

void setup() {
  // initialize GPIO pins
  pinMode(PIN_LED,OUTPUT);
  pinMode(PIN_TRIG,OUTPUT);
  pinMode(PIN_ECHO,INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // initialize serial port
  Serial.begin(57600);
}

void loop() {
  float dist_raw, dist_filtered;
 
  // wait until next sampling time.
  // millis() returns the number of milliseconds since the program started.
  // will overflow after 50 days.
  if (millis() < last_sampling_time + INTERVAL)
    return;

// get a distance reading from the USS

dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);


if (dist_raw == 0.0 || dist_raw > _DIST_MAX || dist_raw < _DIST_MIN) {
    // 무효 측정은 버퍼에 넣지 않음
} else {
    samples[samp_head] = dist_raw;
    samp_head = (samp_head + 1) % MEDIAN_WIN;
    if (samp_count < MEDIAN_WIN) samp_count++;
}

float dist_median = (samp_count > 0) ? median_of_buffer() : dist_raw;

// EMA
dist_ema = _EMA_ALPHA * dist_median + (1.0 - _EMA_ALPHA) * dist_ema;


  // output the distance to the serial port
  Serial.print("Min:");   Serial.print(_DIST_MIN);
  Serial.print(",raw:"); Serial.print(dist_raw);
  Serial.print(",ema:");  Serial.print(dist_ema);
  Serial.print(",median:");  Serial.print(dist_median);
  Serial.print(",Max:");  Serial.print(_DIST_MAX);
  Serial.println("");

  // do something here
  if ((dist_raw < _DIST_MIN) || (dist_raw > _DIST_MAX))
    digitalWrite(PIN_LED, 1);       // LED OFF
  else
    digitalWrite(PIN_LED, 0);       // LED ON

  // update last sampling time
  last_sampling_time += INTERVAL;
}

// get a distance reading from USS. return value is in millimeter.
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
 
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // unit: mm

  // Pulse duration to distance conversion example (target distance = 17.3m)
  // - pulseIn(ECHO, HIGH, timeout) returns microseconds (음파의 왕복 시간)
  // - 편도 거리 = (pulseIn() / 1,000,000) * SND_VEL / 2 (미터 단위)
  //   mm 단위로 하려면 * 1,000이 필요 ==>  SCALE = 0.001 * 0.5 * SND_VEL
  //
  // - 예, pusseIn()이 100,000 이면 (= 0.1초, 왕복 거리 34.6m)
  //        = 100,000 micro*sec * 0.001 milli/micro * 0.5 * 346 meter/sec
  //        = 100,000 * 0.001 * 0.5 * 346
  //        = 17,300 mm  ==> 17.3m
}

float median_of_buffer() {
  float tmp[MEDIAN_WIN];
  for (int i = 0; i < samp_count; i++) {
    int idx = (samp_head - 1 - i);
    if (idx < 0) idx += MEDIAN_WIN;
    tmp[i] = samples[idx];
  }
  for (int i = 1; i < samp_count; i++) {
    float key = tmp[i];
    int j = i - 1;
    while (j >= 0 && tmp[j] > key) {
      tmp[j+1] = tmp[j];
      j--;
    }
    tmp[j+1] = key;
  }
  if (samp_count % 2 == 1) {
    return tmp[samp_count / 2];
  } else {
    int r = samp_count / 2;
    return 0.5f * (tmp[r-1] + tmp[r]);
  }
}

bool is_valid_distance(float d) {
  if (d <= 0.0f) return false;
  if (d < _DIST_MIN) return false;
  if (d > _DIST_MAX) return false;
  return true;
}
