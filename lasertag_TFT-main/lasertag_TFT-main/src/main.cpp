#include <Arduino.h>
#include <TFT_eSPI.h>

// Define the TFT display object
TFT_eSPI tft;

// Define the GameState structure for testing
struct GameState {
    char name[16] = "GEALLA";
    uint8_t life = 5;
    bool dirtyName = true;
    bool dirtyLife = true;
};

// Function to initialize the TFT display
void renderInit() {
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
}

// Function to render the game state
void render(GameState &st) {
    // Render the name
    if (st.dirtyName) {
        tft.fillRect(0, 34, 240, 40, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4); // Font 4 is alphanumeric. Font 6 only has digits and apm!
        int w = tft.textWidth(st.name);
        tft.drawString(st.name, (240 - w) / 2, 38);
        st.dirtyName = false;
    }

    // Render the life
    if (st.dirtyLife) {
        tft.fillRect(0, 90, 240, 30, TFT_BLACK);
        for (int i = 0; i < 5; i++) {
            uint16_t col = (i < st.life) ? TFT_RED : TFT_DARKGREY;
            tft.fillCircle(30 + i * 40, 105, 10, col);
        }
        st.dirtyLife = false;
    }
}

// Initialize the game state
GameState state;

// Non-blocking line reader for UART
static char s_line[64];
static size_t s_len = 0;
static bool s_lineReady = false;

void linkPoll() {
    while (Serial1.available() && !s_lineReady) {
        int c = Serial1.read();
        if (c == '\r') continue;
        if (c == '\n') {
            s_line[s_len] = '\0';
            s_lineReady = (s_len > 0);
            s_len = 0;
            return;
        }
        if (s_len < sizeof(s_line) - 1) {
            s_line[s_len++] = (char)c;
        } else {
            s_len = 0;  // overflow -> drop this line
        }
    }
}

void parseLine(const char *line, GameState &st) {
    if (!line || !*line || line[0] == '#') return;

    const char *colon = strchr(line, ':');
    if (!colon) return;

    size_t keyLen = colon - line;
    char key[16];
    if (keyLen >= sizeof(key)) return;
    memcpy(key, line, keyLen);
    key[keyLen] = '\0';

    const char *value = colon + 1;

    if (strcasecmp(key, "NAME") == 0) {
        if (strncmp(st.name, value, sizeof(st.name)) != 0) {
            strncpy(st.name, value, sizeof(st.name) - 1);
            st.name[sizeof(st.name) - 1] = '\0';
            st.dirtyName = true;
        }
    } else if (strcasecmp(key, "HIT") == 0) {
        if (st.life > 0) {
            st.life--;
            st.dirtyLife = true;
        }
    }
}

void setup() {
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}
    Serial.println("Starting TFT Test...");
#endif

    // Setup UART1 for incoming commands (RX on GPIO 18, TX not used)
    Serial1.begin(115200, SERIAL_8N1, 18, -1);
    Serial1.setRxBufferSize(512);

    // Configure the backlight pin
    pinMode(33, OUTPUT);
    digitalWrite(33, HIGH);
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.println("Backlight enabled");
#endif

    // Initialize the TFT display
    renderInit();
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.println("TFT initialized");
#endif

    // Render the initial state
    render(state);
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.println("TFT Test Complete!");
#endif
}

void loop() {
    linkPoll();
    if (s_lineReady) {
#if ARDUINO_USB_CDC_ON_BOOT
        Serial.print("Received: ");
        Serial.println(s_line);
#endif
        parseLine(s_line, state);
        s_lineReady = false;
    }

    // Update the display if anything changed
    render(state);
}