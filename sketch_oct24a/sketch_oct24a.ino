#include <Wire.h>              // Biblioteca para comunicação I2C, necessária para controlar o display OLED.
#include <Adafruit_GFX.h>      // Biblioteca para suporte gráfico em displays, usada em conjunto com o display OLED.
#include <Adafruit_SSD1306.h>  // Biblioteca específica para controle do display OLED modelo SSD1306.
#include <WiFi.h>              // Biblioteca para permitir a conexão do ESP32 a redes Wi-Fi.
#include <WebServer.h>         // Biblioteca para criar e gerenciar um servidor HTTP.
#include <PubSubClient.h>      // Biblioteca para comunicação com um broker MQTT, permitindo a publicação e assinatura de tópicos.

// Define as dimensões do display OLED em pixels.
#define SCREEN_WIDTH 128       // Largura do display (128 pixels).
#define SCREEN_HEIGHT 64       // Altura do display (64 pixels).
#define OLED_RESET -1          // Define o pino de reset do display (não é usado com o ESP32).
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Cria um objeto `display` para o OLED com as configurações definidas.

// Definições de pinos e variáveis para o sensor de corrente ACS712.
const int analogIn = 36;       // Pino analógico VP (GPIO36) utilizado para a leitura do sinal do sensor de corrente ACS712.
const int pinoSaida = 5;       // Pino digital 5 que será acionado com base na leitura da corrente.
int mVperAmp = 66;             // Sensibilidade do sensor ACS712 em mV por Ampere (66 mV/A para o modelo de 30A).
int ACSoffset = 2500;          // Offset de calibração do sensor, em mV, usado para medir correntes AC.
double Voltage = 0;            // Variável para armazenar a tensão medida no pino analógico.
double Amps = 0;               // Variável para armazenar a corrente medida, em amperes.

// Credenciais da rede Wi-Fi (nome e senha).
const char* ssid = "Altatelas2G";      // Nome (SSID) da rede Wi-Fi à qual o ESP32 se conectará.
const char* password = "alta972600";   // Senha da rede Wi-Fi.
const char* mqttServer = "192.168.18.82"; // Endereço IP do broker MQTT para conexão.
const int mqttPort = 1883;             // Porta usada pelo broker MQTT (1883 é a porta padrão).

String ipAddress;                      // Variável para armazenar o endereço IP do ESP32 após conexão à rede.
WiFiClient wifiClient;                 // Cliente Wi-Fi para permitir a comunicação com o broker MQTT.
PubSubClient mqttClient(wifiClient);   // Cliente MQTT configurado usando o cliente Wi-Fi.
WebServer server(80);                  // Criação de um servidor HTTP na porta 80 para receber requisições.

// Configuração inicial do dispositivo.
void setup() {
  Serial.begin(115200);                     // Inicializa a porta serial com a taxa de 115200 baud para depuração.
  Serial.println("Inicializando...");       // Mensagem de depuração indicando início da configuração.

  // Inicializa o display OLED e verifica se foi bem-sucedida.
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Tenta iniciar o display na comunicação I2C (endereço 0x3C).
    Serial.println("Falha ao inicializar o display OLED"); // Mensagem de erro caso o display não inicialize.
    while (true);                                    // Entra em um loop infinito para interromper o programa em caso de falha.
  }
  display.clearDisplay();                            // Limpa o conteúdo do display.
  display.setTextColor(SSD1306_WHITE);               // Configura a cor do texto para branco.

  // Configuração do pino digital para controle do relé ou outro dispositivo.
  pinMode(pinoSaida, OUTPUT);                        // Define o pino de saída como saída digital.
  digitalWrite(pinoSaida, LOW);                      // Inicializa o pino na condição desligada (LOW).

  // Conexão à rede Wi-Fi.
  WiFi.begin(ssid, password);                        // Inicia a tentativa de conexão com a rede Wi-Fi.
  while (WiFi.status() != WL_CONNECTED) {            // Verifica continuamente se o ESP32 está conectado.
    delay(1000);                                     // Aguarda 1 segundo entre cada tentativa de verificação.
    Serial.println("Conectando ao WiFi...");         // Imprime no monitor serial uma mensagem de status de conexão.
  }
  ipAddress = WiFi.localIP().toString();             // Armazena o endereço IP do ESP32 como string.
  Serial.print("Conectado ao WiFi com IP: ");        // Imprime uma mensagem de sucesso na conexão.
  Serial.println(ipAddress);

  // Configuração do servidor HTTP.
  server.on("/", handleRoot);                        // Define que a função `handleRoot` será chamada quando houver requisições à raiz do servidor.
  server.begin();                                    // Inicia o servidor HTTP.

  // Configuração do cliente MQTT.
  mqttClient.setServer(mqttServer, mqttPort);        // Define o endereço do servidor MQTT e a porta.
}

