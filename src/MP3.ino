/* 
  Project: Personalized MP3 Player Project
  Class: CS 3651- Prototyping Intelligent Devices
  Description: Code for the MP3 player I built from scratch.
  Author: Emily Mizuki
  Date Created: July 10, 2025
  Date Modified: July 28, 2025
*/

// Include necessary libraries
#include <DFRobot_DF1201S.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "music.h" // Bitmap resources for display

// Define pin mappings for display
#define TFT_CS D10
#define TFT_DC D12
#define TFT_RST D11

// Define pin mappings for buttons
#define PAUSE_PLAY D5
#define VOL_UP D24
#define VOL_DOWN A1
#define FORWARD A2
#define BACKWARD A3

// Define pin mappings for DFRobot MP3 Player Pro
#define RX D9
#define TX D6

// Define colors for the display to use
#define BACKGROUND 0xFD56
#define LIGHT_PINK 0xEC12
#define MED_PINK 0xE330
#define DARK_PINK 0xE22D

// Define Konami Code aliases
#define KONAMI_VOL_UP VOL_UP
#define KONAMI_VOL_DOWN VOL_DOWN
#define KONAMI_BACK BACKWARD
#define KONAMI_FORWARD FORWARD

// Create TFT display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Connect RP to audio module and start serial comm
SoftwareSerial DF1201SSerial(D9, D6);  // RX  TX
DFRobot_DF1201S DF1201S;

// Variables
int volume = 10;
unsigned long lastVolumeChange = 0;
bool showVolumeBar = false;

String currentFileName;
unsigned long lastUpdate = 0;
bool trackJustChanged = false;
bool musicVisualMode = false; // Toggle for visual mode
static bool lastModeWasVisual = true;  // Track last state
bool isPlaying = false;

int scrollOffsetSong = 0;
int scrollOffsetArtist = 0;
int prevScrollOffsetSong = 0;
int prevScrollOffsetArtist = 0;

// Last drawn text (for detecting changes)
String lastSongName = "";
String lastArtistName = "";


void setup() {
  // Start Serial comm
  Serial.begin(115200);
  DF1201SSerial.begin(115200);
  SPI.begin();

  // Setup buttons
  pinMode(PAUSE_PLAY, INPUT);
  pinMode(VOL_UP, INPUT);
  pinMode(VOL_DOWN, INPUT);
  pinMode(FORWARD, INPUT);
  pinMode(BACKWARD, INPUT);

  // Make sure audio module is initialized
  while (!DF1201S.begin(DF1201SSerial)) {
    Serial.println("Init failed, please check the wire connection!");
    delay(1000);
  }

  // Set up display and align it properly
  tft.init(240, 320);
  tft.setSPISpeed(40000000);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.invertDisplay(true);
  tft.drawRGBBitmap(0, 0, music1, 320, 240);

  // Set MP3 module to music mode and repeat-all playback
  DF1201S.switchFunction(DF1201S.MUSIC);
  delay(2500);
  DF1201S.setPlayMode(DF1201S.ALLCYCLE);
  currentFileName = DF1201S.getFileName();

  // Draw initial screen
  tft.fillScreen(BACKGROUND);
  showSongInfo(currentFileName);
}

