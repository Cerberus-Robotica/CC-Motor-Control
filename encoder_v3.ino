#define HALLA 4
#define HALLB 5

volatile bool lastRawA = 0, lastRawB = 0;  // última leitura bruta
volatile bool stateA = 0, stateB = 0;      // estado filtrado (válido)
volatile unsigned long debounceTimeA = 0, debounceTimeB = 0;

const long debounceTime = 1000;  // 100 µs
const float tau = 10000, PPR = 3;

const int8_t transition_table[16] = {
  0, +1, -1, 0,
  -1, 0, 0, +1,
  +1, 0, 0, -1,
  0, -1, +1, 0
};

void setup() {
  pinMode(HALLA, INPUT);
  pinMode(HALLB, INPUT);

  attachInterrupt(digitalPinToInterrupt(HALLA), gpio_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(HALLB), gpio_isr, CHANGE);

  Serial.begin(9600);
}


void gpio_isr() {
  unsigned long now = micros();
  bool rawA = digitalRead(HALLA), rawB = digitalRead(HALLB);

  // Detectou mudança no sinal bruto
  if (rawA != lastRawA) {
    debounceTimeA = now;  // Marca tempo da mudança
    lastRawA = rawA;      // Atualiza o último valor lido
  }

  // Se o sinal ficou estável pelo tempo de debounce
  if ((now - debounceTimeA) >= debounceTime) {
    // Confirma o novo estado
    if (rawA != stateA) stateA = lastRawA;
  }

  if (rawB != lastRawB) {
    debounceTimeB = now;
    lastRawB = rawB;
  }

  if ((now - debounceTimeB) >= debounceTime) {
    if (rawB != stateB) stateB = lastRawB;
  }
}

void loop() {
  static unsigned long lastTime = 0;
  static unsigned long lastRead = 0;
  static float filteredTime = 0, RPM = 0, angularVelocity = 0;
  unsigned long now = micros();
  uint8_t stateAB = (stateA << 1) | stateB;
  static uint8_t lastAB = 0;
  const long timeout = 1000000;  // 1 segundo

  if (stateAB != lastAB) {
    int8_t dir = transition_table[(lastAB << 2) | stateAB];

    if (stateAB == 0b11 && dir != 0) {
      unsigned long rawTime = now - lastTime;
      float alpha = (float)rawTime / (tau + (float)rawTime);
      filteredTime = (1.0 - alpha) * filteredTime + alpha * rawTime;  // sem multiplicar por dir
      RPM = (60.0e6) * (float)dir / (filteredTime * PPR);
      angularVelocity = (2.0 * PI * 1e6 * (float)dir) / (filteredTime * PPR);
      lastTime = lastRead = now;
    }

    lastAB = stateAB;
  }

  // Timeout: se não houver pulso novo
  if (now - lastRead >= timeout) RPM = 0;

  static float lastRPM = 0;

  if (RPM != lastRPM) {
    Serial.print(RPM);
    Serial.println(" rpm");
    lastRPM = RPM;
  }
}
