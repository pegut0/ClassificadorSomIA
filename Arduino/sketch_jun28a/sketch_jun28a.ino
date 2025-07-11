#include <WiFi.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIGURAÇÕES DO PROJETO ---
// WiFi
const char* WIFI_SSID = "Guedes";
const char* WIFI_PASSWORD = "13140107Jp";

// Servidor Flask
const char* SERVER_HOST = "192.168.18.103"; // Apenas o IP ou nome do host
const int SERVER_PORT = 5000;
const char* SERVER_PATH = "/predict";

// Display OLED
#define SCREEN_WIDTH 128 // Largura do OLED em pixels
#define SCREEN_HEIGHT 64 // Altura do OLED em pixels
#define OLED_RESET    -1 // Pino de Reset (-1 se não estiver em uso)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pinos do Microfone I2S
#define I2S_WS_PIN    15
#define I2S_SCK_PIN   14
#define I2S_SD_PIN    32
#define I2S_PORT      I2S_NUM_0

// Parâmetros do Áudio
const int SAMPLE_RATE = 22050;
const int RECORD_DURATION_S = 5; 
const int BITS_PER_SAMPLE = 16;
const float GAIN_FACTOR = 2.0; 
const int16_t NOISE_GATE_THRESHOLD = 150;
const float CONFIDENCE_THRESHOLD = 0.70; 

// --- TAMANHOS DO BUFFER ---
const int WAV_HEADER_SIZE = 44;
const int RECORD_BUFFER_SIZE = (SAMPLE_RATE * RECORD_DURATION_S * BITS_PER_SAMPLE / 8);
const int I2S_BUFFER_SIZE_IN_BYTES = 4096;

// --- PROTÓTIPOS DAS FUNÇÕES ---
void connectToWiFi();
void initI2S();
void recordAndStreamAudio();
void createWavHeader(uint8_t* header, int dataSize);
void displayMessage(const String& line1, const String& line2 = "");

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nIniciando Classificador de Som (v8 - Display Otimizado)...");

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Falha ao iniciar o display SSD1306"));
    for(;;);
  }
  displayMessage("Iniciando...");

  connectToWiFi();
  initI2S();
  
  pinMode(0, INPUT_PULLUP);

  displayMessage("Pronto!");
  Serial.println("\nSetup concluído.");
}

// --- LOOP ---
void loop() {
  if (digitalRead(0) == LOW) {
    recordAndStreamAudio();
    delay(5000); 
    displayMessage("Pronto!");
  }
}

// --- FUNÇÕES AUXILIARES ---

// --- MUDANÇA: Nova função de display inteligente ---
void displayMessage(const String& line1, const String& line2) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2); // Tamanho grande para a predição

  int spaceIndex = line1.indexOf(' ');

  // Se o texto for longo e tiver um espaço, divide em duas linhas
  if (line1.length() > 11 && spaceIndex > 0) {
    String first_part = line1.substring(0, spaceIndex);
    String second_part = line1.substring(spaceIndex + 1);
    
    display.setCursor(0, 10);
    display.println(first_part);
    display.setCursor(0, 35);
    display.println(second_part);
  } else { // Se for curto, centraliza em uma linha
    display.setCursor(0, 25);
    display.println(line1);
  }

  // Mostra a confiança na parte de baixo com fonte menor
  if (line2 != "") {
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.println(line2);
  }

  display.display();
}

void connectToWiFi() {
  displayMessage("Conectando", "WiFi...");
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  displayMessage("WiFi OK!", WiFi.localIP().toString());
  delay(2000);
}

void initI2S() {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, 
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 1024,
      .use_apll = false
  };
  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK_PIN,
      .ws_io_num = I2S_WS_PIN,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD_PIN
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  Serial.println("Driver I2S do microfone instalado.");
}

