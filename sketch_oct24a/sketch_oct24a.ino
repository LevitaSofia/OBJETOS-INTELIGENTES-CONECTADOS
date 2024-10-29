#include <Wire.h>              // Biblioteca para comunicação I2C.
#include <Adafruit_GFX.h>      // Biblioteca para gráficos em displays.
#include <Adafruit_SSD1306.h>  // Biblioteca para controlar o display OLED SSD1306.
#include <WiFi.h>              // Biblioteca para conexão Wi-Fi.
#include <WebServer.h>         // Biblioteca para criar um servidor HTTP.
#include <PubSubClient.h>      // Biblioteca para comunicação com o broker MQTT.

// Define as dimensões do display OLED
#define SCREEN_WIDTH 128       // Largura do display em pixels
#define SCREEN_HEIGHT 64       // Altura do display em pixels
#define OLED_RESET -1          // Pino de reset (não necessário para o ESP32)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Inicializa o objeto display

const int analogIn = 36;       // Pino de entrada analógica VP (GPIO36) para leitura do sensor de corrente ACS712
const int pinoSaida = 5;       // Pino digital 5, que será acionado com base na corrente
int mVperAmp = 66;             // Sensibilidade do sensor ACS712 em mV/A para o modelo de 30A
int ACSoffset = 2500;          // Offset do sensor ACS712 em mV
double Voltage = 0;            // Variável para armazenar a tensão medida
double Amps = 0;               // Variável para armazenar a corrente medida em amperes

// Credenciais da rede Wi-Fi
const char* ssid = "Altatelas2G";      // Nome da rede Wi-Fi
const char* password = "alta972600";   // Senha da rede Wi-Fi
const char* mqttServer = "192.168.18.82"; // Endereço IP do broker MQTT
const int mqttPort = 1883;             // Porta do broker MQTT

String ipAddress;                      // Variável para armazenar o IP do ESP32 na rede
WiFiClient wifiClient;                 // Cliente Wi-Fi para conexão com o broker MQTT
PubSubClient mqttClient(wifiClient);   // Cliente MQTT configurado com o cliente Wi-Fi
WebServer server(80);                  // Cria um servidor HTTP na porta 80

void setup() {
  Serial.begin(115200);                     // Inicializa a comunicação serial para depuração
  Serial.println("Inicializando...");

  // Inicializa o display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Verifica se o display inicializou corretamente
    Serial.println("Falha ao inicializar o display OLED");
    while (true);                                    // Se falhar, fica em loop infinito
  }
  display.clearDisplay();                            // Limpa o display
  display.setTextColor(SSD1306_WHITE);               // Define a cor do texto como branco

  // Configura o pino 5 como saída
  pinMode(pinoSaida, OUTPUT);                        // Define o pino de saída para controle do relé
  digitalWrite(pinoSaida, LOW);                      // Inicializa o pino desligado

  // Conexão à rede Wi-Fi
  WiFi.begin(ssid, password);                        // Inicia a conexão com a rede Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {            // Aguarda até estar conectado
    delay(1000);                                     // Aguarda 1 segundo
    Serial.println("Conectando ao WiFi...");
  }
  ipAddress = WiFi.localIP().toString();             // Obtém o IP do ESP32
  Serial.print("Conectado ao WiFi com IP: ");
  Serial.println(ipAddress);

  // Configuração do servidor HTTP
  server.on("/", handleRoot);                        // Configura o manipulador de requisições para a página raiz
  server.begin();                                    // Inicia o servidor

  // Configuração do cliente MQTT
  mqttClient.setServer(mqttServer, mqttPort);        // Define o endereço e a porta do broker MQTT
}

void loop() {
  server.handleClient();                             // Processa as requisições do cliente HTTP

  // Calcula a corrente e exibe no display
  Calcula_corrente();                                // Função que calcula a corrente
  exibeDisplay();                                    // Função que exibe a corrente no display OLED

  // Verifica se o cliente MQTT está conectado
  if (!mqttClient.connected()) {
    reconnectMQTT();                                 // Se não estiver conectado, tenta reconectar ao broker MQTT
  }
  mqttClient.loop();                                 // Mantém a conexão com o broker ativa

  // Publica corrente e potência
  String ipAddressStr = String(ipAddress);           // Converte o IP para uma string
  String correnteStr = String(Amps, 3);              // Converte a corrente para string com 3 casas decimais
  String potenciaStr = String(Amps * 127);           // Calcula a potência e converte para string, assumindo 127V

  // Publica os dados nos tópicos MQTT
  mqttClient.publish("Casa/Monitoramento_Energia/corrente", correnteStr.c_str());
  mqttClient.publish("Casa/Monitoramento_Energia/potencia", potenciaStr.c_str());
  mqttClient.publish("Casa/Monitoramento_Energia/Ipv4 rede local do esp32", ipAddressStr.c_str());

  // Publica o estado do relé com base na corrente
  if (Amps == 0) {
    mqttClient.publish("Casa/Monitoramento_Energia/rele", "Desligado"); // Publica que o relé está desligado
  } else {
    mqttClient.publish("Casa/Monitoramento_Energia/rele", "Ligado");    // Publica que o relé está ligado
  }

  delay(5000);                                       // Intervalo de 5 segundos entre leituras e publicações
}

void handleRoot() {
  // Função que gera uma página HTML com as informações de corrente e potência
  String html = "<html><head><title>Monitor de Corrente</title>";
  html += "<style>body{font-family: Arial; text-align: center;}</style></head><body>";
  html += "<h1>Monitor de Corrente</h1>";
  html += "<p><strong>Corrente RMS:</strong> " + String(Amps, 3) + " A</p>";
  html += "<p><strong>Potência Aparente:</strong> " + String(Amps * 127) + " WATTS</p>";
  html += "<p><strong>Endereço IP:</strong> " + ipAddress + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);               // Envia o HTML como resposta ao cliente
}

void Calcula_corrente() {
  int RawValue = analogRead(analogIn);               // Lê o valor do pino analógico
  Voltage = (RawValue / 4095.0) * 3300;              // Converte o valor bruto para milivolts
  Amps = (((Voltage - ACSoffset) / mVperAmp) / 10 + 0.850); // Calcula a corrente em amperes

  // Ajuste para valores negativos
  if (Amps < 0) {
    Amps = 0;
  }

  // Controla o estado do relé com base na corrente medida
  if (Amps < 0.100) {                                // Se corrente for menor que 0.1A
    digitalWrite(pinoSaida, HIGH);                   // Liga o relé
  } else {
    digitalWrite(pinoSaida, LOW);                    // Desliga o relé
  }

  Serial.print("Corrente (A): ");
  Serial.println(Amps, 3);                           // Exibe a corrente no monitor serial
}

void exibeDisplay() {
  // Função que exibe os dados no display OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 0);
  display.print("Corrente (A)");
  display.setTextSize(2);
  display.setCursor(20, 30);
  display.print(Amps, 3);                            // Mostra a corrente com 3 casas decimais
  display.display();
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {                  // Tenta reconectar ao broker MQTT
    Serial.print("Tentando conectar ao MQTT...");
    if (mqttClient.connect("ESP32Client")) {         // Conecta usando o nome "ESP32Client"
      Serial.println("conectado");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 2 segundos");
      delay(2000);                                   // Espera 2 segundos antes de tentar novamente
    }
  }
}
