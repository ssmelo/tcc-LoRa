#include <Arduino.h>
#include "LoraMesher.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <mbedtls/md.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "WiFiClientSecure.h"
#include <AES.h>

#define LoRa_MOSI 10
#define LoRa_MISO 11
#define LoRa_SCK 9

#define LoRa_nss 8
#define LoRa_dio1 14
#define LoRa_nrst 12
#define LoRa_busy 13

#define OLED_RESET 21 
#define OLED_SDA 17
#define OLED_SCL 18

#define FLOW_TOPIC "/UFMG/Pampulha/sprinkler/vazao"

// Wifi
const char* ssid = "rede"; // rede
const char* password = "123456"; // senha

const char* CA_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"-----END CERTIFICATE-----";

const char* ESP_CA_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"-----END CERTIFICATE-----";

const char* ESP_RSA_key= \
"-----BEGIN PRIVATE KEY-----\n" \
"-----END PRIVATE KEY-----";

// Definições do servidor MQTT
const char* mqtt_server = "test.mosquitto.org"; // Servidor mqtt
const int mqtt_port = 8884; // porta do servidor
volatile bool flow_message_flag = false;
const int combinedMessageSize = 80; // Ajuste o tamanho conforme necessário
char combinedMessageChar[combinedMessageSize];

WiFiClientSecure client;
PubSubClient mqtt_client(client);

AES256 aes;

// Constantes para tempo de reconexão Wifi
unsigned long previousMillis = 0;
unsigned long interval = 30000;

// Parametros do display
U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA, /* reset=*/ OLED_RESET);   // All Boards without Reset of the Display

SPIClass customSPI(HSPI);
LoraMesher::LoraMesherConfig mesher;
LoraMesher &radio = LoraMesher::getInstance();

char key[] = "sua-chave-secreta"; // Recomendado uma chave de 32 bits

struct dataPacket {
  char message[80];
};
// Pacote para enviar dados
dataPacket* helloPacket = new dataPacket;

long timezone = -3;
byte daysavetime = 1;
int hour_to_sleep = 23;

// Inicialização do Wifi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

// Inicialização do MQTT
void connect_mqtt() {
  String client_id = "esp32-client-";
  client_id += String(WiFi.macAddress());
  Serial.printf("Trying to connect %s to the mqtt broker\n", client_id.c_str());

  if (mqtt_client.connect(client_id.c_str())) {
    Serial.println("Connected to the broker");
  } else {
    Serial.print("Failed to connect: ");
    Serial.print(mqtt_client.state());
  }
}

// Callback para mensagens recebidas no broker MQTT
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  if (strcmp(topic, FLOW_TOPIC) == 0)
  {
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
      message += (char)payload[i];
    }

    Serial.print("Mensagem: ");
    Serial.println(message);

    byte encrypted[16];

    aes.encryptBlock((uint8_t*) combinedMessageChar, (uint8_t*) message.c_str());

    flow_message_flag = true;
  }

  Serial.println();
  Serial.println("-----------------------");
}

void processReceivedPackets(void *)
{
  for (;;)
  {
    /* Wait for the notification of processReceivedPackets and enter blocking */
    ulTaskNotifyTake(pdPASS, portMAX_DELAY);

    // Iterate through all the packets inside the Received User Packets Queue
    while (radio.getReceivedQueueSize() > 0)
    {
      Serial.println("ReceivedUserData_TaskHandle notify received");
      Serial.printf("Queue receiveUserData size: %d\n", radio.getReceivedQueueSize());

      // Get the first element inside the Received User Packets Queue
      AppPacket<String> *packet = radio.getNextAppPacket<String>();

      char textToWrite[ 16 ];
      // as per comment from LS_dev, platform is int 16bits
      display.clearBuffer();
      display.drawStr(0, 10, "Receiving");
		  display.drawStr(0, 20, "ALO");
		  display.sendBuffer();

      // Delete the packet when used. It is very important to call this function to release the memory of the packet.
      radio.deletePacket(packet);
    }
  }
}

TaskHandle_t receiveLoRaMessage_Handle = NULL;

