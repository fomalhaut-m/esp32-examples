#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================
// WiFi 配置
// ============================================================
const char* WIFI_SSID = "fomalhaut-luke";
const char* WIFI_PASSWORD = "fomalhaut-luke2019";

// ============================================================
// MQTT 配置
// ============================================================
const char* MQTT_SERVER = "192.168.31.190";
const int MQTT_PORT = 11883;
const char* MQTT_USER = "mqtt_user";
const char* MQTT_PASSWORD = "mqtt_password";
const char* MQTT_CLIENT_NAME = "esp32_led_client";

// ============================================================
// LED 状态上报主题 (Reporter 模式)
// ============================================================
const char* LED_TOPIC = "device/esp32/led/reporter";

// ============================================================
// 硬件引脚定义
// ============================================================
#define RED_LED_PIN       48
#define YELLOW_LED_PIN     2
#define GREEN_LED_PIN      1

#define RED_SWITCH_PIN    10
#define YELLOW_SWITCH_PIN 11
#define GREEN_SWITCH_PIN  12

// ============================================================
// 全局变量
// ============================================================
WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool lastRedState = false;
bool lastYellowState = false;
bool lastGreenState = false;

// ============================================================
// 初始化函数
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("===========================================");
  Serial.println("ESP32 LED MQTT Reporter 初始化中...");
  Serial.println("===========================================");

  // 配置 LED 引脚为输出模式
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // 配置开关引脚为输入模式（启用内部上拉电阻）
  pinMode(RED_SWITCH_PIN, INPUT_PULLUP);
  pinMode(YELLOW_SWITCH_PIN, INPUT_PULLUP);
  pinMode(GREEN_SWITCH_PIN, INPUT_PULLUP);

  // 初始状态：所有 LED 关闭
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  Serial.println("[INIT] LED 和 开关引脚配置完成");

  // 连接 WiFi
  setup_wifi();

  // 配置 MQTT 服务器
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  Serial.println("[INIT] MQTT 服务器配置完成");
  Serial.println("===========================================");
}

// ============================================================
// WiFi 连接函数
// ============================================================
void setup_wifi() {
  Serial.print("[WIFI] 正在连接 SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;

    if (attempts % 10 == 0) {
      Serial.printf("\n[WIFI] 状态: %d, 信号强度: %d dBm\n", WiFi.status(), WiFi.RSSI());
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] WiFi 连接成功!");
    Serial.print("[WIFI] IP 地址: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WIFI] 信号强度: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n[WIFI] WiFi 连接失败!");
    Serial.print("[WIFI] 错误码: ");
    Serial.println(WiFi.status());
    Serial.println("[WIFI] 请检查 SSID 和密码是否正确");
  }
}

// ============================================================
// MQTT 重连函数
// ============================================================
void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] 正在连接 MQTT 服务器...");
    Serial.print(MQTT_SERVER);
    Serial.print(":");
    Serial.println(MQTT_PORT);

    if (mqttClient.connect(MQTT_CLIENT_NAME, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("[MQTT] MQTT 连接成功!");
      Serial.print("[MQTT] 客户端名称: ");
      Serial.println(MQTT_CLIENT_NAME);
      Serial.print("[MQTT] 上报主题: ");
      Serial.println(LED_TOPIC);
    } else {
      Serial.print("[MQTT] 连接失败, rc=");
      Serial.print(mqttClient.state());
      Serial.println(", 5秒后重试...");
      delay(5000);
    }
  }
}

// ============================================================
// 发布 LED 状态 (仅状态变化时发送)
// ============================================================
void publishLedStatus() {
  bool redState = (digitalRead(RED_SWITCH_PIN) == LOW);
  bool yellowState = (digitalRead(YELLOW_SWITCH_PIN) == LOW);
  bool greenState = (digitalRead(GREEN_SWITCH_PIN) == LOW);

  // 检测状态是否有变化
  if (redState != lastRedState || yellowState != lastYellowState || greenState != lastGreenState) {

    // 构建 JSON 格式的 payload
    String payload = "{";
    payload += "\"red\":";
    payload += redState ? "true" : "false";
    payload += ",\"yellow\":";
    payload += yellowState ? "true" : "false";
    payload += ",\"green\":";
    payload += greenState ? "true" : "false";
    payload += "}";

    // 发布到 MQTT 主题
    bool success = mqttClient.publish(LED_TOPIC, (const uint8_t*)payload.c_str(), payload.length());

    // 打印日志
    Serial.println("-------------------------------------------");
    Serial.println("[MQTT] LED 状态变化，已上报:");
    Serial.print("  - 主题: ");
    Serial.println(LED_TOPIC);
    Serial.print("  - Payload: ");
    Serial.println(payload);
    Serial.print("  - 发布结果: ");
    Serial.println(success ? "成功" : "失败");
    Serial.printf("  - 红色 LED: %s\n", redState ? "亮" : "灭");
    Serial.printf("  - 黄色 LED: %s\n", yellowState ? "亮" : "灭");
    Serial.printf("  - 绿色 LED: %s\n", greenState ? "亮" : "灭");
    Serial.println("-------------------------------------------");

    // 更新状态缓存
    lastRedState = redState;
    lastYellowState = yellowState;
    lastGreenState = greenState;
  }
}

// ============================================================
// 主循环
// ============================================================
void loop() {
  // 检查 MQTT 连接状态
  if (!mqttClient.connected()) {
    Serial.println("[LOOP] MQTT 连接已断开，正在重连...");
    reconnect_mqtt();
  }

  // 处理 MQTT 客户端事件
  mqttClient.loop();

  // 检查灯的状态并设置
  digitalWrite(RED_LED_PIN, lastRedState? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, lastYellowState? HIGH : LOW);
  digitalWrite(GREEN_LED_PIN, lastGreenState? HIGH : LOW);

  // 检查 LED 状态并发布
  publishLedStatus();

  // 50ms 采样间隔
  delay(50);
}
