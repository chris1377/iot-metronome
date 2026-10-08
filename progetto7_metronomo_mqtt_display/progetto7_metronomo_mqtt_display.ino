#include <WiFiS3.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>
#include "Arduino_LED_Matrix.h"

char ssid[] = "cloud";
char pass[] = "password";
const char* brokerIP = "10.42.0.1"; 

WiFiClient espClient;
PubSubClient mqttClient(espClient);
LiquidCrystal lcd(12, 11, 10, 7, 6, 3);
ArduinoLEDMatrix matrix;

byte notaMusicale[8][12] = {
  { 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0 },
  { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 }
};
byte schermoVuoto[8][12] = { 0 };

int minBPM = 0;
int maxBPM = 0;

const int pinLedVerde = 8;
const int pinLedRosso = 9;
const int pinBtnMode = 4;
const int pinBtnTap = 5;
const int pinBtnBuzzer = 2; 
const int pinBuzzer = A0;   

bool isLearnMode = false;
bool isBuzzerEnabled = true; 

int ultimoStatoBtn = HIGH;
int ultimoStatoTap = HIGH;
int ultimoStatoBtnBuzzer = HIGH;

unsigned long tempiTap[10];
int numeroTap = 0;         

int currentBPM = 0;
unsigned long playInterval = 0;
unsigned long ultimoBlink = 0;
int ultimoBPMStampato = -1;
unsigned long ultimoTentativoRete = 0;

// ==========================================
// FUNZIONI METRONOMO (Fisiche + Remote)
// ==========================================
void impostaSuono(bool stato) {
  isBuzzerEnabled = stato;
  lcd.clear();
  lcd.setCursor(0, 0);
  if (isBuzzerEnabled) {
    lcd.print("Suono: ATTIVO   ");
    matrix.renderBitmap(notaMusicale, 8, 12); 
    if (mqttClient.connected()) mqttClient.publish("metronomo/suono/stato", "ON"); 
  } else {
    lcd.print("Suono: MUTO     ");
    matrix.renderBitmap(schermoVuoto, 8, 12);
    if (mqttClient.connected()) mqttClient.publish("metronomo/suono/stato", "OFF"); 
  }
  ultimoBPMStampato = -1; 
}

void cambiaModalita() {
  isLearnMode = !isLearnMode; 
  if (isLearnMode) {
    if (mqttClient.connected()) mqttClient.publish("metronomo/stato/rec", "ON");
    numeroTap = 0;
    digitalWrite(pinLedVerde, LOW);
    digitalWrite(pinLedRosso, HIGH);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Modalita': REC");
    lcd.setCursor(0, 1);
    lcd.print("Batti il tempo");
    ultimoBPMStampato = -1; 
  } else {
    if (mqttClient.connected()) mqttClient.publish("metronomo/stato/rec", "OFF");
    if (numeroTap >= 4) {
      unsigned long tempoTotale = tempiTap[numeroTap - 1] - tempiTap[0];
      unsigned long intervalloMedio = tempoTotale / (numeroTap - 1);

      currentBPM = 60000 / intervalloMedio;
      playInterval = intervalloMedio;

      if (minBPM == 0 || currentBPM < minBPM) minBPM = currentBPM;
      if (currentBPM > maxBPM) maxBPM = currentBPM;

      if (mqttClient.connected()) mqttClient.publish("metronomo/bpm", String(currentBPM).c_str());
    }
    digitalWrite(pinLedRosso, LOW);
    lcd.clear(); 
  }
}

void registraTap() {
  if (isLearnMode && numeroTap < 10) {
    tempiTap[numeroTap] = millis();
    numeroTap++;
    
    digitalWrite(pinLedRosso, LOW);
    if (isBuzzerEnabled) tone(pinBuzzer, 1200, 30);
    delay(50);
    digitalWrite(pinLedRosso, HIGH);
    
    lcd.setCursor(0, 1);
    lcd.print("Tap num: ");
    lcd.print(numeroTap);
    lcd.print("   ");
  }
}

