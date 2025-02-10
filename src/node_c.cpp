// #include <Arduino.h>
// #include "LoraMesher.h"
// #include <Wire.h>
// #include <U8g2lib.h>
// #include <WiFi.h>

// #define BOARD_LED 4
// #define LED_ON LOW
// #define LED_OFF HIGH

// #define LoRa_MOSI 10
// #define LoRa_MISO 11
// #define LoRa_SCK 9

// #define LoRa_nss 8
// #define LoRa_dio1 14
// #define LoRa_nrst 12
// #define LoRa_busy 13

// #define OLED_RESET 21 
// #define OLED_SDA 17
// #define OLED_SCL 18

// U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA, /* reset=*/ OLED_RESET);   // All Boards without Reset of the Display

// SPIClass customSPI(HSPI);
// LoraMesher::LoraMesherConfig mesher;
// LoraMesher &radio = LoraMesher::getInstance();

// // Wifi
// const char* ssid = "rede"; // rede
// const char* password = "teste"; // senha

// String my_ip_message = String("NODE C - IP: ");

// struct dataPacket {
//   char message[80];
// };
// dataPacket* helloPacket = new dataPacket;

// long timezone = -3;
// byte daysavetime = 1;
// int hour_to_sleep = 23;

// void printDataPacket(AppPacket<uint8_t>* packet) {
//     //Get the payload to iterate through it
//     size_t payloadLength = packet->getPayloadLength();

//     for (size_t i = 0; i < payloadLength; i++) {
//       //Print the packet
//       Serial.println(packet->payload[i]);
//     }
// }

// void processReceivedPackets(void *)
// {
//   for (;;)
//   {
//     /* Wait for the notification of processReceivedPackets and enter blocking */
//     ulTaskNotifyTake(pdPASS, portMAX_DELAY);

//     // Iterate through all the packets inside the Received User Packets Queue
//     while (radio.getReceivedQueueSize() > 0)
//     {
//       Serial.println("ReceivedUserData_TaskHandle notify received");
//       Serial.printf("Queue receiveUserData size: %d\n", radio.getReceivedQueueSize());

//       // Get the first element inside the Received User Packets Queue
//       AppPacket<dataPacket>* packet = radio.getNextAppPacket<dataPacket>();

//       Serial.println("Message received:");
//       Serial.println(packet->payload->message);

//       display.clearBuffer();
//       display.drawStr(0, 10, "Receiving");
// 		  display.sendBuffer();

//       // Delete the packet when used. It is very important to call this function to release the memory of the packet.
//       radio.deletePacket(packet);
//     }
//   }
// }

// TaskHandle_t receiveLoRaMessage_Handle = NULL;

// void createReceiveMessages()
// {
//   int res = xTaskCreate(
//       processReceivedPackets,
//       "Receive App Task",
//       4096,
//       (void *)1,
//       2,
//       &receiveLoRaMessage_Handle);
//   if (res != pdPASS)
//   {
//     Serial.printf("Error: Receive App Task creation gave error: %d\n", res);
//   }

//   radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle);
// }

// void setup_WiFi()
// {
//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi ..");
//   delay(4000);
// }

// void setupLoraMesher()
// {
//   customSPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_nss);

//   mesher.module = LoraMesher::LoraModules::SX1262_MOD;
//   mesher.loraIo1 = 13;
//   mesher.loraRst = 12;
//   mesher.loraIrq = 14;
//   mesher.loraCs = 8;
//   mesher.spi = &customSPI;
//   mesher.freq = 915.0F;
//   mesher.bw = 125.0F;
//   mesher.sf = 6U;
//   mesher.cr = 7U;
//   mesher.syncWord = 0x26U;
//   mesher.power = 2;
//   mesher.preambleLength = 8U;
//   radio.begin(mesher);

//   createReceiveMessages();

//   radio.start();

//   Serial.println("Lora initialized");
// }

// void setup()
// {
//   Serial.begin(115200);

//   setup_WiFi();

//   my_ip_message.concat(String(radio.getLocalAddress(), HEX));

//   if(WiFi.status() == WL_CONNECTED) {
//     configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
//   }

//   Serial.println("initBoard");

//   display.begin();
// 	display.setFont(u8g2_font_NokiaSmallPlain_te );

//   display.clearBuffer();
//   display.drawStr(0, 10, "NODE C");
//   display.drawStr(0, 20, "Routing Table:");
//   display.sendBuffer();

//   Serial.println("Local IP:");
//   Serial.println(radio.getLocalAddress());

//   setupLoraMesher();
// }

// void loop()
// {
//   display.clearBuffer();
//   RoutingTableService::routingTableList->setInUse();

//   if (RoutingTableService::routingTableList->moveToStart()) {
//     size_t position = 0;

//     do {
//       RouteNode* node = RoutingTableService::routingTableList->getCurrent();
//       String address = String(node->networkNode.address, HEX);
//       address.concat(" via ");
//       address.concat(String(node->via, HEX));
//       address.concat(", metric ");
//       address.concat(String(node->networkNode.metric));
//       address.concat(", ");
//       address.concat(String(node->networkNode.role));
    
//       display.drawStr(0, 10, my_ip_message.c_str());
//       display.drawStr(0, 30, "Routing Table:");
//       display.drawStr(0, (position * 10) + 40, address.c_str());

//       position++;
//     } while (RoutingTableService::routingTableList->next());
//   }

//   display.sendBuffer();
//   RoutingTableService::routingTableList->releaseInUse();

//   vTaskDelay(5000);
// }