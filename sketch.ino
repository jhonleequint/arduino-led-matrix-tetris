#include <TM1637.h>

// ============================================================
// PINS
// ============================================================
// MAX7219 LED matrices (daisy-chained)
#define CLK 13
#define DIN 11
#define CS 10

// TM1637 score display
#define TM_CLK 21
#define TM_DIO 20

// Buzzer
#define BUZZ_IN 12

// ============================================================
// DISPLAY GEOMETRY
// ============================================================
#define X_SEGMENTS 2
#define Y_SEGMENTS 4
#define SEGMENT_SIZE 8
#define NUM_SEGMENTS (X_SEGMENTS * Y_SEGMENTS)

// ============================================================
// TIMING (ms)
// ============================================================
#define MOVE_INTERVAL 500
#define KEY_PRESS_INTERVAL 100

// ============================================================
// PIECES
// ============================================================
#define NUM_PIECES 7
#define O_PIECE 0
#define I_PIECE 1
#define T_PIECE 2
#define L_PIECE 3
#define J_PIECE 4
#define S_PIECE 5
#define Z_PIECE 6

// ============================================================
// MOVES
// ============================================================
#define MOVE_LEFT 0
#define MOVE_RIGHT 1
#define ROTATE 2
#define MOVE_DOWN 3

TM1637 tm(TM_CLK, TM_DIO);

// ============================================================
// GAME STATE
// ============================================================
struct position {
  short x, y;
};

struct piece {
  short id;
  position pos[4];
} curr_piece, next_piece;

unsigned short score = 0;
short rotation = 0;
boolean lost = 0;
byte fb[SEGMENT_SIZE * NUM_SEGMENTS]; // framebuffer: one bit per LED

// ============================================================
// MAX7219 OUTPUT
// ============================================================
void shiftAll(byte send_to_address, byte send_this_data) {
  digitalWrite(CS, LOW);
  int i = NUM_SEGMENTS;
  while (i--) {
    shiftOut(DIN, CLK, MSBFIRST, send_to_address);
    shiftOut(DIN, CLK, MSBFIRST, send_this_data);
  }
  digitalWrite(CS, HIGH);
}

void show() {
  for (byte row = 0; row < SEGMENT_SIZE; row++) {
    digitalWrite(CS, LOW);
    byte segment = NUM_SEGMENTS;
    while (segment--) { // segments go from NUM_SEGMENTS - 1 down to 0
      byte x = segment % X_SEGMENTS;
      byte y = segment / X_SEGMENTS;
      byte addr = y * X_SEGMENTS * SEGMENT_SIZE + row * X_SEGMENTS;

      if (segment & X_SEGMENTS) { // odd rows of segments (mounted upside down)
        shiftOut(DIN, CLK, MSBFIRST, 8 - row);
        shiftOut(DIN, CLK, LSBFIRST, fb[addr + x]);
      } else { // even rows of segments
        shiftOut(DIN, CLK, MSBFIRST, 1 + row);
        shiftOut(DIN, CLK, MSBFIRST, fb[addr - x + X_SEGMENTS - 1]);
      }
    }
    digitalWrite(CS, HIGH);
  }
}

// ============================================================
// FRAMEBUFFER
// ============================================================
void set(short x, short y, bool state) {
  byte segmentX = x / SEGMENT_SIZE; // segment column in the matrix
  byte segmentY = y / SEGMENT_SIZE; // segment row in the matrix

  // Position inside the segment
  byte localRow = y % SEGMENT_SIZE;
  byte localCol = x % SEGMENT_SIZE;

  // Index in the framebuffer
  short int index = segmentY * SEGMENT_SIZE * X_SEGMENTS + localRow * X_SEGMENTS + segmentX;

  if (state) {
    fb[index] |= (1 << localCol);  // set the bit
  } else {
    fb[index] &= ~(1 << localCol); // clear the bit
  }
}

void setPiece(piece p, bool state) {
  for (short i = 0; i < 4; i++) {
    set(p.pos[i].x, p.pos[i].y, state);
  }
}

