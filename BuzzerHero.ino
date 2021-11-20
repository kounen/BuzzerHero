#include <FastLED.h>

#define LED_TYPE             WS2811
#define COLOR_ORDER          GRB
#define STRIPS_NBR           1
#define LEDS_PER_STRIP_NBR   30
#define BRIGHTNESS           50
#define INIT_REFRESH_RATE    250
#define MAX_TILES_PER_STRIP  3

CRGB leds[STRIPS_NBR][LEDS_PER_STRIP_NBR];
int score;
unsigned long lastTime;
CRGB currentColor;
int currentDelay;
int tilesNbr[STRIPS_NBR];
int tiles[STRIPS_NBR][MAX_TILES_PER_STRIP];

void setupLeds(void)
{
    // Led PIN 2
    FastLED.addLeds<LED_TYPE, 2, COLOR_ORDER>(leds[0], LEDS_PER_STRIP_NBR).setCorrection(TypicalLEDStrip);
    // Led PIN 3
    FastLED.addLeds<LED_TYPE, 3, COLOR_ORDER>(leds[1], LEDS_PER_STRIP_NBR).setCorrection(TypicalLEDStrip);
    // Led PIN 4
    FastLED.addLeds<LED_TYPE, 4, COLOR_ORDER>(leds[2], LEDS_PER_STRIP_NBR).setCorrection(TypicalLEDStrip);
    // Led PIN 5
    FastLED.addLeds<LED_TYPE, 5, COLOR_ORDER>(leds[3], LEDS_PER_STRIP_NBR).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
}



int currentIndex[STRIPS_NBR];









bool canCheck[STRIPS_NBR];
bool canCreate[STRIPS_NBR];



void initGame(void)
{
  score = 0;
  lastTime = millis();
  currentColor = CRGB::Yellow;
  currentDelay = INIT_REFRESH_RATE;
  
  for(int i = 0; i < STRIPS_NBR; i++) {
    tilesNbr[i] = 0;
    canCheck[i] = true;
    canCreate[i] = true;
    currentIndex[i] = 0;
    
    for(int j = 0; j < MAX_TILES_PER_STRIP; j++) {
      tiles[i][j] = -1;
    }
  }
}

void setup(void)
{
    delay(3000); // Power-up safety delay
    Serial.begin(9600); // Use the serial port
    setupLeds();
    initGame();
    randomSeed(37);

}

#define TILE_LENGTH          4 // number of leds per tile
#define MIN_INTERVAL         5 // the minimum number of leds switched off between two tiles
#define LEVEL_DURATION       5 // number of successful tap before level up
#define MIN_TILE_DELAY       50 // in milliseconds, minimum duration for a tile in a position
#define PIEZO_THRESHOLD      500
#define GAME_OVER_DELAY      1000 // in miliseconds, duration of the break when game is over



#define TILE_GENERATION_PROB 15 // probability to generate a new tile when possible the smaller it is, the more likely it is to generate one
#define LEVEL_SPEED_INCREASE 5 // in milliseconds, increase the speed of the led strip refresh rate when level









