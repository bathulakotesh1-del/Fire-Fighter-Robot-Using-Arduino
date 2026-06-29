#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "img_converters.h"

#define CAMERA_MODEL_AI_THINKER

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ================= WIFI DETAILS =================
const char* ssid = "POCOX5PRO";
const char* password = "123456789";

WebServer server(80);

// ================= STREAM FUNCTION =================
void handle_jpg_stream() {
  camera_fb_t * fb = NULL;
  uint8_t * jpg_buf = NULL;
  size_t jpg_len = 0;
  char response_buf[64];

  String response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.client().write(response.c_str(), response.length());

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) break;

    // Convert RAW RGB565 → JPEG
    bool ok = frame2jpg(fb, 15, &jpg_buf, &jpg_len); // quality=15 best stable clarity
    esp_camera_fb_return(fb);
    if (!ok) continue;

    size_t hlen = snprintf(response_buf, sizeof(response_buf),
                           "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                           jpg_len);
    server.client().write(response_buf, hlen);
    server.client().write((const char*)jpg_buf, jpg_len);
    server.client().write("\r\n", 2);

    free(jpg_buf);
    if (!server.client().connected()) break;
  }
}

// ================= WEB PAGE =================
void handle_root() {
  server.send(200, "text/html",
              "<h2>📷 ESP32-CAM Live Stream</h2><img src='/stream' width='600'>");
}

// ================= SERVER INIT =================
void startServer() {
  server.on("/", HTTP_GET, handle_root);
  server.on("/stream", HTTP_GET, handle_jpg_stream);
  server.begin();
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(500);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  // Best stable clarity for NON-JPEG sensors
  config.xclk_freq_hz = 20000000;              // Do NOT increase to 24MHz
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_CIF;           // 400x296 → clear and stable
  config.jpeg_quality = 15;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed!");
    return;
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("Stream at: http://");
  Serial.println(WiFi.localIP());

  startServer();
  Serial.println("Camera READY!");
}

// ================= LOOP =================
void loop() {
  server.handleClient();
}
