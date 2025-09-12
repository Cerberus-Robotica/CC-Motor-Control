enum HALL_PINS { HALLA = 4, HALLB };
enum MOTOR_PINS { MOTOR_IN1 = 22, MOTOR_IN2 };

float kp = 1;
const float max_angular_velocity = 29;

volatile unsigned long last_pulse_time = 0;
volatile float angular_velocity_real_time = 0;

const int pulses_per_revolution = 3;         // Pulsos por volta
const unsigned long rpm_timeout = 300000;    // 0.3s

void setup() {
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);

  pinMode(HALLA, INPUT);
  pinMode(HALLB, INPUT);

  attachInterrupt(digitalPinToInterrupt(HALLA), counter, RISING);

  Serial.begin(9600);
  Serial.setTimeout(10); // evita travar no parseFloat
}

void loop() {
  static float desired_angular_velocity = 0;
  unsigned long now_us = micros();

  // Se passou do tempo sem pulso, considera motor parado
  if (now_us - last_pulse_time > rpm_timeout) {
    angular_velocity_real_time = 0;
  }

  // Ler valor do Serial se houver
  if (Serial.available() > 0) {
    desired_angular_velocity = Serial.parseFloat();
    while (Serial.available() > 0) Serial.read(); // limpa buffer
  }

  float absolut_angular_velocity = angular_velocity_real_time;
  float error = desired_angular_velocity - absolut_angular_velocity;

  // Controle proporcional
  float angular_velocity = desired_angular_velocity + kp * error;

  if (angular_velocity > 0) {
    analogWrite(MOTOR_IN1, constrain(angular_velocity * 255 / max_angular_velocity, 0, 255));
    digitalWrite(MOTOR_IN2, LOW);
  } else if (angular_velocity < 0) {
    digitalWrite(MOTOR_IN1, LOW);
    analogWrite(MOTOR_IN2, constrain(-angular_velocity * 255 / max_angular_velocity, 0, 255));
  } else {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, HIGH);
  }

  // Saída de debug
  Serial.print("IN: ");
  Serial.print(absolut_angular_velocity);
  Serial.print(" | SP: ");
  Serial.print(desired_angular_velocity);
  Serial.print(" | E: ");
  Serial.print(error);
  Serial.print(" | OUT: ");
  Serial.println(angular_velocity);

  delay(100); // imprime a cada 100ms
}

void counter() {
  unsigned long now_us = micros();
  unsigned long between_pulses_time = now_us - last_pulse_time;
  last_pulse_time = now_us;

  // Calcula RPM e velocidade angular (proteção contra divisões inválidas)
  if (between_pulses_time > 0) {
    if (digitalRead(HALLB) == LOW) {
      angular_velocity_real_time = (2.0 * PI * 1e6) / (pulses_per_revolution * between_pulses_time);
    } else {
        angular_velocity_real_time = -(2.0 * PI * 1e6) / (pulses_per_revolution * between_pulses_time);
    }
  }
}