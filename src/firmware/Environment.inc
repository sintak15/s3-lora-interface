// AM2302 on its own digital pin; unrelated to ADS1115 and I2C.
static_assert(AM2302_DATA_PIN == -1 || AM2302_DATA_PIN == 2,
              "AM2302 uses expansion GPIO2, never the UART0 header");
static_assert(AM2302_DATA_PIN < 0 || (AM2302_DATA_PIN != GPS_TX_PIN && AM2302_DATA_PIN != GPS_RX_PIN),
              "AM2302 cannot share the GPS UART pins; use receive-only ATGM336H mode");
static portMUX_TYPE environmentMux = portMUX_INITIALIZER_UNLOCKED;
static Am2302::Capture environmentCapture;
static Am2302::Reading environmentReading;
static uint32_t environmentAttempts = 0, environmentErrors = 0, environmentLastGoodMs = 0;

static void ARDUINO_ISR_ATTR environmentEdge() {
  uint32_t now = micros();
  bool high = digitalRead(AM2302_DATA_PIN) != LOW;
  portENTER_CRITICAL_ISR(&environmentMux);
  environmentCapture.edge(high, now);
  portEXIT_CRITICAL_ISR(&environmentMux);
}

static void serviceEnvironment() {
  if (AM2302_DATA_PIN < 0) return;
  static bool initialized = false, waiting = false;
  static uint32_t lastAttempt = 0, captureStarted = 0;
  const uint32_t now = millis();
  if (!initialized) {
    pinMode(AM2302_DATA_PIN, INPUT_PULLUP);
    initialized = true; lastAttempt = now;
    return; // Sensor must settle after power-on.
  }
  if (waiting) {
    bool finished;
    Am2302::Reading reading;
    portENTER_CRITICAL(&environmentMux);
    finished = environmentCapture.done || environmentCapture.failed;
    if (environmentCapture.done) reading = Am2302::decode(environmentCapture.bytes);
    portEXIT_CRITICAL(&environmentMux);
    if (!finished && uint32_t(now - captureStarted) < 20) return;
    detachInterrupt(digitalPinToInterrupt(AM2302_DATA_PIN));
    waiting = false;
    environmentReading = reading;
    if (reading.valid) environmentLastGoodMs = now;
    else ++environmentErrors;
    return;
  }
  if (uint32_t(now - lastAttempt) < 5000) return;
  lastAttempt = now; ++environmentAttempts;
  pinMode(AM2302_DATA_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(AM2302_DATA_PIN, LOW);
  delayMicroseconds(1200); // Only the host start pulse; reception uses interrupts.
  portENTER_CRITICAL(&environmentMux);
  environmentCapture.reset(micros());
  portEXIT_CRITICAL(&environmentMux);
  attachInterrupt(digitalPinToInterrupt(AM2302_DATA_PIN), environmentEdge, CHANGE);
  pinMode(AM2302_DATA_PIN, INPUT_PULLUP);
  captureStarted = millis(); waiting = true;
}

static bool environmentAvailable() {
  return AM2302_DATA_PIN >= 0 && environmentReading.valid && uint32_t(millis() - environmentLastGoodMs) < 15000;
}
