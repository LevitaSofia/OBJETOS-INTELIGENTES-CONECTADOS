#include <Wire.h>              // Biblioteca para comunicação I2C.
#include <Adafruit_GFX.h>      // Biblioteca para gráficos em displays.
#include <Adafruit_SSD1306.h>  // Biblioteca para controlar o display OLED SSD1306.
#include <WiFi.h>              // Biblioteca para conexão Wi-Fi.
#include <WebServer.h>         // Biblioteca para criar um servidor HTTP.
#include <PubSubClient.h>      // Biblioteca para comunicação com o broker MQTT.

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1       // Pino reset do display (não necessário no ESP32)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int analogIn = 36;       // Pino VP (GPIO36) para leitura do sensor ACS712
int mVperAmp = 66;             // Sensibilidade do sensor ACS712 (66 para o modelo de 30A)
int ACSoffset = 2500;          // Offset do sensor ACS712 em mV
double Voltage = 0;
double Amps = 0;

const char* ssid = "Altatelas2G";            // Rede Wi-Fi
const char* password = "alta972600";         // Senha da rede Wi-Fi
const char* mqttServer = "broker.mqtt-dashboard.com"; // Endereço do broker MQTT
const int mqttPort = 1883;                   // Porta do broker MQTT

String ipAddress;                            // Armazena o endereço IP do ESP32
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando...");

  // Inicialização do display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("Falha ao inicializar o display OLED");
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  ipAddress = WiFi.localIP().toString();
  Serial.print("Conectado ao WiFi com IP: ");
  Serial.println(ipAddress);

  // Configuração do servidor HTTP
  server.on("/", handleRoot);
  server.begin();

  // Configuração do cliente MQTT
  mqttClient.setServer(mqttServer, mqttPort);
}

void loop() {
  // Processa clientes HTTP
  server.handleClient();
  
  // Calcula corrente e atualiza o display
  Calcula_corrente();
  exibeDisplay();

  // Verifica conexão MQTT
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Publica corrente e potência
  String correnteStr = String(Amps, 3);
  String potenciaStr = String(Amps * 127); // Considerando 127V como voltagem padrão
  mqttClient.publish("casa/monitoramento/corrente", correnteStr.c_str());
  mqttClient.publish("casa/monitoramento/potencia", potenciaStr.c_str());

  delay(5000);  // Intervalo entre leituras e publicações
}

void handleRoot() {
  // Função de resposta ao cliente HTTP com informações de corrente
  String html = "<html><head><title>Monitor de Corrente</title>";
  html += "<style>body{font-family: Arial; text-align: center;}</style></head><body>";
  html += "<h1>Monitor de Corrente</h1>";
  html += "<p><strong>Corrente RMS:</strong> " + String(Amps, 3) + " A</p>";
  html += "<p><strong>Potência Aparente:</strong> " + String(Amps * 127) + " WATTS</p>";
  html += "<p><strong>Endereço IP:</strong> " + ipAddress + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void Calcula_corrente() {
  int RawValue = analogRead(analogIn);                     // Lê o valor do pino analógico VP
  Voltage = (RawValue / 4095.0) * 3300;                    // Converte para mV
  Amps = (((Voltage - ACSoffset) / mVperAmp) / 10 + 0.950); // Calcula a corrente em A
  
  // Ajuste para mostrar apenas valores positivos
  if (Amps < 0) {
    Amps = 0;
  }

  Serial.print("Corrente (A): ");
  Serial.println(Amps, 3);
}

void exibeDisplay() {
  // Exibe os dados no display OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 0);
  display.print("Corrente (A)");
  display.setTextSize(2);
  display.setCursor(20, 30);
  display.print(Amps, 3);
  display.display();
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("conectado");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 2 segundos");
      delay(2000);
    }
  }
}
