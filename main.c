#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <pthread.h>

#define DARK_BLUE "\033[38;5;19;48;5;19m▀▀\033[0m"
#define LIGHT_BLUE "\033[38;5;14;48;5;14m▀▀\033[0m"
#define ORANGE "\033[38;5;208;48;5;208m▀▀\033[0m"
#define PURPLE "\033[38;5;57;48;5;57m▀▀\033[0m"
#define YELLOW "\033[38;5;226;48;5;226m▀▀\033[0m"
#define RED "\033[38;5;9;48;5;9m▀▀\033[0m"
#define GREEN "\033[38;5;82;48;5;82m▀▀\033[0m"
#define WHITE "\033[38;5;15;48;5;15m▀▀\033[0m"

#define NEWLINE "\n"
#define TAB "  "

#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))

int HEIGHT = 20;
int WIDTH = 15;
int TETROMINOS_LIST_LENGTH = 100000;
int MAIN_THREAD_SLEEP_TIME = 300000;
int INPUT_THREAD_SLEEP_TIME = 150000;

struct termios orig_termios;

void disable_raw_mode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode()
{
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw_mode);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

char read_key()
{
  char c;
  read(STDIN_FILENO, &c, 1);

  if (c == '\x1b')
  { // ESC sequence
    char seq[2];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';

    if (seq[0] == '[')
    {
      switch (seq[1])
      {
      case 'A':
        return 'U'; // Up
      case 'B':
        return 'D'; // Down
      case 'C':
        return 'R'; // Right
      case 'D':
        return 'L'; // Left
      }
    }
  }
  return c;
}

/*
  CONVENTIONS

  The following straight tetromino is 0 degrees:
    ___
   |__|
   |__|
   |__|
   |__|

  The follwing T tetromino is 0 degrees:
       ___
    __|__|___
   |__|__|__|

  The following L right tetromino is at 0 degrees:
   ___
  |__|
  |__|___
  |__|__|

  The following L left tetromino is at 0 degrees:
      ___
     |__|
   __|__|
  |__|__|

  The following skew right tetromino is at 0 degrees:
      ______
   __|__|__|
  |__|__|

  The following skew left tetromino is at 0 degrees:
   ______
  |__|__|___
     |__|__|
*/

enum SquareColor
{
  LIGHT_BLUE_SQUARE = 1,
  YELLOW_SQUARE = 2,
  PURPLE_SQUARE = 3,
  ORANGE_SQAURE = 4,
  DARK_BLUE_SQAURE = 5,
  GREEN_SQUARE = 6,
  RED_SQUARE = 7,
  WHITE_SQUARE = 8
};

enum TetrominoTypes
{
  STRAIGHT = 1,
  SQUARE = 2,
  T = 3,
  L_RIGHT = 4,
  L_LEFT = 5,
  SKEW_RIGHT = 6,
  SKEW_LEFT = 7
};

typedef struct
{
  int tetrominoType;
  char *color;
  int x;
  int y;
  int angle; // 0, 90, 180, or 270
} Tetromino;

int *playingGrid;
int currentTetrominoIndex;
Tetromino *tetrominosList;

char *getColor(int color)
{
  switch (color)
  {
  case DARK_BLUE_SQAURE:
    return DARK_BLUE;
    break;
  case LIGHT_BLUE_SQUARE:
    return LIGHT_BLUE;
    break;
  case ORANGE_SQAURE:
    return ORANGE;
    break;
  case PURPLE_SQUARE:
    return PURPLE;
    break;
  case YELLOW_SQUARE:
    return YELLOW;
    break;
  case RED_SQUARE:
    return RED;
    break;
  case GREEN_SQUARE:
    return GREEN;
    break;
  default:
    return WHITE;
    break;
  }
}

int getPlayingGridIndex(int row, int column)
{
  return column * WIDTH + row;
}

void printSquare(char *color)
{
  printf(color);
}

void straightTetromino(Tetromino tetromino)
{
  int TETROMINO_LENGTH = 4;
  if (tetromino.angle == 0 || tetromino.angle == 180)
    for (int i = tetromino.y; i < tetromino.y + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(tetromino.x, i)] = tetromino.color;
  else
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(i, tetromino.y)] = tetromino.color;
}

void squareTetromino(Tetromino tetromino)
{
  playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y)] = tetromino.color;
  playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y + 1)] = tetromino.color;
  playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y)] = tetromino.color;
  playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + 1)] = tetromino.color;
}