// ============================================================
// PIECE GENERATION
// ============================================================
piece generateNewPiece(short id) {
  piece p;
  p.id = id;

  // Each piece is drawn in a 3x3 or 4x4 box, anchored at its lower-right corner.
  // Corner of the 3x3 box:
  short x = 7;
  short y = 8 * 3 - 3;
  // The 4x4 box uses x - 1 and y - 1.

  switch (id) {
    case O_PIECE:
      p.pos[0] = {x, y + 2};
      p.pos[1] = {x, y + 1};
      p.pos[2] = {x + 1, y + 2};
      p.pos[3] = {x + 1, y + 1};
      break;

    case I_PIECE:
      x--;
      y--;
      p.pos[0] = {x + 3, y + 2};
      p.pos[1] = {x + 1, y + 2};
      p.pos[2] = {x + 2, y + 2};
      p.pos[3] = {x, y + 2};
      break;

    case T_PIECE:
      p.pos[0] = {x + 1, y + 2};
      p.pos[1] = {x + 2, y + 1};
      p.pos[2] = {x + 1, y + 1};
      p.pos[3] = {x, y + 1};
      break;

    case L_PIECE:
      p.pos[0] = {x, y + 2};
      p.pos[1] = {x + 2, y + 1};
      p.pos[2] = {x + 1, y + 1};
      p.pos[3] = {x, y + 1};
      break;

    case J_PIECE:
      p.pos[0] = {x + 2, y + 2};
      p.pos[1] = {x + 2, y + 1};
      p.pos[2] = {x + 1, y + 1};
      p.pos[3] = {x, y + 1};
      break;

    case S_PIECE:
      p.pos[0] = {x + 1, y + 2};
      p.pos[1] = {x, y + 2};
      p.pos[2] = {x + 2, y + 1};
      p.pos[3] = {x + 1, y + 1};
      break;

    case Z_PIECE:
      p.pos[0] = {x + 2, y + 2};
      p.pos[1] = {x + 1, y + 2};
      p.pos[2] = {x + 1, y + 1};
      p.pos[3] = {x, y + 1};
      break;
  }

  return p;
}

// ============================================================
// ROTATION
// ============================================================
void rotatePiece(piece& p) {
  setPiece(p, false);
  short x, y, new_rotation;
  new_rotation = rotation + 1;
  new_rotation %= 4;

  piece copy = p;
  switch (p.id) {
    case O_PIECE:
      return;

    case I_PIECE:
      if (new_rotation == 0) {
        x = p.pos[3].x - 2;
        y = p.pos[3].y;
        p.pos[0] = {x + 3, y + 2};
        p.pos[1] = {x + 1, y + 2};
        p.pos[2] = {x + 2, y + 2};
        p.pos[3] = {x, y + 2};
      } else if (new_rotation == 1) {
        x = p.pos[3].x;
        y = p.pos[3].y - 2;
        p.pos[0] = {x + 1, y + 3};
        p.pos[1] = {x + 1, y + 2};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x + 1, y};
      } else if (new_rotation == 2) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 3, y + 1};
        p.pos[1] = {x + 2, y + 1};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x, y + 1};
      } else if (new_rotation == 3) {
        x = p.pos[3].x;
        y = p.pos[3].y - 1;
        p.pos[0] = {x + 2, y + 3};
        p.pos[1] = {x + 2, y + 2};
        p.pos[2] = {x + 2, y + 1};
        p.pos[3] = {x + 2, y};
      }
      break;

    case T_PIECE:
      if (new_rotation == 0) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x + 2, y + 1};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x, y + 1};
      } else if (new_rotation == 1) {
        x = p.pos[3].x;
        y = p.pos[3].y - 1;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x, y + 1};
        p.pos[3] = {x + 1, y};
      } else if (new_rotation == 2) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 2, y + 1};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x, y + 1};
        p.pos[3] = {x + 1, y};
      } else if (new_rotation == 3) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x + 2, y + 1};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x + 1, y};
      }
      break;

    case L_PIECE:
      if (new_rotation == 0) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x, y + 2};
        p.pos[1] = {x + 2, y + 1};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x, y + 1};
      } else if (new_rotation == 1) {
        x = p.pos[3].x;
        y = p.pos[3].y - 1;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x + 1, y};
        p.pos[3] = {x, y};
      } else if (new_rotation == 2) {
        x = p.pos[3].x;
        y = p.pos[3].y;
        p.pos[0] = {x + 2, y + 1};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x, y + 1};
        p.pos[3] = {x + 2, y};
      } else if (new_rotation == 3) {
        x = p.pos[3].x - 2;
        y = p.pos[3].y;
        p.pos[0] = {x + 2, y + 2};
        p.pos[1] = {x + 1, y + 2};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x + 1, y};
      }
      break;

    case J_PIECE:
      if (new_rotation == 0) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 2, y + 2};
        p.pos[1] = {x + 2, y + 1};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x, y + 1};
      } else if (new_rotation == 1) {
        x = p.pos[3].x;
        y = p.pos[3].y - 1;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x, y + 2};
        p.pos[3] = {x + 1, y};
      } else if (new_rotation == 2) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 2, y + 1};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x, y + 1};
        p.pos[3] = {x, y};
      } else if (new_rotation == 3) {
        x = p.pos[3].x;
        y = p.pos[3].y;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x + 2, y};
        p.pos[3] = {x + 1, y};
      }
      break;

    case S_PIECE:
      if (new_rotation == 0) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x, y + 2};
        p.pos[2] = {x + 2, y + 1};
        p.pos[3] = {x + 1, y + 1};
      } else if (new_rotation == 1) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y - 1;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x, y + 1};
        p.pos[3] = {x, y};
      } else if (new_rotation == 2) {
        x = p.pos[3].x;
        y = p.pos[3].y;
        p.pos[0] = {x + 1, y + 1};
        p.pos[1] = {x, y + 1};
        p.pos[2] = {x + 2, y};
        p.pos[3] = {x + 1, y};
      } else if (new_rotation == 3) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 2, y + 2};
        p.pos[1] = {x + 2, y + 1};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x + 1, y};
      }
      break;

    case Z_PIECE:
      if (new_rotation == 0) {
        x = p.pos[3].x - 2;
        y = p.pos[3].y;
        p.pos[0] = {x + 2, y + 2};
        p.pos[1] = {x + 1, y + 2};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x, y + 1};
      } else if (new_rotation == 1) {
        x = p.pos[3].x;
        y = p.pos[3].y - 1;
        p.pos[0] = {x, y + 2};
        p.pos[1] = {x, y + 1};
        p.pos[2] = {x + 1, y + 1};
        p.pos[3] = {x + 1, y};
      } else if (new_rotation == 2) {
        x = p.pos[3].x - 1;
        y = p.pos[3].y;
        p.pos[0] = {x + 2, y + 1};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x + 1, y};
        p.pos[3] = {x, y};
      } else if (new_rotation == 3) {
        x = p.pos[3].x;
        y = p.pos[3].y;
        p.pos[0] = {x + 1, y + 2};
        p.pos[1] = {x + 1, y + 1};
        p.pos[2] = {x + 2, y + 1};
        p.pos[3] = {x + 2, y};
      }
      break;
  }

  // Undo the rotation if it would overlap something
  if (checkPieceCollision(p)) {
    p = copy;
  } else {
    rotation = new_rotation;
  }
}