void loop()
{ 
  for(int i = 0; i < STRIPS_NBR; i++) {
    int piezoValue = analogRead(i);

    // check if a bongo was hit
    if(piezoValue > PIEZO_THRESHOLD) {
      bool ok = false;
      for(int j = 0; j < MAX_TILES_PER_STRIP; j++) {
        if(tiles[i][j] < TILE_LENGTH*2 && tiles[i][j] != -1 && canCheck[i]) {
          score++;
          ok = true;
          canCheck[i] = false;
          FastLED.setBrightness(3*BRIGHTNESS);
          break;
        }
      }
    
      if(!ok && canCheck[i]) {
        GameOver();
      }
    }
  }

  unsigned long currentTime = millis();

  if(currentTime - lastTime > currentDelay) {
    lastTime = currentTime;

    // level up
    if(score % LEVEL_DURATION == 0 && score > 0) {
      currentDelay -= LEVEL_SPEED_INCREASE;
      if(currentDelay < MIN_TILE_DELAY) currentDelay = MIN_TILE_DELAY; // maximum speed
      currentColor = nextColor();
    }

    // one tile maximum will be created during each refresh
    bool tileCreated = false;
    
    // refresh display
    for(int i = 0; i < STRIPS_NBR; i++) {
      SwitchOffLEDs(leds[i]);

      // Fill last tile
      FillTile(TILE_LENGTH - 1, leds[i], CRGB::White);
    
      if(tilesNbr[i] < MAX_TILES_PER_STRIP) {
        // if there is enough space left on the strip
        // we try to create a new tile
        // 1/6 chance to create a new tile
        if(canCreate[i] && random(0, TILE_GENERATION_PROB) == 0 && !tileCreated) {
          tilesNbr[i]++;
          tiles[i][currentIndex[i]] = LEDS_PER_STRIP_NBR - 1;
          currentIndex[i]++;
          tileCreated = true;
        }
        canCreate[i] = true;
      }

      // if tiles array is full,
      // next tile will be created at index 0
      if( currentIndex[i] == MAX_TILES_PER_STRIP ) {
        currentIndex[i] = 0;
      }

      // Fill tiles on the strip
      for(int j = 0; j < MAX_TILES_PER_STRIP; j++) {
        if(tiles[i][j] == -1) continue;

        if(!canCheck[i] && tiles[i][j] < TILE_LENGTH + MIN_INTERVAL && MIN_INTERVAL >= TILE_LENGTH) {
          // We don't show a tile if it has already been taken in account by the player.
          // This cannot be done this way if MIN_INTERVAL < TILE_LENGTH,
          // in this case we'll have a fallback behaviour (we'll see the tile leaving the strip).
        } else {
          FillTile(tiles[i][j], leds[i], currentColor);
        }
        tiles[i][j]--;
    
        // if tile exits the strip we decrement the number of tiles
        if(tiles[i][j] == 0){
          tilesNbr[i]--;
          tiles[i][j] = -1;
          if(score > 0) {
            if(canCheck[i]) {
              GameOver();
            }
            canCheck[i] = true;
          }
        }

        // check if the tile is at the beginning of the strip
        if(tiles[i][j] >= LEDS_PER_STRIP_NBR - TILE_LENGTH - MIN_INTERVAL) canCreate[i] = false;
      }
    }

    FastLED.show();
    FastLED.setBrightness(BRIGHTNESS);
  }
}

void FillTile(int tile, CRGB leds[], CRGB color)
{
    for(int i = tile; i > tile - TILE_LENGTH; i--) {
      leds[i] = color;
      if (i == 0) break;
    }
}

void SwitchOffLEDs(CRGB leds[])
{
  for(int i = 0; i < LEDS_PER_STRIP_NBR; i++) {
    leds[i] = CRGB::Black;
  }
}



void GameOver()
{
  for(int i = 0; i < STRIPS_NBR; i++) {
    // Fill last tile in red
    FillTile(TILE_LENGTH - 1, leds[i], CRGB::Red);
  }
  FastLED.show();
  delay(GAME_OVER_DELAY);
  initGame();
}

CRGB nextColor() {
  switch (score) {
    case LEVEL_DURATION:
      return CRGB::Blue;
    case 2*LEVEL_DURATION:
      return CRGB::Green;
    case 3*LEVEL_DURATION:
      return CRGB::Purple;
    case 4*LEVEL_DURATION:
      return CRGB::Orange;
    case 5*LEVEL_DURATION:
      return CRGB::Pink;
    case 6*LEVEL_DURATION:
      return CRGB::Salmon;
    case 7*LEVEL_DURATION:
      return CRGB::DeepSkyBlue;
    case 8*LEVEL_DURATION:
      return CRGB::ForestGreen;
    default:
      return CRGB::Yellow;
  }
}
