/* TomatoClock for M5Cardputer
 * Author: Faisal F Rafat
 * GitHub: https://github.com/ffrafat
 * Date: 5 May 2025
 * Version: 1.0
 *
 * Description: A Pomodoro timer with auto-start functionality. 
 * The timer alternates between focus sessions and break periods.
 * The user can start, pause, reset, and adjust focus/break durations.
 * 
 * Features:
 * - Auto-start toggle
 * - Time adjustment (+/-)
 * - Pause/resume functionality
 * - Session count display

#include "M5Cardputer.h"
#include "M5GFX.h"

M5Canvas canvas(&M5Cardputer.Display);

enum TimerState { STOPPED, RUNNING, PAUSED };
TimerState state = STOPPED;

unsigned long startTime = 0;
unsigned long pausedTime = 0;
int focusDuration = 0.5 * 60;  // Default: 25 min
int breakDuration = 0.2 * 60;   // Default: 5 min
bool onBreak = false;
int sessionsCompleted = 0;
bool autoStartNext = false;  // Toggle auto-start
bool showHelp = false;
bool showSplash = true;      // Show splash screen on startup
unsigned long splashStartTime = 0;

String statusMessage = "[S]tart Focus";

// Progress bar variables
int progressBarWidth = 200;
int progressBarHeight = 8;
int progressBarX = (240 - progressBarWidth) / 2;
int progressBarY = 90;

void drawSplash() {
    canvas.fillSprite(BLACK);
    
    // Draw "Tomato Clock" in big red font
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1.5);
    canvas.setTextColor(RED);  // Red
    canvas.drawString("TomatoClock", canvas.width() / 2, canvas.height() / 2 - 20);
    
    // Draw GitHub URL in small green font
    canvas.setTextSize(0.75);
    canvas.setTextColor(GREEN);  // Green
    canvas.drawString("github.com/ffrafat", canvas.width() / 2, canvas.height() / 2 + 10);
    
    // Draw version in white
    canvas.setTextSize(0.75);
    canvas.setTextColor(WHITE);
    canvas.drawString("Version 1.0", canvas.width() / 2, canvas.height() / 2 + 50);
    
    canvas.pushSprite(0, 0);
}

void drawUI(int remaining) {
    canvas.fillSprite(BLACK);
    canvas.setCursor(0, 0);

    // Timer display
    int minutes = remaining / 60;
    int seconds = remaining % 60;
    char buffer[16];
    sprintf(buffer, "%02d:%02d", minutes, seconds);

    canvas.setTextDatum(middle_center);
    canvas.setTextSize(3.2);
    canvas.setTextColor(onBreak ? GREEN : RED);  // Green for break, red for focus
    canvas.drawString(buffer, canvas.width() / 2, canvas.height() / 2 + 5);

    // Status message
    canvas.setTextDatum(top_center);
    canvas.setTextSize(1);
    canvas.setTextColor(WHITE);
    canvas.drawString(statusMessage, canvas.width() / 2, 10);

    // Snake progress bar variables
    const int borderThickness = 6;  // Thickness of the progress bar
    const int screenWidth = 240;
    const int screenHeight = 135;
    
    // Snake progress bar around screen border
    uint16_t barColor = onBreak ? GREEN : RED;
    float progress = 1.0 - (float)remaining / (onBreak ? breakDuration : focusDuration);
    
    // Total perimeter length (minus corners to avoid double-counting)
    int totalPerimeter = 2 * (screenWidth + screenHeight) - 4 * borderThickness;
    int filledLength = progress * totalPerimeter;
    
    // Top edge (left to right)
    if (filledLength > 0) {
        int segment = min(filledLength, screenWidth - borderThickness);
        canvas.fillRect(0, 0, segment, borderThickness, barColor);
        filledLength -= segment;
    }
    
    // Right edge (top to bottom)
    if (filledLength > 0) {
        int segment = min(filledLength, screenHeight - borderThickness);
        canvas.fillRect(screenWidth - borderThickness, borderThickness, 
                       borderThickness, segment, barColor);
        filledLength -= segment;
    }
    
    // Bottom edge (right to left)
    if (filledLength > 0) {
        int segment = min(filledLength, screenWidth - borderThickness);
        canvas.fillRect(screenWidth - borderThickness - segment, 
                       screenHeight - borderThickness, 
                       segment, borderThickness, barColor);
        filledLength -= segment;
    }
    
    // Left edge (bottom to top)
    if (filledLength > 0) {
        int segment = min(filledLength, screenHeight - 2 * borderThickness);
        canvas.fillRect(0, screenHeight - borderThickness - segment, 
                       borderThickness, segment, barColor);
    }

    // Sessions counter - centered at bottom with better visibility
    canvas.setTextDatum(bottom_center);
    canvas.setTextSize(0.9);
    canvas.setTextColor(YELLOW);

    canvas.drawString("Streak: x" + String(sessionsCompleted), canvas.width() / 2, canvas.height() - 10);

    canvas.pushSprite(0, 0);
}

void drawHelp() {
    canvas.fillSprite(BLACK);
    canvas.setTextColor(YELLOW);
    canvas.setTextDatum(top_left);
    canvas.setTextSize(1);
    int y = 10;

    canvas.drawString("[S] Start", 10, y); y += 18;
    canvas.drawString("[P] Pause", 10, y); y += 18;
    canvas.drawString("[R] Reset", 10, y); y += 18;
    canvas.drawString("[A] Auto-start", 10, y); y += 18;
    canvas.drawString("[+] Increase", 10, y); y += 18;
    canvas.drawString("[-] Decrease", 10, y); y += 18;

    canvas.pushSprite(0, 0);
}

void resetTimer() {
    state = STOPPED;
    onBreak = false;
    startTime = millis();
    statusMessage = "[S]tart Focus";
    drawUI(focusDuration);
}

void startTimer() {
    if (state == STOPPED || state == PAUSED) {
        if (state == PAUSED) {
            startTime = millis() - pausedTime;
        } else {
            startTime = millis();
        }
        state = RUNNING;
        statusMessage = onBreak ? "Relax Mode" : "Focus Mode";
    }
}

void pauseTimer() {
    if (state == RUNNING) {
        pausedTime = millis() - startTime;
        state = PAUSED;
        statusMessage = "Paused. [S]tart";
    }
}

void playAlert() {
    // Beep buzzer
    for (int i = 0; i < 3; i++) {
        M5Cardputer.Speaker.tone(1000, 300);
        delay(1000);
    }
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);

    canvas.setColorDepth(8);
    canvas.setTextFont(&fonts::FreeMonoBold12pt7b);
    canvas.createSprite(240, 135);
    canvas.setTextSize(1);
    canvas.setTextScroll(false);
    canvas.setTextWrap(true);

    splashStartTime = millis();
    drawSplash();
}

void loop() {
    M5Cardputer.update();

    // Handle splash screen timeout (3 seconds)
    if (showSplash && millis() - splashStartTime > 3000) {
        showSplash = false;
        resetTimer();
    }

    if (showSplash) {
        // Skip other processing while splash is showing
        delay(100);
        return;
    }

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto keys = M5Cardputer.Keyboard.keysState();

        if (showHelp) {
            showHelp = false;
            drawUI(onBreak ? breakDuration : focusDuration);
            return;
        }

        bool recognized = false;

        for (auto c : keys.word) {
            if (c == 'S' || c == 's') {
                startTimer(); recognized = true;
            } else if (c == 'P' || c == 'p') {
                pauseTimer(); recognized = true;
            } else if (c == 'R' || c == 'r') {
                resetTimer(); recognized = true;
            } else if (c == 'A' || c == 'a') {
                autoStartNext = !autoStartNext;
                statusMessage = autoStartNext ? "Auto-start: ON" : "Auto-start: OFF";
                drawUI(onBreak ? breakDuration : focusDuration);
                recognized = true;
            } else if (c == '+' && state == STOPPED) {
                if (!onBreak) focusDuration += 60;
                else breakDuration += 60;
                drawUI(onBreak ? breakDuration : focusDuration);
                recognized = true;
            } else if (c == '-' && state == STOPPED) {
                if (!onBreak && focusDuration > 60) focusDuration -= 60;
                else if (onBreak && breakDuration > 60) breakDuration -= 60;
                drawUI(onBreak ? breakDuration : focusDuration);
                recognized = true;
            }
        }

        if (!recognized) {
            showHelp = true;
            drawHelp();
        }
    }

    if (state == RUNNING) {
        int elapsed = (millis() - startTime) / 1000;
        int remaining = (onBreak ? breakDuration : focusDuration) - elapsed;

        if (remaining <= 0) {
            playAlert();  // Beep!
            onBreak = !onBreak;
            if (!onBreak) sessionsCompleted++;

            if (autoStartNext) {
                startTime = millis();
                state = RUNNING;
                statusMessage = onBreak ? "Relax Mode" : "Focus Mode";
            } else {
                state = STOPPED;
                statusMessage = onBreak ? "[S]tart Relax" : "[S]tart Focus";
            }
        }

        drawUI(remaining);
    } else if (state == PAUSED) {
        drawUI((onBreak ? breakDuration : focusDuration) - (pausedTime / 1000));
    }

    delay(100);
}