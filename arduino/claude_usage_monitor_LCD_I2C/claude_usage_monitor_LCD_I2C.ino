/*
 * Claude Usage Monitor — ESP32 + LCD 16x2 I2C (0x27)
 * --------------------------------------------------
 * Duas telas que alternam a cada 5s:
 *
 *   Tela A (barras):     5h ▓▓▓▓░░░  42%
 *
 *                        7d ▓▓▓▓▓▓░  78%
 *
 *   Tela B (reset):      5h reset 4h12m
 *                        7d reset 2d 5h
 *
 *
 * O relogio e sincronizado por NTP pra calcular o "falta X" do reset.
 *
 * Ligacao: LCD SDA->GPIO21, SCL->GPIO22, VCC->5V, GND->GND
 * Token: rode  `claude setup-token`  e cole abaixo.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

// --------------- Setup (edite os valores) ---------------
const char* WIFI_SSID   = "WIFI-NAME"; //  Nome da rede
const char* WIFI_PASS   = "PASSWORD"; // Senha da rede
const char* OAUTH_TOKEN = "sk-ant-oat01-YOUR-TOKEN"; // token claude
  
const unsigned long POLL_INTERVAL_MS = 60UL * 1000UL;   // 60s para ler o usage
const unsigned long VIEW_SWITCH_MS   = 5UL  * 1000UL;   // 5s para trocar de tela
const char* MODEL = "claude-haiku-4-5-20251001";

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Caracteres customizados: barras de 1 a 4 colunas (a cheia usa 0xFF).
byte barChar[4][8] = {
  {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10},
  {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18},
  {0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C},
  {0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E},
};

// Estado / cache dos ultimos valores lidos
bool  haveData = false;
float pct5 = 0, pct7 = 0;     
long  reset5 = 0, reset7 = 0; 

unsigned long lastPoll   = 0;
unsigned long lastSwitch = 0;
unsigned long lastTick   = 0;
int   currentView = 0;        // 0 = barras, 1 = reset

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  for (int i = 0; i < 4; i++) lcd.createChar(i, barChar[i]);

  lcd.clear();
  lcd.print("Claude Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Conectando WiFi");
  connectWiFi();

  // Sincroniza relogio pra calcular o countdown do reset.
  lcd.setCursor(0, 1);
  lcd.print("Sync relogio   ");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  unsigned long t0 = millis();
  while (time(nullptr) < 1700000000UL && millis() - t0 < 10000) delay(200);

  lastPoll   = millis() - POLL_INTERVAL_MS;  // forca leitura imediata
  lastSwitch = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  // 1) Le a API de tempos em tempos
  if (millis() - lastPoll >= POLL_INTERVAL_MS) {
    lastPoll = millis();
    if (fetchUsage()) {
      haveData = true;
      renderView(true);          // redesenha ja com dados novos
    } else if (!haveData) {
      showError("Falha leitura");
    }
  }

  // 2) Alterna entre as duas telas
  if (millis() - lastSwitch >= VIEW_SWITCH_MS) {
    lastSwitch = millis();
    currentView ^= 1;
    renderView(true);
  }
  // 3) Na tela de reset, atualiza o countdown a cada 1s (sem chamar a API)
  else if (currentView == 1 && millis() - lastTick >= 1000) {
    lastTick = millis();
    renderView(false);
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) delay(300);
  if (WiFi.status() != WL_CONNECTED) showError("Sem WiFi");
}

// Le a API e atualiza pct5/pct7/reset5/reset7. Retorna true se deu certo.
bool fetchUsage() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, "https://api.anthropic.com/v1/messages")) return false;

  http.addHeader("content-type", "application/json");
  http.addHeader("authorization", String("Bearer ") + OAUTH_TOKEN);
  http.addHeader("anthropic-version", "2023-06-01");
  http.addHeader("anthropic-beta", "oauth-2025-04-20,claude-code-20250219");
  http.addHeader("user-agent", "claude-cli/1.0.0 (external)");
  http.addHeader("x-app", "cli");

  const char* keys[] = {
    "anthropic-ratelimit-unified-5h-utilization",
    "anthropic-ratelimit-unified-7d-utilization",
    "anthropic-ratelimit-unified-5h-reset",
    "anthropic-ratelimit-unified-7d-reset"
  };
  http.collectHeaders(keys, 4);

  String body =
    "{\"model\":\"" + String(MODEL) + "\","
    "\"max_tokens\":1,"
    "\"system\":\"You are Claude Code, Anthropic's official CLI for Claude.\","
    "\"messages\":[{\"role\":\"user\",\"content\":\"ping\"}]}";

  int code = http.POST(body);
  Serial.printf("HTTP %d\n", code);

  bool ok = false;
  if (code > 0) {
    String u5 = http.header("anthropic-ratelimit-unified-5h-utilization");
    String u7 = http.header("anthropic-ratelimit-unified-7d-utilization");
    String r5 = http.header("anthropic-ratelimit-unified-5h-reset");
    String r7 = http.header("anthropic-ratelimit-unified-7d-reset");

    if (u5.length() > 0) { pct5 = normalizePct(u5); ok = true; }
    if (u7.length() > 0) { pct7 = normalizePct(u7); ok = true; }
    if (r5.length() > 0) reset5 = strtol(r5.c_str(), nullptr, 10);
    if (r7.length() > 0) reset7 = strtol(r7.c_str(), nullptr, 10);

    if (!ok) Serial.println(http.getString());  // debug se faltou header
  }
  http.end();
  return ok;
}

// Aceita "0.48" ou "48" e devolve sempre 0..100
float normalizePct(const String &s) {
  float v = s.toFloat();
  if (v <= 1.0) v *= 100.0;
  if (v < 0)   v = 0;
  if (v > 100) v = 100;
  return v;
}

void renderView(bool full) {
  if (!haveData) return;
  if (currentView == 0) {       // tela 1, usage
    if (full) lcd.clear();
    drawBarRow(0, "5h ", pct5);
    drawBarRow(1, "7d ", pct7);
  } else {                      // tela 2, tempo
    if (full) {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("5h reset");
      lcd.setCursor(0, 1); lcd.print("7d reset");
    }
    printCountdown(0, reset5);
    printCountdown(1, reset7);
  }
}

void drawBarRow(int row, const char* label, float pct) {
  lcd.setCursor(0, row);
  lcd.print(label);
  drawBar(row, 3, 9, pct);
  char buf[6];
  snprintf(buf, sizeof(buf), "%3d%%", (int)(pct + 0.5));
  lcd.setCursor(12, row);
  lcd.print(buf);
}

void drawBar(int row, int colStart, int width, float pct) {
  int totalPx = width * 5;
  int filled  = (int)(pct / 100.0 * totalPx + 0.5);
  if (pct > 0 && filled < 1) filled = 1;   // traço minimo em 1%
  lcd.setCursor(colStart, row);
  for (int i = 0; i < width; i++) {
    int cellPx = filled - i * 5;
    if (cellPx >= 5)      lcd.write((uint8_t)0xFF);
    else if (cellPx <= 0) lcd.write(' ');
    else                  lcd.write((uint8_t)(cellPx - 1));
  }
}

// Escreve o tempo restante a partir da coluna 10 (6 chars de espaco).
void printCountdown(int row, long resetEpoch) {
  lcd.setCursor(9, row);
  String t = formatCountdown(resetEpoch);
  while (t.length() < 7) t += ' ';   // limpa sobras da leitura anterior
  lcd.print(t);
}

// Converte epoch de reset em algo curto: "2d 5h", "4h12m", "12m", "now".
String formatCountdown(long resetEpoch) {
  long now = (long)time(nullptr);
  if (resetEpoch <= 0 || now < 1700000000L) return "--";
  long rem = resetEpoch - now;
  if (rem <= 0) return "now";

  long d = rem / 86400;
  long h = (rem % 86400) / 3600;
  long m = (rem % 3600) / 60;

  char buf[8];
  if (d > 0)      snprintf(buf, sizeof(buf), "%ldd %ldh", d, h);
  else if (h > 0) snprintf(buf, sizeof(buf), "%ldh%02ldm", h, m);
  else if (m > 0) snprintf(buf, sizeof(buf), "%ldm", m);
  else            snprintf(buf, sizeof(buf), "<1m");
  return String(buf);
}

void showError(const char* msg) {
  lcd.clear();
  lcd.print("Erro:");
  lcd.setCursor(0, 1);
  lcd.print(msg);
}
