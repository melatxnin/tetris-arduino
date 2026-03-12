/*
  TETRIS GAME
  
  Controls:
  Left button   (A5) : Move piece left
  Rotate button (A4) : Rotate piece
  Speed button  (A3) : Accelerate fall 
  Right button  (A2) : Move piece right

  Author:
  Nicolas GOSSARD
*/

#include <Adafruit_NeoPixel.h>

// pins
constexpr int dinPin = 2;
constexpr int leftButtonPin = A5;
constexpr int rotateButtonPin = A4;
constexpr int speedButtonPin = A3;
constexpr int rightButtonPin = A2;

// grid
constexpr int gridWidth = 10;
constexpr int gridHeight = 15;
constexpr int numberPixels = gridWidth * gridHeight;
int board[gridHeight][gridWidth];

// colours
constexpr uint32_t colourOff     = 0x000000;
constexpr uint32_t colourRed     = 0xFF0000;
constexpr uint32_t colourGreen   = 0x00FF00;
constexpr uint32_t colourBlue    = 0x0000FF;
constexpr uint32_t colourYellow  = 0xFFFF00;
constexpr uint32_t colourCyan    = 0x00FFFF;
constexpr uint32_t colourMagenta = 0xFF00FF;
constexpr uint32_t colourOrange  = 0xFF6400;

Adafruit_NeoPixel pixels(numberPixels, dinPin, NEO_GRB + NEO_KHZ800);

// pieces
enum PieceType
{
  PIECE_O = 0,
  PIECE_I = 1,
  PIECE_T = 2,
  PIECE_S = 3,
  PIECE_COUNT = 4
};

struct Piece
{
  int x;
  int y;
  int type;
  int rotation;
  int colourId;
};

Piece currentPiece;

// state variables
int placedPiecesCount = 0;
bool hasActivePiece = false;
bool waitingRespawn = false;

// timers
unsigned long lastFallTime = 0;
unsigned long normalFallDelay = 300;
unsigned long fastFallDelay = 25;
unsigned long lastMoveTime = 0;
unsigned long moveRepeatDelay = 100;
unsigned long lastRotateTime = 0;
unsigned long rotateRepeatDelay = 150;
unsigned long respawnStartTime = 0;
unsigned long respawnDelay = 750;
unsigned long fallAccelerationStep = 5;
unsigned long minFallDelay = 100;


void setup()
{
  randomSeed(analogRead(A0));

  pixels.begin();
  pixels.clear();
  pixels.show();

  pinMode(leftButtonPin, INPUT_PULLUP);
  pinMode(rotateButtonPin, INPUT_PULLUP);
  pinMode(speedButtonPin, INPUT_PULLUP);
  pinMode(rightButtonPin, INPUT_PULLUP);

  resetGame();
  spawnNewPiece();
}

void loop()
{
  if (waitingRespawn)
  {
    if (millis() - respawnStartTime >= respawnDelay)
    {
      waitingRespawn = false;
      spawnNewPiece();
    }

    renderGame();
    return;
  }

  handleInput();
  updateFall();
  renderGame();
}

void resetGame()
{
  clearBoard();
  lastFallTime = millis();
  lastMoveTime = millis();
  lastRotateTime = millis();
  normalFallDelay = 300;
  placedPiecesCount = 0;
  hasActivePiece = false;
  waitingRespawn = false;
}

void clearBoard()
{
  for (int y = 0; y < gridHeight; y++)
  {
    for (int x = 0; x < gridWidth; x++)
    {
      board[y][x] = 0;
    }
  }
}

void spawnNewPiece()
{
  createPiece();
  lastFallTime = millis();

  if (!canPlacePiece(currentPiece.x, currentPiece.y,
                     currentPiece.type, currentPiece.rotation))
  {
    resetGame();
    waitingRespawn = true;
    respawnStartTime = millis();
    hasActivePiece = false;
    return;
  }

  hasActivePiece = true;
}