void loop() {
  // Redraw music UI if exiting visual mode
  if (!musicVisualMode && lastModeWasVisual) {
    tft.drawRGBBitmap(10, 30, staff2, 300, 70);
  }
  lastModeWasVisual = musicVisualMode;  // Update tracker


  // Hide volume bar after 3s
  if (!musicVisualMode && showVolumeBar && (millis() - lastVolumeChange > 3000)) {
    tft.fillRect(85, 10, 150, 10, BACKGROUND);
    showVolumeBar = false;
  }

  // Update progress bar every 1s
  static unsigned long lastProgressUpdate = 0;
  if (!musicVisualMode && millis() - lastProgressUpdate > 1000) {
    lastProgressUpdate = millis();
    int currentSec = DF1201S.getCurTime();
    int totalSec = DF1201S.getTotalTime();
    drawProgressBarWithTime(currentSec, totalSec);
  }

  // If song has changed, clear UI and update filename
  String playingNow = DF1201S.getFileName();
  if (!musicVisualMode && playingNow != currentFileName && playingNow != "") {
    currentFileName = playingNow;
    tft.fillRect(10, 120, 300, 25, BACKGROUND); // Clear song area
    tft.fillRect(10, 150, 300, 20, BACKGROUND); // Clear artist area
    scrollOffsetSong = scrollOffsetArtist = 0;
    lastSongName = "";
    lastArtistName = "";
    drawProgressBarWithTime(0, DF1201S.getTotalTime());
    trackJustChanged = true;
  }

  // Update marquee animation every 100ms
  static unsigned long lastUpdate = 0;
  if (!musicVisualMode && millis() - lastUpdate > 100) {
    lastUpdate = millis();
    showSongInfo(currentFileName);
  }

  // Button press detection (debounced)
  static int lastButton = -1;
  static unsigned long lastButtonTime = 0;

  int pressed = -1;
  if (digitalRead(PAUSE_PLAY) == HIGH) pressed = PAUSE_PLAY;
  else if (digitalRead(VOL_UP) == HIGH) pressed = VOL_UP;
  else if (digitalRead(VOL_DOWN) == HIGH) pressed = VOL_DOWN;
  else if (digitalRead(FORWARD) == HIGH) pressed = FORWARD;
  else if (digitalRead(BACKWARD) == HIGH) pressed = BACKWARD;

  // Button action handling
  if (pressed != -1 && pressed != lastButton && millis() - lastButtonTime > 200) {
    lastButton = pressed;
    lastButtonTime = millis();

    switch (pressed) {
      case PAUSE_PLAY:
        updateKonami(PAUSE_PLAY);   // Resets Konami if wrong (Pause/Play was pushed)
        isPlaying = !isPlaying;
        isPlaying ? DF1201S.start() : DF1201S.pause();
        break;

      case VOL_UP:
        updateKonami(VOL_UP);
        volume = min(volume + 1, 30);
        DF1201S.setVol(volume);

        if (!musicVisualMode) {
          drawVolumeBar(volume);
          showVolumeBar = true;
          lastVolumeChange = millis();
        }
        break;

      case VOL_DOWN:
        updateKonami(VOL_DOWN);
        volume = max(volume - 1, 0);
        DF1201S.setVol(volume);

        if (!musicVisualMode) {
          drawVolumeBar(volume);
          showVolumeBar = true;
          lastVolumeChange = millis();
        }
        break;

      case FORWARD:
        updateKonami(FORWARD);
        DF1201S.next();
        currentFileName = DF1201S.getFileName();
        scrollOffsetSong = scrollOffsetArtist = 0;

        if (!musicVisualMode) {
          tft.fillRect(10, 120, 300, 25, BACKGROUND);
          tft.fillRect(10, 150, 300, 20, BACKGROUND);
        }
        break;

      case BACKWARD:
        updateKonami(BACKWARD);
        DF1201S.last();
        currentFileName = DF1201S.getFileName();
        scrollOffsetSong = scrollOffsetArtist = 0;

        if (!musicVisualMode) {
          tft.fillRect(10, 120, 300, 25, BACKGROUND);
          tft.fillRect(10, 150, 300, 20, BACKGROUND);
        }
        break;
    }
  }

  if (pressed == -1) {
    lastButton = -1;
  }
}

/**
 * @brief Draws the volume bar on the display.
 *
 * This function draws a visual representation of the current volume level.
 * It shows a light pink background and a medium pink filled bar representing the volume.
 *
 * @param vol The current volume level (0 to 30).
 */
void drawVolumeBar(int vol) {
  int barWidth = map(vol, 0, 30, 0, 150);
  tft.fillRect(85, 10, 150, 10, LIGHT_PINK);     // Background bar
  tft.fillRect(85, 10, barWidth, 10, MED_PINK);  // Volume bar
}

/**
 * @brief Draws the playback progress bar and timestamps.
 *
 * This function draws a horizontal bar to represent current track progress and displays
 * elapsed and total time in MM:SS format below the bar.
 *
 * @param currentSec The current playback time in seconds.
 * @param totalSec The total duration of the track in seconds.
 */
void drawProgressBarWithTime(int currentSec, int totalSec) {
  int barX = 10, barY = 180, barWidth = 300, barHeight = 10;
  int filledWidth = map(currentSec, 0, totalSec, 0, barWidth);

  // Draw background
  tft.fillRect(barX, barY, barWidth, barHeight, LIGHT_PINK);

  // Draw progress
  tft.fillRect(barX, barY, filledWidth, barHeight, MED_PINK);

  // Draw time info
  char elapsedStr[6];
  char totalStr[6];

  sprintf(elapsedStr, "%02d:%02d", currentSec / 60, currentSec % 60);
  sprintf(totalStr, "%02d:%02d", totalSec / 60, totalSec % 60);

  // Clear previous time (overdraw with BACKGROUND)
  tft.fillRect(barX, barY + 15, barWidth, 15, BACKGROUND);

  // Elapsed time on left
  tft.setTextSize(1);
  tft.setTextColor(DARK_PINK);
  tft.setCursor(barX, barY + 15);
  tft.print(elapsedStr);

  // Total time on right
  tft.setCursor(barX + barWidth - 30, barY + 15);
  tft.print(totalStr);
}

