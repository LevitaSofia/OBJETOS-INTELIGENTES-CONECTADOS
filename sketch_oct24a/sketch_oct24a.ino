#include <Wire.h>              // Biblioteca para comunicação I2C, necessária para o display OLED.
#include <Adafruit_GFX.h>      // Biblioteca para gráficos em displays, usada para criar textos e desenhos.
#include <Adafruit_SSD1306.h>  // Biblioteca para controle do display OLED SSD1306, fornecendo métodos específicos.
#include <WiFi.h>              // Biblioteca para estabelecer a conexão Wi-Fi com redes locais.
#include <WebServer.h>         // Biblioteca para criar e gerenciar um servidor HTTP.
#include <PubSubClient.h>      // Biblioteca para comunicação com o broker MQTT, essencial para a transmissão de dados.

#define SCREEN_WIDTH 128       // Define a largura do display OLED em pixels.
#define SCREEN_HEIGHT 64       // Define a altura do display OLED em pixels.
#define OLED_RESET -1          // Define o pino de reset do OLED, -1 indica que o pino não será usado no ESP32.
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Inicializa o display com os parâmetros definidos.

const int analogIn = 36;       // Define o pino VP (GPIO36) como entrada para o sensor ACS712.
int mVperAmp = 66;             // Sensibilidade do sensor ACS712 em mV (66 para o modelo de 30A).
int ACSoffset = 2500;          // Offset padrão do sensor em mV, para calibração.
double Voltage = 0;            // Variável para armazenar a voltagem lida.
double Amps = 0;               // Variável para armazenar a corrente calculada em amperes.

const char* ssid = "Altatelas2G";            // Nome da rede Wi-Fi.
const char* password = "alta972600";         // Senha da rede Wi-Fi.
const char* mqttServer = "broker.mqtt-dashboard.com"; // Endereço do broker MQTT.
const int mqttPort = 1883;                   // Porta padrão para conexão com o broker MQTT.

String ipAddress;                            // Armazena o endereço IP obtido ao conectar ao Wi-Fi.
WiFiClient wifiClient;                       // Instancia o cliente Wi-Fi para o ESP32.
PubSubClient mqttClient(wifiClient);         // Cliente MQTT, configurado com o cliente Wi-Fi.
WebServer server(80);                        // Inicializa o servidor HTTP na porta 80.

void setup() {
  Serial.begin(115200);                      // Inicia a comunicação serial a 115200 bps.
  Serial.println("Inicializando...");        // Mensagem de inicialização no monitor serial.

  // Inicialização do display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("Falha ao inicializar o display OLED"); // Mensagem de erro se o display não iniciar.
    while (true);                                          // Loop infinito se houver erro.
  }
  display.clearDisplay();                                  // Limpa o display.
  display.setTextColor(SSD1306_WHITE);                     // Define a cor do texto como branca.

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);                              // Conecta ao Wi-Fi usando o SSID e a senha definidos.
  while (WiFi.status() != WL_CONNECTED) {                  // Loop até conectar ao Wi-Fi.
    delay(1000);                                           // Atraso de 1 segundo entre tentativas.
    Serial.println("Conectando ao WiFi...");               // Mensagem de tentativa de conexão.
  }
  ipAddress = WiFi.localIP().toString();                   // Armazena o IP obtido na variável.
  Serial.print("Conectado ao WiFi com IP: ");              // Exibe o IP no monitor serial.
  Serial.println(ipAddress);

  // Configuração do servidor HTTP
  server.on("/", handleRoot);                              // Define a função handleRoot para responder à rota principal.
  server.begin();                                          // Inicia o servidor.

  // Configuração do cliente MQTT
  mqttClient.setServer(mqttServer, mqttPort);              // Configura o endereço e porta do broker MQTT.
}

void loop() {
  // Processa clientes HTTP
  server.handleClient();                                   // Trata solicitações HTTP recebidas.

  // Calcula corrente e atualiza o display
  Calcula_corrente();                                      // Função para cálculo da corrente.
  exibeDisplay();                                          // Exibe a corrente no display OLED.

  // Verifica conexão MQTT
  if (!mqttClient.connected()) {                           // Verifica se há conexão com o broker MQTT.
    reconnectMQTT();                                       // Se não estiver conectado, tenta reconectar.
  }
  mqttClient.loop();                                       // Mantém a conexão MQTT ativa.

  // Publica corrente e potência
  String correnteStr = String(Amps, 3);                    // Converte a corrente para string com 3 casas decimais.
  String potenciaStr = String(Amps * 127);                 // Calcula a potência aparente com 127V e converte para string.
  mqttClient.publish("casa/monitoramento/corrente", correnteStr.c_str()); // Publica a corrente no tópico MQTT.
  mqttClient.publish("casa/monitoramento/potencia", potenciaStr.c_str()); // Publica a potência no tópico MQTT.

  delay(5000);                                             // Atraso de 5 segundos entre leituras e publicações.
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
  server.send(200, "text/html", html);                     // Envia a resposta em HTML para o cliente.
}

void Calcula_corrente() {
  int RawValue = analogRead(analogIn);                     // Lê o valor do pino analógico VP.
  Voltage = (RawValue / 4095.0) * 3300;                    // Converte o valor lido para mV.
  Amps = (((Voltage - ACSoffset) / mVperAmp) / 10 + 0.950); // Calcula a corrente em A com ajuste.

  // Ajuste para mostrar apenas valores positivos
  if (Amps < 0) {                                          // Se a corrente for negativa,
    Amps = 0;                                              // define como zero para evitar valores negativos.
  }

  Serial.print("Corrente (A): ");                          // Exibe a corrente no monitor serial.
  Serial.println(Amps, 3);
}

void exibeDisplay() {
  // Exibe os dados no display OLED
  display.clearDisplay();                                  // Limpa o display antes de escrever novos dados.
  display.setTextSize(1);                                  // Define o tamanho do texto.
  display.setCursor(20, 0);                                // Define a posição do cursor no display.
  display.print("Corrente (A)");                           // Exibe a legenda.
  display.setTextSize(2);                                  // Aumenta o tamanho do texto para o valor da corrente.
  display.setCursor(20, 30);                               // Posição do valor da corrente.
  display.print(Amps, 3);                                  // Exibe o valor da corrente com 3 casas decimais.
  display.display();                                       // Atualiza o display com as informações.
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {                        // Enquanto não estiver conectado ao MQTT,
    Serial.print("Tentando conectar ao MQTT...");          // Exibe mensagem de tentativa de conexão.
    if (mqttClient.connect("ESP32Client")) {               // Tenta conectar ao broker com o ID "ESP32Client".
      Serial.println("conectado");                         // Exibe confirmação de conexão.
    } else {
      Serial.print("Falha, rc=");                          // Exibe mensagem de falha de conexão.
      Serial.print(mqttClient.state());                    // Exibe o estado de erro do cliente MQTT.
      Serial.println(" Tentando novamente em 2 segundos");
      delay(2000);                                         // Espera 2 segundos antes de tentar novamente.
    }
  }
}