void recordAndStreamAudio() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ERRO: WiFi desconectado.");
    displayMessage("Erro WiFi");
    return;
  }

  Serial.println("\n-------------------------------------");
  Serial.println("Iniciando gravação e streaming...");
  displayMessage("Gravando...");

  WiFiClient client;
  if (!client.connect(SERVER_HOST, SERVER_PORT)) {
    Serial.println("ERRO: Falha na conexão com o servidor.");
    displayMessage("Erro Servidor");
    return;
  }
  Serial.println("Conexão com o servidor estabelecida.");

  client.println(String("POST ") + SERVER_PATH + " HTTP/1.1");
  client.println(String("Host: ") + SERVER_HOST);
  client.println("Content-Type: audio/wav");
  client.println(String("Content-Length: ") + (WAV_HEADER_SIZE + RECORD_BUFFER_SIZE));
  client.println("Connection: close");
  client.println();

  uint8_t wav_header[WAV_HEADER_SIZE];
  createWavHeader(wav_header, RECORD_BUFFER_SIZE);
  client.write(wav_header, WAV_HEADER_SIZE);
  Serial.println("Cabeçalho WAV enviado.");

  int32_t* i2s_raw_buffer = (int32_t*) malloc(I2S_BUFFER_SIZE_IN_BYTES);
  int16_t* i2s_16bit_buffer = (int16_t*) malloc(I2S_BUFFER_SIZE_IN_BYTES / 2);

  if (!i2s_raw_buffer || !i2s_16bit_buffer) {
    Serial.println("ERRO: Falha ao alocar memória!");
    if(i2s_raw_buffer) free(i2s_raw_buffer);
    if(i2s_16bit_buffer) free(i2s_16bit_buffer);
    client.stop();
    return;
  }

  size_t bytes_sent = 0;
  Serial.printf("Gravando e enviando áudio por %d segundos...\n", RECORD_DURATION_S);
  
  int16_t last_sample = 0;

  while (bytes_sent < RECORD_BUFFER_SIZE) {
    size_t bytes_read;
    i2s_read(I2S_PORT, i2s_raw_buffer, I2S_BUFFER_SIZE_IN_BYTES, &bytes_read, portMAX_DELAY);
    if (bytes_read > 0) {
      int samples_read = bytes_read / 4;
      for (int i = 0; i < samples_read; i++) {
        int16_t sample16 = (int16_t)(i2s_raw_buffer[i] >> 11);
        if (abs(sample16) < NOISE_GATE_THRESHOLD) {
          sample16 = 0;
        }
        int32_t amplified_sample = (int32_t)sample16 * GAIN_FACTOR;
        if (amplified_sample > 32767) amplified_sample = 32767;
        if (amplified_sample < -32768) amplified_sample = -32768;
        int16_t final_sample = (amplified_sample + last_sample) / 2;
        last_sample = amplified_sample;
        i2s_16bit_buffer[i] = final_sample;
      }
      client.write((const uint8_t*)i2s_16bit_buffer, bytes_read / 2);
      bytes_sent += (bytes_read / 2);
    }
  }
  Serial.println("Envio do áudio concluído.");

  free(i2s_raw_buffer);
  free(i2s_16bit_buffer);

  displayMessage("Aguardando", "Servidor...");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("ERRO: Servidor não respondeu.");
      displayMessage("Sem Resposta");
      client.stop();
      return;
    }
  }

  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") { break; }
  }

  String payload = client.readString();
  client.stop();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("Falha ao interpretar JSON: ");
    Serial.println(error.c_str());
    displayMessage("Erro JSON");
    return;
  }

  const char* prediction = doc["prediction"] | "N/A";
  const char* confidence_char = doc["confidence"] | "0.0";
  float confidence_float = atof(confidence_char);

  Serial.println("\n--- RESULTADO DA CLASSIFICAÇÃO ---");

  if (confidence_float >= CONFIDENCE_THRESHOLD) {
    Serial.printf(">> Predição: %s (Confiança: %.2f)\n", prediction, confidence_float);
    
    String conf_str = "Conf: ";
    conf_str += confidence_char;
    displayMessage(prediction, conf_str);
  } else {
    Serial.printf(">> Som não reconhecido (Confiança: %.2f < %.2f)\n", confidence_float, CONFIDENCE_THRESHOLD);
    displayMessage("Nao", "reconhecido");
  }
  Serial.println("------------------------------------");
}

void createWavHeader(uint8_t* header, int dataSize) {
  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  unsigned int fileSize = dataSize + 36;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1; header[21] = 0;
  header[22] = 1; /* Mono */ header[23] = 0;
  header[24] = (byte)(SAMPLE_RATE & 0xFF);
  header[25] = (byte)((SAMPLE_RATE >> 8) & 0xFF);
  header[26] = 0; header[27] = 0;
  unsigned int byteRate = SAMPLE_RATE * 1 * BITS_PER_SAMPLE / 8;
  header[28] = (byte)(byteRate & 0xFF);
  header[29] = (byte)((byteRate >> 8) & 0xFF);
  header[30] = 0; header[31] = 0;
  header[32] = 1 * BITS_PER_SAMPLE / 8; header[33] = 0;
  header[34] = BITS_PER_SAMPLE; header[35] = 0;
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = (byte)(dataSize & 0xFF);
  header[41] = (byte)((dataSize >> 8) & 0xFF);
  header[42] = (byte)((dataSize >> 16) & 0xFF);
  header[43] = (byte)((dataSize >> 24) & 0xFF);
}