// ============================================================
// COLLISION
// ============================================================
// True when the LED at (x, y) is already lit
bool checkPositionCollision(int x, int y) {
  byte segmentX = x / SEGMENT_SIZE; // segment column in the matrix
  byte segmentY = y / SEGMENT_SIZE; // segment row in the matrix

  // Position inside the segment
  byte localRow = y % SEGMENT_SIZE;
  byte localCol = x % SEGMENT_SIZE;

  // Index in the framebuffer
  short int index = segmentY * SEGMENT_SIZE * X_SEGMENTS + localRow * X_SEGMENTS + segmentX;

  return (fb[index] & (1 << localCol)) != 0;
}

bool checkPieceCollision(piece p) {
  for (short i = 0; i < 4; i++) {
    if (checkPositionCollision(p.pos[i].x, p.pos[i].y)) {
      return true;
    }
  }

  return false;
}

// ============================================================
// MOVEMENT
// ============================================================
void movePiece(piece& p, short direction) {
  setPiece(p, false);

  piece copy = p;
  if (direction == MOVE_LEFT) {
    for (short i = 0; i < 4; i++) {
      copy.pos[i].x++;
    }
  } else if (direction == MOVE_RIGHT) {
    for (short i = 0; i < 4; i++) {
      copy.pos[i].x--;
    }
  } else if (direction == MOVE_DOWN) {
    for (short i = 0; i < 4; i++) {
      copy.pos[i].y--;
    }
  }

  if (!checkPieceCollision(copy)) {
    p = copy;
    play_move_sound();
  }
}