void tTetromino(Tetromino tetromino)
{
  int TETROMINO_LENGTH = 3;
  if (tetromino.angle == 0)
  {
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y)] = tetromino.color;
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(i, tetromino.y + 1)] = tetromino.color;
  }
  else if (tetromino.angle == 180)
  {
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(i, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + 1)] = tetromino.color;
  }
  else if (tetromino.angle == 90)
  {
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(tetromino.x, i)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + 1)] = tetromino.color;
  }
  else if (tetromino.angle == 270)
  {
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y + 1)] = tetromino.color;
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(tetromino.x + 1, i)] = tetromino.color;
  }
}

void lTetrominoRight(Tetromino tetromino)
{
  int TETROMINO_LENGTH = 3;
  if (tetromino.angle == 0)
  {
    for (int i = tetromino.y; i < tetromino.y + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(tetromino.x, i)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + TETROMINO_LENGTH - 1)] = tetromino.color;
  }
  else if (tetromino.angle == 180)
  {
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y)] = tetromino.color;
    for (int i = tetromino.y; i < tetromino.y + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(tetromino.x + 1, i)] = tetromino.color;
  }
  else if (tetromino.angle == 90)
  {
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(i, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y + 1)] = tetromino.color;
  }
  else if (tetromino.angle == 270)
  {
    playingGrid[getPlayingGridIndex(tetromino.x + TETROMINO_LENGTH - 1, tetromino.y)] = tetromino.color;
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(i, tetromino.y + 1)] = tetromino.color;
  }
}

void lTetrominoLeft(Tetromino tetromino)
{
  int TETROMINO_LENGTH = 3;
  if (tetromino.angle == 0)
  {
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y + TETROMINO_LENGTH - 1)] = tetromino.color;
    for (int i = tetromino.y; i < tetromino.y + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(tetromino.x + 1, i)] = tetromino.color;
  }
  else if (tetromino.angle == 180)
  {
    for (int i = tetromino.y; i < tetromino.y + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(tetromino.x, i)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y)] = tetromino.color;
  }
  else if (tetromino.angle == 90)
  {
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y)] = tetromino.color;
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(i, tetromino.y + 1)] = tetromino.color;
  }
  else if (tetromino.angle == 270)
  {
    for (int i = tetromino.x; i < tetromino.x + TETROMINO_LENGTH; ++i)
      playingGrid[getPlayingGridIndex(i, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + TETROMINO_LENGTH - 1, tetromino.y + 1)] = tetromino.color;
  }
}

void skewTetrominoRight(Tetromino tetromino)
{
  if (tetromino.angle == 0 || tetromino.angle == 180)
  {
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 2, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y + 1)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + 1)] = tetromino.color;
  }
  else if (tetromino.angle == 90 || tetromino.angle == 270)
  {
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y + 1)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + 1)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + 2)] = tetromino.color;
  }
}

void skewTetrominoLeft(Tetromino tetromino)
{
  if (tetromino.angle == 0 || tetromino.angle == 180)
  {
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + 1)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 2, tetromino.y + 1)] = tetromino.color;
  }
  else if (tetromino.angle == 90 || tetromino.angle == 270)
  {
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y + 1)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x + 1, tetromino.y + 1)] = tetromino.color;
    playingGrid[getPlayingGridIndex(tetromino.x, tetromino.y + 2)] = tetromino.color;
  }
}

void drawTetromino(Tetromino tetromino)
{
  switch (tetromino.tetrominoType)
  {
  case STRAIGHT:
    straightTetromino(tetromino);
    break;
  case SQUARE:
    squareTetromino(tetromino);
    break;
  case T:
    tTetromino(tetromino);
    break;
  case L_RIGHT:
    lTetrominoRight(tetromino);
    break;
  case L_LEFT:
    lTetrominoLeft(tetromino);
    break;
  case SKEW_RIGHT:
    skewTetrominoRight(tetromino);
    break;
  case SKEW_LEFT:
    skewTetrominoLeft(tetromino);
    break;
  default:
    break;
  }
}

void displayPlayingGrid()
{
  for (int i = 0; i < HEIGHT; ++i)
  {
    for (int j = 0; j < WIDTH; ++j)
      printf("%d ", playingGrid[getPlayingGridIndex(j, i)]);
    printf(NEWLINE);
  }
}

void clearPlayingGrid()
{
  for (int i = 0; i < HEIGHT; ++i)
    for (int j = 0; j < WIDTH; ++j)
      playingGrid[getPlayingGridIndex(j, i)] = WHITE;
}

void drawMatrix()
{
  gotoxy(0, 0);
  for (int i = 0; i < HEIGHT; ++i)
  {
    printf("\033[38;5;15m| \033[0m");
    for (int j = 0; j < WIDTH; ++j)
      printSquare(getColor(playingGrid[getPlayingGridIndex(j, i)]));
    printf("\033[38;5;15m |\033[0m");
    printf(NEWLINE);
  }
  printf(TAB);
  for (int i = 0; i < WIDTH * 2; ++i)
    printf("\033[38;5;15m=\033[0m");
  printf(NEWLINE);
}