void createPiece()
{
  currentPiece.type = random(0, PIECE_COUNT);
  currentPiece.rotation = random(0, 4);
  currentPiece.x = random(0, gridWidth - 3);
  currentPiece.y = 0;
  currentPiece.colourId = random(1, 8);
}

bool canPlacePiece(int testX, int testY, int pieceType, int rotation)
{
  for (int localY = 0; localY < 4; localY++)
  {
    for (int localX = 0; localX < 4; localX++)
    {
      if (!pieceCellIsFilled(pieceType, rotation, localX, localY))
      {
        continue;
      }

      int worldX = testX + localX;
      int worldY = testY + localY;

      if (!isInside(worldX, worldY))
      {
        return false;
      }

      if (board[worldY][worldX] != 0)
      {
        return false;
      }
    }
  }

  return true;
}

bool pieceCellIsFilled(int pieceType, int rotation, int localX, int localY)
{
  if (localX < 0 || localX > 3 || localY < 0 || localY > 3)
  {
    return false;
  }

  if (pieceType == PIECE_O)
  {
    return (localX == 1 || localX == 2) && (localY == 0 || localY == 1);
  }

  if (pieceType == PIECE_I)
  {
    rotation = rotation % 4;

    if (rotation == 0 || rotation == 2)
    {
      return localY == 1 && localX >= 0 && localX <= 3;
    }

    return localX == 1 && localY >= 0 && localY <= 3;
  }

  if (pieceType == PIECE_T)
  {
    rotation = rotation % 4;

    if (rotation == 0)
    {
      return
        (localX == 1 && localY == 0) ||
        (localY == 1 && localX >= 0 && localX <= 2);
    }

    if (rotation == 1)
    {
      return
        (localX == 1 && localY == 0) ||
        (localX == 1 && localY == 1) ||
        (localX == 2 && localY == 1) ||
        (localX == 1 && localY == 2);
    }

    if (rotation == 2)
    {
      return
        (localY == 1 && localX >= 0 && localX <= 2) ||
        (localX == 1 && localY == 2);
    }

    return
      (localX == 1 && localY == 0) ||
      (localX == 0 && localY == 1) ||
      (localX == 1 && localY == 1) ||
      (localX == 1 && localY == 2);
  }

  if (pieceType == PIECE_S)
  {
    rotation = rotation % 4;

    if (rotation == 0 || rotation == 2)
    {
      return
        (localX == 1 && localY == 0) ||
        (localX == 2 && localY == 0) ||
        (localX == 0 && localY == 1) ||
        (localX == 1 && localY == 1);
    }

    return
      (localX == 1 && localY == 0) ||
      (localX == 1 && localY == 1) ||
      (localX == 2 && localY == 1) ||
      (localX == 2 && localY == 2);
  }

  return false;
}

bool isInside(int x, int y)
{
  return x >= 0 && x < gridWidth && y >= 0 && y < gridHeight;
}

void handleInput()
{
  if (!hasActivePiece)
  {
    return;
  }

  unsigned long now = millis();

  if (now - lastMoveTime >= moveRepeatDelay)
  {
    if (digitalRead(leftButtonPin) == LOW)
    {
      if (canPlacePiece(currentPiece.x - 1, currentPiece.y,
                        currentPiece.type, currentPiece.rotation))
      {
        currentPiece.x--;
      }

      lastMoveTime = now;
    }
    else if (digitalRead(rightButtonPin) == LOW)
    {
      if (canPlacePiece(currentPiece.x + 1, currentPiece.y,
                        currentPiece.type, currentPiece.rotation))
      {
        currentPiece.x++;
      }

      lastMoveTime = now;
    }
  }

  if (now - lastRotateTime >= rotateRepeatDelay)
  {
    if (digitalRead(rotateButtonPin) == LOW)
    {
      int newRotation = (currentPiece.rotation + 1) % 4;

      if (canPlacePiece(currentPiece.x, currentPiece.y,
                        currentPiece.type, newRotation))
      {
        currentPiece.rotation = newRotation;
      }

      lastRotateTime = now;
    }
  }
}

