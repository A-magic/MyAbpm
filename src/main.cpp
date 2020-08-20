#include <Arduino.h>
#include <WiFi.h> //esp32
#include <WiFiClient.h>
#include "HardwareSerial.h"
#include "BPSensor.h"
#include "DrawOled.h"

// Enable MqttClient logs
#define MQTT_LOG_ENABLED 1
// Include library
#include <MqttClient.h>



#define LOG_PRINTFLN(fmt, ...) logfln(fmt, ##__VA_ARGS__)
#define LOG_SIZE_MAX 128
void logfln(const char *fmt, ...)
{
  char buf[LOG_SIZE_MAX];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, LOG_SIZE_MAX, fmt, ap);
  va_end(ap);
  Serial.println(buf);
}

#define HW_UART_SPEED 115200L //定义serail波特率，弃用已由heltec直接默认开启

char SSID[] = "Ho";           //  your network SSID (name)
char WIFIPASS[] = "88888888"; // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;
//onenet server
#define MQTT_SERVER "183.230.40.39" //MQTTSERVER IP
#define MQTT_PORT 6002              //MQTTSERVER PORT

#define MQTT_USERNAME "208889" //productID
#define MQTT_PASSWORD "1"      //AUTHINFO OR APIKEY
#define MQTT_ID "516152057"    //DEVICEid

#define MQTT_TOPIC_PUB "$dp"           //MQTT_TOPIC_PUB
#define MQTT_TOPIC_SUB "/516152057/HP" //MQTT_TOPIC_SUB

static MqttClient *mqtt = NULL;
static WiFiClient network;

// ============== Object to supply system functions ============================
class System : public MqttClient::System
{
  public:
    unsigned long millis() const
    {
      return ::millis();
    }
    void yield(void)
    {
      ::yield();
    }
};

// ============== Setup all objects ============================================
void setup()
{
  // Setup hardware serial for logging
  // Serial.begin(HW_UART_SPEED);
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/); //begin serial 115200
  Heltec.display->setContrast(255);
  //血压探测器串口定义

  mySerial.begin(115200, SERIAL_8N1, 14, 27); //rx tx


  bp_check_order(Readmv); //向血压探测器发送擦除原校准信息命令


  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(SSID, WIFIPASS);
    Serial.print("status:");
    Serial.println(status);
    // wait 10 seconds for connection:
    delay(5000);
  }
  Serial.println("Connected to wifi");

  IPAddress myIP = WiFi.localIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  LOG_PRINTFLN("\n");
  LOG_PRINTFLN("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    LOG_PRINTFLN(".");
  }
  LOG_PRINTFLN("Connected to WiFi");
  LOG_PRINTFLN("IP: %s", myIP);

  // Setup MqttClient
  MqttClient::System *mqttSystem = new System;
  MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
  MqttClient::Network *mqttNetwork = new MqttClient::NetworkClientImpl<WiFiClient>(network, *mqttSystem);
  //// Make 128 bytes send buffer
  MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<128>();
  //// Make 128 bytes receive buffer
  MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<128>();
  //// Allow up to 2 subscriptions simultaneously
  MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersImpl<2>();
  //// Configure client options
  MqttClient::Options mqttOptions;
  ////// Set command timeout to 10 seconds
  mqttOptions.commandTimeoutMs = 10000;
  //// Make client object
  mqtt = new MqttClient(
    mqttOptions, *mqttLogger, *mqttSystem, *mqttNetwork, *mqttSendBuffer,
    *mqttRecvBuffer, *mqttMessageHandlers);
}

// ============== Subscription callback ========================================
void processMessage(MqttClient::MessageData &md)
{
  const MqttClient::Message &msg = md.message;
  char payload[msg.payloadLen + 1];
  memcpy(payload, msg.payload, msg.payloadLen);
  payload[msg.payloadLen] = '\0';
  LOG_PRINTFLN(
    "Message arrived: qos %d, retained %d, dup %d, packetid %d, payload:[%s]",
    msg.qos, msg.retained, msg.dup, msg.id, payload);
}

// ============== Main loop ====================================================
void loop()
{
  // Check connection status
  if (!mqtt->isConnected())
  {
    // Close connection if exists
    network.stop();
    // Re-establish TCP connection with MQTT broker
    LOG_PRINTFLN("Connecting");
    network.connect(MQTT_SERVER, MQTT_PORT);
    if (!network.connected())
    {
      LOG_PRINTFLN("Can't establish the TCP connection");
      delay(5000);
      ESP.restart();
    }
    // Start new MQTT connection
    MqttClient::ConnectResult connectResult;
    // Connect
    {
      MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
      options.MQTTVersion = 4;
      options.clientID.cstring = (char *)MQTT_ID;
      options.cleansession = true;
      options.keepAliveInterval = 15; // 15 seconds
      options.username.cstring = (char *)MQTT_USERNAME;
      options.password.cstring = (char *)MQTT_PASSWORD;
      MqttClient::Error::type rc = mqtt->connect(options, connectResult);
      if (rc != MqttClient::Error::SUCCESS)
      {
        LOG_PRINTFLN("Connection error: % i", rc);
        return;
      }
    }
    {
      // Add subscribe here if required
      MqttClient::Error::type rc = mqtt->subscribe(
                                     (char *)MQTT_TOPIC_SUB, MqttClient::QOS0, processMessage);
      if (rc != MqttClient::Error::SUCCESS)
      {
        LOG_PRINTFLN("Subscribe error: % i", rc);
        LOG_PRINTFLN("Drop connection");
        mqtt->disconnect();
        return;
      }
    }
  }
  else
  {
    {
      // Add publish here if required

      char json[] = " {\"HP\":\"%d\",\"LP\":\"%d\",\"HR\":\"%d\"}";
      char t_json[255];
      int payload_len;
      char *t_payload;
      unsigned short json_len;

      bp_check_order(readco);
      sprintf(t_json, json, int(SensorData[1]), int(SensorData[2]), int(SensorData[3]));
      
      drawFontFaceDemo(int(SensorData[1]), int(SensorData[2]), int(SensorData[3]));
      Serial.println(t_json);



      payload_len = 1 + 2 + strlen(t_json) / sizeof(char);
      json_len = strlen(t_json) / sizeof(char);

      t_payload = (char *)malloc(payload_len);
      if (t_payload == NULL)
      {
        printf("<%s>: t_payload malloc error\r\n", __FUNCTION__);
      }
      //type
      t_payload[0] = '\x03';
      //length
      t_payload[1] = (json_len & 0xFF00) >> 8;
      t_payload[2] = json_len & 0xFF;
      //json
      memcpy(t_payload + 3, t_json, json_len);

      MqttClient::Message message;
      message.qos = MqttClient::QOS0;
      message.retained = false;
      message.dup = false;
      message.payload = (void *)t_payload;
      message.payloadLen = payload_len;
      mqtt->publish((char *)MQTT_TOPIC_PUB, message);
    }
    // Idle for 30 seconds
    mqtt->yield(5000L);
  }
}