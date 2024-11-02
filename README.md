<p align="center">
  <a href="https://www.mackenzie.br/" target="blank"><img src="./mackenzie-logo.png" width="200" alt="Mackenzie Logo" /></a>
</p>

<div align="center">

<h1>Monitoramento de Energia Residencial com ESP32</h1>

<p><strong>Descrição do Projeto</strong></p>

<p>Este projeto foi desenvolvido para monitorar o consumo de energia em tempo real utilizando o microcontrolador ESP32 junto com um sensor de corrente ACS712, um display OLED SSD1306 e comunicação com um broker MQTT. O sistema apresenta os dados de corrente e potência em um servidor web local, publica os valores em tópicos MQTT e controla um relé com base na corrente medida.</p>

</div>

## Funcionalidades

- **Conexão Wi-Fi**: O ESP32 conecta-se à rede Wi-Fi local para hospedar um servidor HTTP e se comunicar com um broker MQTT.
- **Monitoramento de Corrente**: Leitura da corrente elétrica em um dispositivo utilizando o sensor ACS712 e exibição dos valores em um display OLED.
- **Publicação MQTT**: Publica os valores de corrente, potência e estado do relé em tópicos MQTT para monitoramento remoto.
- **Interface Web**: Apresenta uma página web que mostra a corrente, potência e o endereço IP do ESP32.
- **Controle de Relé**: Aciona ou desliga um relé com base na corrente medida.

## Tecnologias e Bibliotecas Utilizadas

- **ESP32**: Microcontrolador principal do projeto.
- **ACS712**: Sensor de corrente utilizado para medir a corrente elétrica.
- **Display OLED SSD1306**: Exibição dos valores de corrente.
- **Bibliotecas Arduino**:
  - `Wire.h`: Comunicação I2C.
  - `Adafruit_GFX.h` e `Adafruit_SSD1306.h`: Controle do display OLED.
  - `WiFi.h`: Conexão Wi-Fi.
  - `WebServer.h`: Servidor HTTP.
  - `PubSubClient.h`: Cliente MQTT.
- **Protocolo MQTT**: Utilizado para comunicação de dados com um broker para monitoramento remoto.

## Como Funciona

### Inicialização

- O ESP32 conecta-se à rede Wi-Fi configurada e inicializa o display OLED.
- Um servidor HTTP é iniciado para fornecer uma interface web com as leituras de corrente e potência.
- O cliente MQTT é configurado e tenta se conectar ao broker especificado.

### Leitura e Cálculo de Corrente

- O valor de corrente é lido do pino analógico onde o ACS712 está conectado.
- O código converte a leitura para tensão, ajusta o offset e calcula a corrente em amperes.
- Um relé é acionado ou desligado com base na corrente medida (corrente abaixo de 0,1A liga o relé).

### Publicação MQTT e Interface Web

- O ESP32 publica a corrente, potência e estado do relé em tópicos MQTT específicos.
- Uma página web é gerada com as informações atuais de corrente e potência.
- As atualizações são feitas a cada 5 segundos.

## Configurações Necessárias
**SERVIDOR MQTT**
```cpp
const char* mqttServer = "IP DO SERVIDOR MQTT";
const int mqttPort = PORTA DO SERVIDOR MQTT;
```

**Credenciais de Wi-Fi**
```cpp
const char* ssid = "Sua rede";
const char* password = "Sua senha";
```


###
**Pinos Utilizados**
Entrada Analógica: GPIO 36 (VP) para o sensor ACS712.
Saída Digital: GPIO 5 para controle do relé.

### Como Usar:
Clone este repositório no seu ambiente de desenvolvimento Arduino.

**Carregue o código no ESP32.**

-Abra o monitor serial (baud rate: 115200) para verificar as mensagens de depuração.
-Acesse o endereço IP exibido no monitor serial para visualizar a interface web no navegador.
-Monitore os dados nos tópicos MQTT configurados.

### 
Exemplo de Publicação MQTT

-Tópico de Corrente: Casa/Monitoramento_Energia/corrente
-Tópico de Potência: Casa/Monitoramento_Energia/potencia
-Tópico do Estado do Relé: Casa/Monitoramento_Energia/rele