// ==========================================
// RICEZIONE MESSAGGI MQTT
// ==========================================
// ==========================================
// RICEZIONE MESSAGGI MQTT
// ==========================================
void ricezioneMessaggio(char* topic, byte* payload, unsigned int length) {
  String messaggioRicevuto = "";
  for (unsigned int i = 0; i < length; i++) {
    messaggioRicevuto += (char)payload[i];
  }
  messaggioRicevuto.trim();
  messaggioRicevuto.toUpperCase();

  // Imposta BPM tramite tastierino numerico
  if (String(topic) == "metronomo/bpm/set") {
    int nuovoBPM = messaggioRicevuto.toInt();
    if (nuovoBPM > 0) {
      currentBPM = nuovoBPM;
      playInterval = 60000 / currentBPM; 
      if (minBPM == 0 || currentBPM < minBPM) minBPM = currentBPM;
      if (currentBPM > maxBPM) maxBPM = currentBPM;
      if (mqttClient.connected()) mqttClient.publish("metronomo/bpm", String(currentBPM).c_str());
    }
  }
  
  // Imposta Suono
  if (String(topic) == "metronomo/suono/set") {
    if ((messaggioRicevuto == "ON" || messaggioRicevuto == "TRUE" || messaggioRicevuto == "1") && !isBuzzerEnabled) {
      impostaSuono(true);
    } 
    else if ((messaggioRicevuto == "OFF" || messaggioRicevuto == "FALSE" || messaggioRicevuto == "0") && isBuzzerEnabled) {
      impostaSuono(false);
    }
  }

  // Cambio modalità da Node-RED (Ora scatta appena arriva il messaggio, a prescindere dal testo)
  if (String(topic) == "metronomo/stato/set") {
    cambiaModalita();
  }

  // Battito tempo da Node-RED (Ora scatta appena arriva il messaggio, a prescindere dal testo)
  if (String(topic) == "metronomo/tap/set") {
    registraTap();
  }
}

void gestisciRete() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      if (millis() - ultimoTentativoRete > 5000) {
        ultimoTentativoRete = millis();
        if (mqttClient.connect("Arduino_UNO_R4")) {
          // Iscrizioni aggiornate
          mqttClient.subscribe("metronomo/bpm/set");
          mqttClient.subscribe("metronomo/suono/set");
          mqttClient.subscribe("metronomo/stato/set");
          mqttClient.subscribe("metronomo/tap/set");
          
          if (isBuzzerEnabled) mqttClient.publish("metronomo/suono/stato", "ON");
          else mqttClient.publish("metronomo/suono/stato", "OFF");
        }
      }
    } else {
      mqttClient.loop();
    }
  }
}

void setup() {
  Serial.begin(115200);

  matrix.begin();
  matrix.renderBitmap(notaMusicale, 8, 12);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Avvio Metronomo");

  WiFi.begin(ssid, pass);
  unsigned long startAttesa = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttesa < 5000)) { delay(500); }

  lcd.clear();
  lcd.setCursor(0, 0);
  if (WiFi.status() == WL_CONNECTED) lcd.print("WiFi Connesso");
  else lcd.print("Modalita' OFFLINE"); 
  delay(1500);

  mqttClient.setServer(brokerIP, 1883);
  mqttClient.setCallback(ricezioneMessaggio);
  
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinLedRosso, OUTPUT);
  pinMode(pinBtnMode, INPUT_PULLUP);
  pinMode(pinBtnTap, INPUT_PULLUP);
  pinMode(pinBtnBuzzer, INPUT_PULLUP);
  pinMode(pinBuzzer, OUTPUT);
  
  lcd.clear();
}

void loop() {
  gestisciRete();

  static unsigned long ultimoInvio = 0;
  if (mqttClient.connected() && (millis() - ultimoInvio > 3000)) {
    mqttClient.publish("metronomo/bpm", String(currentBPM).c_str());
    mqttClient.publish("metronomo/bpm/min", String(minBPM).c_str());
    mqttClient.publish("metronomo/bpm/max", String(maxBPM).c_str());
    ultimoInvio = millis();
  }

  // Pulsanti Fisici
  int statoBtnBuzzer = digitalRead(pinBtnBuzzer);
  if (statoBtnBuzzer == LOW && ultimoStatoBtnBuzzer == HIGH) {
    impostaSuono(!isBuzzerEnabled); 
    delay(300); 
  }
  ultimoStatoBtnBuzzer = statoBtnBuzzer;

  int statoBtnMode = digitalRead(pinBtnMode);
  if(statoBtnMode == LOW && ultimoStatoBtn == HIGH){
    cambiaModalita();
    delay(250); 
  }
  ultimoStatoBtn = statoBtnMode;

  int statoBtnTap = digitalRead(pinBtnTap);
  if(statoBtnTap == LOW && ultimoStatoTap == HIGH) {
    registraTap();
  }
  ultimoStatoTap = statoBtnTap;

  // RIPRODUZIONE
  if (!isLearnMode) {
    if (currentBPM > 0) {
      if (millis() - ultimoBlink >= playInterval) {
        ultimoBlink = millis();
        digitalWrite(pinLedVerde, !digitalRead(pinLedVerde));
        if (isBuzzerEnabled) tone(pinBuzzer, 800, 50);
      }
    } else {
      digitalWrite(pinLedVerde, LOW);
    }
    
    if (currentBPM != ultimoBPMStampato) {
      lcd.setCursor(0, 0);
      if (isBuzzerEnabled) lcd.print("PLAY - Suono ON ");
      else lcd.print("PLAY - Suono OFF");
      
      lcd.setCursor(0, 1);
      lcd.print("BPM: ");
      lcd.print(currentBPM);
      lcd.print("     ");
      ultimoBPMStampato = currentBPM;
    }
  }
}