/**
 * @brief Displays the current song and artist name on the screen.
 *
 * If the track just changed, the names are drawn statically. Otherwise,
 * scrolling marquee animation is used if the text is wider than the display area.
 *
 * @param fileName The name of the audio file currently being played.
 */
void showSongInfo(String fileName) {
  if (trackJustChanged) {
    // Remove the .mp3 extension from the filename
    int dotIndex = fileName.lastIndexOf('.');
    if (dotIndex != -1) fileName = fileName.substring(0, dotIndex);

    // Split the filename into song name and artist using dash as delimiter
    int dashIndex = fileName.indexOf('-');
    String songName = (dashIndex != -1) ? fileName.substring(0, dashIndex) : fileName;
    String artistName = (dashIndex != -1) ? fileName.substring(dashIndex + 1) : "";
    songName.trim();
    artistName.trim();

    // Draw the song name in large text
    tft.setTextSize(3);
    tft.setTextColor(DARK_PINK);
    tft.setCursor(10, 120);
    tft.print(songName);

// Draw the artist name in smaller text
    tft.setTextSize(2);
    tft.setTextColor(MED_PINK);
    tft.setCursor(10, 150);
    tft.print(artistName);

    // Reset flag so marquee resumes next cycle
    trackJustChanged = false;
    return;  // Exit early to avoid scroll logic this frame
  }

  // Remove .mp3 extension
  int dotIndex = fileName.lastIndexOf('.');
  if (dotIndex != -1) fileName = fileName.substring(0, dotIndex);

  // Split into Song and Artist
  int dashIndex = fileName.indexOf('-');
  String songName = (dashIndex != -1) ? fileName.substring(0, dashIndex) : fileName;
  String artistName = (dashIndex != -1) ? fileName.substring(dashIndex + 1) : "";
  songName.trim();
  artistName.trim();

  // Box positions
  int songX = 10, songY = 120, songWidth = 300, songHeight = 25;
  int artistX = 10, artistY = 150, artistWidth = 300, artistHeight = 20;


  // Character width for default font
  int charW = 6;
  int songPixelWidth = songName.length() * charW * 3;      // textSize = 3
  int artistPixelWidth = artistName.length() * charW * 2;  // textSize = 2

  // Handle drawing of the song name
  if (songPixelWidth <= songWidth) {
    // Text fits in the box — draw once if it changed
    if (songName != lastSongName) {
      tft.fillRect(songX, songY, songWidth, songHeight, BACKGROUND);
      tft.setTextSize(3);
      tft.setTextColor(DARK_PINK);
      tft.setCursor(songX, songY);
      tft.print(songName);
      lastSongName = songName;
    }

    // Reset scrolling state
    scrollOffsetSong = 0;
    prevScrollOffsetSong = 0;
  } else {
    // Erase old scroll text in background color
    drawMarquee(songName, songX, songY, prevScrollOffsetSong, songWidth, 3, BACKGROUND);
    drawMarquee(songName, songX, songY, prevScrollOffsetSong + songPixelWidth + 40, songWidth, 3, BACKGROUND);

    // Draw updated scroll text
    drawMarquee(songName, songX, songY, scrollOffsetSong, songWidth, 3, DARK_PINK);
    drawMarquee(songName, songX, songY, scrollOffsetSong + songPixelWidth + 40, songWidth, 3, DARK_PINK);

    // Update scroll position for next frame
    prevScrollOffsetSong = scrollOffsetSong;
    scrollOffsetSong -= 2;  // Scroll speed
    if (scrollOffsetSong <= -(songPixelWidth + 40)) scrollOffsetSong = 0;

    // Reset name tracker to force redraw if song changes
    lastSongName = "";  // Force redraw if song changes
  }

  // Handle drawing of the artist name
  if (artistPixelWidth <= artistWidth) {
    // Text fits — draw once if changed
    if (artistName != lastArtistName) {
      tft.fillRect(artistX, artistY, artistWidth, artistHeight, BACKGROUND);
      tft.setTextSize(2);
      tft.setTextColor(MED_PINK);
      tft.setCursor(artistX, artistY);
      tft.print(artistName);
      lastArtistName = artistName;
    }
    scrollOffsetArtist = 0;
    prevScrollOffsetArtist = 0;
  } else {
    // Erase old scroll text
    drawMarquee(artistName, artistX, artistY, prevScrollOffsetArtist, artistWidth, 2, BACKGROUND);
    drawMarquee(artistName, artistX, artistY, prevScrollOffsetArtist + artistPixelWidth + 30, artistWidth, 2, BACKGROUND);

    // Draw new scroll text
    drawMarquee(artistName, artistX, artistY, scrollOffsetArtist, artistWidth, 2, MED_PINK);
    drawMarquee(artistName, artistX, artistY, scrollOffsetArtist + artistPixelWidth + 30, artistWidth, 2, MED_PINK);

    // Update scroll position for next frame
    prevScrollOffsetArtist = scrollOffsetArtist;
    scrollOffsetArtist -= 1;  // Slower scroll for artist

    // Reset scroll after full loop
    if (scrollOffsetArtist <= -(artistPixelWidth + 30)) scrollOffsetArtist = 0;

    // Force redraw if artist changes
    lastArtistName = "";
  }
}

