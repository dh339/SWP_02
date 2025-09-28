#define PIN_LED 7

unsigned int count, toggle;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  
  toggle = count = 0;
  digitalWrite(PIN_LED, 0);
  delay(1000);
}

void loop() {
  if(count < 11) {  
    Serial.println(++count);
    toggle = toggle_state(toggle);
    digitalWrite(PIN_LED, toggle);
    delay(100);
  }
  else{
    while(1){
      
      }
    }
}

int toggle_state(int toggle){
  return not toggle;
  }
