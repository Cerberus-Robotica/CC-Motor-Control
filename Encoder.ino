enum Pins { HALLA = 2, HALLB };


volatile long pulse_count = 0;
volatile unsigned long last_pulse_time = 0;
volatile float rpm_real_time = 0;
volatile float angular_velocity_real_time = 0;


const int pulses_per_revolution = 3;         // Pulsos por volta
const unsigned long print_interval = 500000; // 0.5s
const unsigned long rpm_timeout = 300000;    // 0.3s


void setup() {
  pinMode(HALLA, INPUT);
  pinMode(HALLB, INPUT);


  // Interrupção no HALLA
  attachInterrupt(digitalPinToInterrupt(HALLA), counter, RISING);


  Serial.begin(9600);
}


void loop() {
  static unsigned long last_print_time = 0;
  static long last_pulse_count = 0;


  unsigned long now_us = micros();


  // Se passou do tempo sem pulso, considera motor parado
  if (now_us - last_pulse_time > rpm_timeout) {
    rpm_real_time = angular_velocity_real_time = 0;
  }


  // Mostra informações a cada intervalo
  if (now_us - last_print_time >= print_interval) {
    noInterrupts();
    long current_pulse_count = pulse_count;
    float current_rpm = rpm_real_time, current_angular_velocity = angular_velocity_real_time;
    interrupts();


    Serial.print("RPM: ");
    Serial.print(current_rpm);
    Serial.print(" | Angular Velocity: ");
    Serial.print(current_angular_velocity);
    Serial.print(" | Total Pulses: ");
    Serial.print(current_pulse_count);
    Serial.print(" | Direction: ");


    if (current_pulse_count > last_pulse_count) {
      Serial.println("Clockwise");
    } else if (current_pulse_count < last_pulse_count) {
      Serial.println("Counter-clockwise");
    } else {
      Serial.println("Stopped");
    }


    last_pulse_count = current_pulse_count;
    last_print_time = now_us;
  }
}


void counter() {
  unsigned long now_us = micros();
  unsigned long between_pulses_time = now_us - last_pulse_time;
  last_pulse_time = now_us;


  // Direção: depende do estado de HALLB
  if (digitalRead(HALLB) == LOW) {
    pulse_count++;
  } else {
    pulse_count--;
  }


  // Calcula RPM e velocidade angular (proteção contra divisões inválidas)
  if (between_pulses_time > 0) {
    rpm_real_time = (60.0e6) / (between_pulses_time * pulses_per_revolution);
    angular_velocity_real_time = (2.0 * PI * 1e6) / (pulses_per_revolution * between_pulses_time);
  }
}