/**
 * @brief Draws horizontally scrolling text (marquee effect).
 *
 * This function draws characters only when they are fully inside the clipping area
 * to avoid flickering or cutoff. Used for long song/artist names.
 *
 * @param text The text to display.
 * @param baseX The left boundary of the scroll box.
 * @param baseY The vertical position of the text baseline.
 * @param offset The current scroll offset in pixels.
 * @param clipWidth The width of the visible clipping region.
 * @param textSize The text size multiplier (e.g., 2 or 3).
 * @param color The 16-bit RGB565 color of the text.
 */
void drawMarquee(String text, int baseX, int baseY, int offset, int clipWidth, int textSize, uint16_t color) {
  // Calculate character width based on font size (default font is 6 pixels wide)
  int charW = 6 * textSize;

  // Compute total width of the full text in pixels
  int textPixelWidth = text.length() * charW;

  tft.setTextSize(textSize);
  tft.setTextColor(color);

  // Draw characters only if fully inside the box
  for (int i = 0; i < text.length(); i++) {
    int charX = baseX + offset + i * charW;
    if (charX >= baseX && (charX + charW) <= (baseX + clipWidth)) {
      tft.setCursor(charX, baseY);
      tft.print(text[i]);
    }
  }
}

/**
 * @brief Tracks input to detect the Konami code sequence.
 *
 * If the user presses the correct sequence of buttons (Up, Up, Down, Down, Back, Forward, Back, Forward),
 * this function toggles the visual display mode. If an incorrect step is detected, the sequence resets.
 * Pressing the pause/play button always resets the sequence.
 *
 * @param button The most recently pressed button's pin number.
 */
void updateKonami(int button) {
  // Define the correct button sequence for the Konami code
  static const int konamiSequence[] = {
    VOL_UP, VOL_UP,
    VOL_DOWN, VOL_DOWN,
    BACKWARD, FORWARD,
    BACKWARD, FORWARD
  };

  static const int KONAMI_LENGTH = sizeof(konamiSequence) / sizeof(konamiSequence[0]);
  static int konamiIndex = 0;

  // Reset sequence if PAUSE_PLAY is pressed
  if (button == PAUSE_PLAY) {
    if (konamiIndex > 0) {
      Serial.println("PAUSE/PLAY pressed — Konami reset.");
      konamiIndex = 0;
    }
    return;  // No match checking, just reset
  }

  // Normal sequence checking
  if (button == konamiSequence[konamiIndex]) {
    konamiIndex++;
    Serial.print("Konami step ");
    Serial.print(konamiIndex);
    Serial.print(" / ");
    Serial.println(KONAMI_LENGTH);

    // Check if sequence is complete
    if (konamiIndex == KONAMI_LENGTH) {
      musicVisualMode = !musicVisualMode;
      konamiIndex = 0;

      Serial.println("KONAMI ACTIVATED");
      Serial.print("Visual Mode: ");
      Serial.println(musicVisualMode ? "ON" : "OFF");

      if (musicVisualMode) {
        tft.drawRGBBitmap(0, 0, music1, 320, 240);  // your visual mode image
      } else {
        tft.fillScreen(BACKGROUND);
        showSongInfo(currentFileName);  // redraw normal UI
      }
    }
  // input didn't match sequence, reset
  } else if (button == VOL_UP || button == VOL_DOWN || button == BACKWARD || button == FORWARD) {
    if (konamiIndex > 0) {
      Serial.println("Wrong step — Konami reset.");
      konamiIndex = 0;
    }
  }
}