bool reachedBottom(piece p) {
  setPiece(p, false);

  for (short i = 0; i < 4; i++) {
    p.pos[i].y--; // move a copy of the piece one row down
  }

  bool r = checkPieceCollision(p); // a collision means the piece has reached the bottom

  return r;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  tm.init();
  tm.set(BRIGHT_TYPICAL);
  Serial.begin(115200);

  pinMode(CLK, OUTPUT);
  pinMode(DIN, OUTPUT);
  pinMode(CS, OUTPUT);
  pinMode(BUZZ_IN, OUTPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);

  // MAX7219 setup
  shiftAll(0x0f, 0x00); // display test register: test mode off
  shiftAll(0x0b, 0x07); // scan limit register: digits 0 to 7
  shiftAll(0x0c, 0x01); // shutdown register: normal operation
  shiftAll(0x0a, 0x0f); // intensity register: max brightness
  shiftAll(0x09, 0x00); // decode mode register: no decode

  randomSeed(Serial.read());

  // Light everything outside the 10x20 playfield (walls and floor)
  for (int i = 0; i < X_SEGMENTS * SEGMENT_SIZE; i++) {
    for (int j = 0; j < Y_SEGMENTS * SEGMENT_SIZE; j++) {
      if (i < 3 || i >= 3 + 10 || j < 6 || j >= 6 + 20) {
        set(i, j, true);
      }
    }
  }

  curr_piece = generateNewPiece(random() % NUM_PIECES);
  next_piece = generateNewPiece(random() % NUM_PIECES);
  setNextPiece(next_piece);

  show();
}

// ============================================================
// LINES AND NEXT-PIECE PREVIEW
// ============================================================
void removeLines() {
  for (short i = 6 * 2; i < (6 + 20 - 1) * X_SEGMENTS; i += 2) {
    if (fb[i] == 0XFF && fb[i + 1] == 0XFF) {
      score++;

      for (short j = i; j < (6 + 20 - 2) * X_SEGMENTS; j++) {
        fb[j] = fb[j + 2];
      }

      i -= 2;
    }
  }
}

void setNextPiece(piece n) {
  for (short i = 8 * 4 * 2 - 6 * 2; i < 8 * 4 * 2; i++) {
    fb[i] = 0XFF;
  }

  short add_to_x = 3 - 7; // move the lower-right corner from x = 7 to x = 3
  short add_to_y = 26 - (8 * 3 - 3);

  for (short i = 0; i < 4; i++) {
    n.pos[i].x += add_to_x;
    n.pos[i].y += add_to_y;
  }

  setPiece(n, false);
}

// ============================================================
// LOOP
// ============================================================
byte a_down = 0;
byte d_down = 0;
byte w_down = 0;
byte s_down = 0;
unsigned long long curr_millis = 0;
unsigned long long prev_millis = 0;
unsigned long long prev_key_press_millis = 0;

void loop() {
  // Score on the TM1637
  tm.display(0, (score / 1000) % 10);
  tm.display(1, (score / 100) % 10);
  tm.display(2, (score / 10) % 10);
  tm.display(3, score % 10);
  if (score == 1000) score = 0;

  if (lost) {
    play_lose_sound();
    return;
  }

  curr_millis = millis();

  byte a = digitalRead(6);
  byte w = digitalRead(7);
  byte d = digitalRead(4);
  byte s = digitalRead(5);

  // Buttons
  if (curr_millis - KEY_PRESS_INTERVAL >= prev_key_press_millis) {
    prev_key_press_millis = curr_millis;
    if (a) {
      movePiece(curr_piece, MOVE_LEFT);
      setPiece(curr_piece, true);
      show();
    } else if (w) {
      rotatePiece(curr_piece);
      setPiece(curr_piece, true);
      show();
    } else if (d) {
      movePiece(curr_piece, MOVE_RIGHT);
      setPiece(curr_piece, true);
      show();
    } else if (s) {
      movePiece(curr_piece, MOVE_DOWN);
      setPiece(curr_piece, true);
      show();
    }

    a_down = a;
    w_down = w;
    d_down = d;
    s_down = s;
  }

  // Gravity
  if (prev_millis + MOVE_INTERVAL <= curr_millis) {
    prev_millis = curr_millis;
    if (reachedBottom(curr_piece)) {
      setPiece(curr_piece, true);
      removeLines();

      curr_piece = next_piece;
      rotation = 0;
      if (checkPieceCollision(curr_piece)) {
        lost = true;
        show();
        play_lose_sound();
        return;
      }

      next_piece = generateNewPiece(random() % NUM_PIECES);
      setNextPiece(next_piece);
    } else {
      movePiece(curr_piece, MOVE_DOWN);
    }
    setPiece(curr_piece, true);
    show();
  }
}