// Função que é chamada continuamente para manter o funcionamento do dispositivo.
void loop() {
  server.handleClient();                             // Processa requisições de clientes conectados ao servidor HTTP.

  // Chama a função para calcular a corrente medida pelo sensor.
  Calcula_corrente();                                // Atualiza a variável `Amps` com o valor da corrente medida.
  exibeDisplay();                                    // Atualiza o display OLED com a leitura da corrente.

  // Verifica se o cliente MQTT está conectado, caso contrário, tenta reconectar.
  if (!mqttClient.connected()) {
    reconnectMQTT();                                 // Chama a função para tentar a reconexão ao broker MQTT.
  }
  mqttClient.loop();                                 // Mantém a conexão com o broker MQTT ativa e processa mensagens.

  // Prepara e publica as leituras de corrente e potência nos tópicos MQTT.
  String ipAddressStr = String(ipAddress);           // Converte o IP para uma string.
  String correnteStr = String(Amps, 3);              // Converte a corrente medida para uma string com 3 casas decimais.
  String potenciaStr = String(Amps * 127);           // Calcula a potência (assumindo 127V) e converte para string.

  // Publica os valores de corrente, potência e IP nos respectivos tópicos do broker MQTT.
  mqttClient.publish("Casa/Monitoramento_Energia/corrente", correnteStr.c_str());
  mqttClient.publish("Casa/Monitoramento_Energia/potencia", potenciaStr.c_str());
  mqttClient.publish("Casa/Monitoramento_Energia/Ipv4 rede local do esp32", ipAddressStr.c_str());

  // Publica o estado do relé com base na corrente medida.
  if (Amps == 0) {
    mqttClient.publish("Casa/Monitoramento_Energia/rele", "Desligado"); // Indica que o relé está desligado.
  } else {
    mqttClient.publish("Casa/Monitoramento_Energia/rele", "Ligado");    // Indica que o relé está ligado.
  }

  delay(5000);                                       // Aguarda 5 segundos antes da próxima execução do loop.
}

// Função que gera e envia a página HTML da raiz do servidor HTTP.
void handleRoot() {
  String html = "<html><head><title>Monitor de Corrente</title>"; // Cabeçalho HTML com título da página.
  html += "<style>body{font-family: Arial; text-align: center;}</style></head><body>"; // Estiliza a página.
  html += "<h1>Monitor de Corrente</h1>";                      // Título da página.
  html += "<p><strong>Corrente RMS:</strong> " + String(Amps, 3) + " A</p>"; // Mostra a corrente medida.
  html += "<p><strong>Potência Aparente:</strong> " + String(Amps * 127) + " WATTS</p>"; // Mostra a potência.
  html += "<p><strong>Endereço IP:</strong> " + ipAddress + "</p>"; // Mostra o endereço IP do ESP32.
  html += "</body></html>";
  server.send(200, "text/html", html);               // Envia a página HTML ao cliente que fez a requisição.
}

// Função que realiza a leitura do sensor de corrente e calcula a corrente em amperes.
void Calcula_corrente() {
  int RawValue = analogRead(analogIn);               // Lê o valor bruto do pino analógico (0 a 4095).
  Voltage = (RawValue / 4095.0) * 3300;              // Converte o valor bruto para milivolts (0 a 3300 mV).
  Amps = (((Voltage - ACSoffset) / mVperAmp) / 10 + 0.850); // Calcula a corrente em amperes com compensação.

  // Verifica se a corrente é negativa e corrige para zero.
  if (Amps < 0) {
    Amps = 0;                                        // Ajusta a corrente para zero se for negativa.
  }
}

// Função que exibe as informações no display OLED.
void exibeDisplay() {
  display.clearDisplay();                            // Limpa o display antes de escrever novas informações.
  display.setTextSize(1);                            // Define o tamanho do texto para 1.
  display.setCursor(0, 0);                           // Posiciona o cursor na posição (0, 0).
  display.print("Corrente: ");                       // Exibe o texto "Corrente: ".
  display.println(Amps, 3);                          // Exibe o valor da corrente com 3 casas decimais.
  display.print("Potencia: ");                       // Exibe o texto "Potencia: ".
  display.println(Amps * 127);                       // Exibe o valor da potência.
  display.display();                                 // Atualiza o display com as informações.
}

// Função para reconectar ao broker MQTT.
void reconnectMQTT() {
  while (!mqttClient.connected()) {                  // Verifica se o cliente MQTT está desconectado.
    Serial.println("Tentando reconectar ao broker MQTT..."); // Imprime mensagem de tentativa de reconexão.
    if (mqttClient.connect("ESP32Monitor")) {        // Tenta conectar ao broker com o nome "ESP32Monitor".
      Serial.println("Conectado ao broker MQTT!");   // Mensagem de sucesso na conexão.
    } else {
      Serial.print("Falha na conexao, erro: ");      // Mensagem de erro com o código de falha.
      Serial.println(mqttClient.state());            // Exibe o código do estado do cliente MQTT.
      delay(5000);                                   // Aguarda 5 segundos antes de tentar novamente.
    }
  }
}