void updateFall()
{
  if (!hasActivePiece)
  {
    return;
  }

  unsigned long now = millis();

  unsigned long currentFallDelay = normalFallDelay;

  if (digitalRead(speedButtonPin) == LOW)
  {
    currentFallDelay = fastFallDelay;
  }

  if (now - lastFallTime < currentFallDelay)
  {
    return;
  }

  lastFallTime = now;

  if (canPlacePiece(currentPiece.x, currentPiece.y + 1,
                    currentPiece.type, currentPiece.rotation))
  {
    currentPiece.y++;
    return;
  }

  lockCurrentPiece();
  hasActivePiece = false;
  placedPiecesCount++;

  if (placedPiecesCount % 3 == 0 && normalFallDelay > minFallDelay)
  {
    if (normalFallDelay - fallAccelerationStep < minFallDelay)
    {
      normalFallDelay = minFallDelay;
    }
    else
    {
      normalFallDelay -= fallAccelerationStep;
    }
  }

  clearFullLines();
  spawnNewPiece();
}

void lockCurrentPiece()
{
  for (int localY = 0; localY < 4; localY++)
  {
    for (int localX = 0; localX < 4; localX++)
    {
      if (!pieceCellIsFilled(currentPiece.type, currentPiece.rotation,
                             localX, localY))
      {
        continue;
      }

      int worldX = currentPiece.x + localX;
      int worldY = currentPiece.y + localY;

      if (isInside(worldX, worldY))
      {
        board[worldY][worldX] = currentPiece.colourId;
      }
    }
  }
}

void clearFullLines()
{
  for (int y = gridHeight - 1; y >= 0; y--)
  {
    if (isLineFull(y))
    {
      removeLine(y);
      y++;
    }
  }
}

bool isLineFull(int y)
{
  for (int x = 0; x < gridWidth; x++)
  {
    if (board[y][x] == 0)
    {
      return false;
    }
  }

  return true;
}

void removeLine(int lineY)
{
  for (int y = lineY; y > 0; y--)
  {
    for (int x = 0; x < gridWidth; x++)
    {
      board[y][x] = board[y - 1][x];
    }
  }

  for (int x = 0; x < gridWidth; x++)
  {
    board[0][x] = 0;
  }
}

void renderGame()
{
  pixels.clear();

  drawBoardOnly();

  if (hasActivePiece)
  {
    drawCurrentPiece();
  }

  pixels.show();
}

void drawBoardOnly()
{
  for (int y = 0; y < gridHeight; y++)
  {
    for (int x = 0; x < gridWidth; x++)
    {
      setPixelXY(x, y, getColourFromId(board[y][x]));
    }
  }
}

void drawCurrentPiece()
{
  for (int localY = 0; localY < 4; localY++)
  {
    for (int localX = 0; localX < 4; localX++)
    {
      if (!pieceCellIsFilled(currentPiece.type, currentPiece.rotation,
                             localX, localY))
      {
        continue;
      }

      int worldX = currentPiece.x + localX;
      int worldY = currentPiece.y + localY;

      setPixelXY(worldX, worldY, getColourFromId(currentPiece.colourId));
    }
  }
}

void setPixelXY(int x, int y, uint32_t colour)
{
  if (!isInside(x, y))
  {
    return;
  }

  pixels.setPixelColor(xyToIndex(x, y), colour);
}

int xyToIndex(int x, int y)
{
  return y * gridWidth + x;
}

uint32_t getColourFromId(int id)
{
  switch (id)
  {
    case 1: return colourRed;
    case 2: return colourGreen;
    case 3: return colourBlue;
    case 4: return colourYellow;
    case 5: return colourCyan;
    case 6: return colourMagenta;
    case 7: return colourOrange;
    default: return colourOff;
  }
}