void createReceiveMessages()
{
  int res = xTaskCreate(
      processReceivedPackets,
      "Receive App Task",
      4096,
      (void *)1,
      2,
      &receiveLoRaMessage_Handle);
  if (res != pdPASS)
  {
    Serial.printf("Error: Receive App Task creation gave error: %d\n", res);
  }

  radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle);
}

void setup_WiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  delay(4000);
}

void setupLoraMesher()
{
  customSPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_nss);

  mesher.module = LoraMesher::LoraModules::SX1262_MOD;
  mesher.loraIo1 = 13;
  mesher.loraRst = 12;
  mesher.loraIrq = 14;
  mesher.loraCs = 8;
  mesher.spi = &customSPI;
  mesher.freq = 915.0F;
  mesher.bw = 125.0F;
  mesher.sf = 6U;
  mesher.cr = 7U;
  mesher.syncWord = 0x26U;      // LoRa sync word. 0x34 is reserved for LoRaWAN networks.
  mesher.power = 2;
  mesher.preambleLength = 8U;
  // Inicializacão do radio com os parametros de rede
  radio.begin(mesher);

  // Criando a task para recebimento de mensagens do LoRaMesher
  createReceiveMessages();

  // Iniciando rede LoRa Mesh
  radio.start();

  Serial.println("Lora initialized");
}

void setup()
{
  // Desabilitando watchdog
  disableCore0WDT();
  disableCore1WDT();
  Serial.begin(115200);

  setup_WiFi();

  if(WiFi.status() == WL_CONNECTED) {
    configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  }

  aes = AES256();
  aes.setKey((uint8_t*) key, strlen(key));

  Serial.println("initBoard");

  display.begin();
	display.setFont(u8g2_font_NokiaSmallPlain_te );

  String message_to_draw = String("GATEWAY D - IP: ");
  message_to_draw.concat(String(radio.getLocalAddress(), HEX));

  display.clearBuffer();
  display.drawStr(0, 10, message_to_draw.c_str());
  display.sendBuffer();

  Serial.println("Local IP:");
  Serial.println(radio.getLocalAddress());

  initWiFi();

  // Setup dos certificados
  client.setCACert(CA_cert);          // Certificado da CA
  client.setCertificate(ESP_CA_cert); // Certificado do cliente
  client.setPrivateKey(ESP_RSA_key);  // Chave privada
  
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(callback);
  connect_mqtt();
  mqtt_client.subscribe(FLOW_TOPIC);
  setupLoraMesher();
}

void loop()
{
  for (;;)
  {
    unsigned long currentMillis = millis();
    // Reconexão do Wifi
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
      Serial.print(millis());
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      previousMillis = currentMillis;
    }

    struct tm tmstruct ;

    tmstruct.tm_year = 0;
    getLocalTime(&tmstruct);

    // Sleep mode
    // if(tmstruct.tm_hour >= hour_to_sleep) {
    //   Serial.println("Time:");
    //   Serial.println(tmstruct.tm_hour);
    //   esp_sleep_enable_timer_wakeup(8 * 60 * 60 * 1000000LL); // 1000000LL = 1 second --> 8 * 60 * 60 * 1000000LL = 8 HORAS
    //   esp_deep_sleep_start();
    // }

    if(flow_message_flag == true) {
      strncpy(helloPacket->message, combinedMessageChar, sizeof(combinedMessageChar) - 1);
      Serial.println("Send packet:");
      Serial.println(helloPacket->message);

      // Criando pacote e enviando
      radio.createPacketAndSend(0xFC40, helloPacket, 1);

      String message_to_draw = String("GATEWAY D - IP: ");
      message_to_draw.concat(String(radio.getLocalAddress(), HEX));

      display.clearBuffer();
      display.drawStr(0, 10, message_to_draw.c_str());
      display.drawStr(0, 20, "Enviado:");
      display.drawStr(0, 30, combinedMessageChar);
      display.sendBuffer();
      flow_message_flag = false;
    }

    mqtt_client.loop();
  }
}