int findFirstEmptyIndexInTetrominosList()
{
  for (int i = 0; i < TETROMINOS_LIST_LENGTH; ++i)
    if (tetrominosList[i].tetrominoType == 0)
      return i;
}

int addTetrominoToTetrominosList(Tetromino tetromino)
{
  int firstEmptyIndex = findFirstEmptyIndexInTetrominosList();
  tetrominosList[firstEmptyIndex] = tetromino;
  return firstEmptyIndex;
}

Tetromino getRandomTetromino()
{
  srand(time(NULL));
  // Random number in the range [1, 7]
  int randomTetrominoNumber = rand() % 7 + 1;

  srand(time(NULL));
  // Random number in the range [0, WIDTH - 3]
  int randomX = rand() % (WIDTH - 3) + 1;

  Tetromino randomTetromino;
  randomTetromino.x = randomX;
  randomTetromino.y = 0;
  randomTetromino.angle = 0;

  switch (randomTetrominoNumber)
  {
  case STRAIGHT:
    randomTetromino.tetrominoType = STRAIGHT;
    randomTetromino.color = LIGHT_BLUE_SQUARE;
    break;
  case SQUARE:
    randomTetromino.tetrominoType = SQUARE;
    randomTetromino.color = YELLOW_SQUARE;
    break;
  case T:
    randomTetromino.tetrominoType = T;
    randomTetromino.color = PURPLE_SQUARE;
    break;
  case L_RIGHT:
    randomTetromino.tetrominoType = L_RIGHT;
    randomTetromino.color = ORANGE_SQAURE;
    break;
  case L_LEFT:
    randomTetromino.tetrominoType = L_LEFT;
    randomTetromino.color = DARK_BLUE_SQAURE;
    break;
  case SKEW_RIGHT:
    randomTetromino.tetrominoType = SKEW_RIGHT;
    randomTetromino.color = GREEN_SQUARE;
    break;
  case SKEW_LEFT:
    randomTetromino.tetrominoType = SKEW_LEFT;
    randomTetromino.color = RED_SQUARE;
    break;
  default:
    break;
  }

  printf("type: %d\n", randomTetromino.tetrominoType);
  printf("x: %d\n", randomTetromino.x);
  return randomTetromino;
}

void addTetrominosFromListToMatrix()
{
  for (int i = 0; i < TETROMINOS_LIST_LENGTH; ++i)
  {
    if (tetrominosList[i].tetrominoType == 0)
      break;
    drawTetromino(tetrominosList[i]);
  }
}

void update()
{
  clearPlayingGrid();
  addTetrominosFromListToMatrix();
  drawMatrix();
}

char getch()
{
  char buf = 0;
  struct termios old = {0};
  if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 1;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ICANON");
  if (read(0, &buf, 1) < 0)
    perror("read()");
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror("tcsetattr ~ICANON");
  return (buf);
}

void *inputThreadCallback(void *arg)
{
  while (true)
  {
    char key = read_key();

    switch (key)
    {
    case 'U':
      printf("UP ARROW\n");
      break;
    case 'D':
      ++tetrominosList[currentTetrominoIndex].y;
      break;
    case 'L':
      --tetrominosList[currentTetrominoIndex].x;
      break;
    case 'R':
      ++tetrominosList[currentTetrominoIndex].x;
      break;
    }
    usleep(INPUT_THREAD_SLEEP_TIME);
  }

  return NULL;
}

int main()
{
  playingGrid = malloc(sizeof(int[WIDTH][HEIGHT]));
  tetrominosList = (Tetromino *)malloc(sizeof(Tetromino) * TETROMINOS_LIST_LENGTH);

  // Tetromino tetromino;
  // tetromino.x = 0;
  // tetromino.y = 0;
  // tetromino.angle = 0;
  // tetromino.tetrominoType = SKEW_RIGHT;
  // tetromino.color = GREEN_SQUARE;
  // drawTetromino(tetromino);

  enable_raw_mode();

  pthread_t inputThread;
  pthread_create(&inputThread, NULL, inputThreadCallback, NULL);

  Tetromino currentTetromino = getRandomTetromino();
  currentTetrominoIndex = addTetrominoToTetrominosList(currentTetromino);
  system("clear");
  while (true)
  {
    update();
    ++tetrominosList[currentTetrominoIndex].y;
    usleep(MAIN_THREAD_SLEEP_TIME);
  }

  free(playingGrid);
  free(tetrominosList);
  return 0;
}
