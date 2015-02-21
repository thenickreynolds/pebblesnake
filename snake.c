#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "pebble_random.h"

// NOTES
// Resolution 144 x 168

#define APP_VERSION 1
PBL_APP_INFO_SIMPLE("Snake ala Nokia", "while(true);", APP_VERSION);

#define COLOR_FOREGROUND GColorBlack
#define COLOR_BACKGROUND GColorWhite

#define HEIGHT 15
#define WIDTH 15
#define PADDING 2
#define CELL_HEIGHT 6
#define CELL_WIDTH 6
#define BORDER_PADDING PADDING

#define TICK_TIME_MS 200
#define TICK_TIME_COOKIE 1337

#define X_TOP 12
#define Y_TOP 12

#define NUM_CELLS (HEIGHT * WIDTH)

typedef struct {
  unsigned short x;
  unsigned short y;
} Position;
#define Position(x, y) ((Position){(x), (y)})

#define SNAKE_HEAD_START_X 10
#define SNAKE_HEAD_START_Y 10

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3
#define DIRECTION_COUNT (LEFT + 1)

#define CLOCKWISE -1
#define COUNTER_CLOCKWISE 1

#define SNAKE_MAX_LENGTH NUM_CELLS

typedef struct {
  Position body[SNAKE_MAX_LENGTH];
  unsigned short direction;
  unsigned short length;
} Snake;
#define head(s) (s.body[0])
#define tail(s) (s.body[s.length - 1])

Window window;
Layer gameLayer;
// TODO border layer
// TODO score layer

Snake snake;
Position fruit;
unsigned short game_running;

void draw_rect(GContext *ctx, GPoint topLeft, unsigned short width, unsigned short height) {
  GPoint topRight = GPoint(topLeft.x + height, topLeft.y);
  GPoint bottomLeft = GPoint(topLeft.x, topLeft.y + width);
  GPoint bottomRight = GPoint(topLeft.x + height, topLeft.y + width);

  graphics_context_set_stroke_color(ctx, COLOR_FOREGROUND);
  graphics_draw_line(ctx, topLeft, topRight);
  graphics_draw_line(ctx, topLeft, bottomLeft);
  graphics_draw_line(ctx, bottomLeft, bottomRight);
  graphics_draw_line(ctx, topRight, bottomRight);
}

GPoint get_point_from_position(Position *pos) {
  unsigned short x_start;
  x_start = X_TOP + BORDER_PADDING + pos->x * (CELL_HEIGHT + PADDING);
  unsigned short y_start;
  y_start = Y_TOP + BORDER_PADDING + pos->y * (CELL_WIDTH + PADDING);

  return GPoint(x_start, y_start);
}

void draw_fruit(GContext *ctx) {
  GPoint pos = get_point_from_position(&fruit);
  pos.x++;
  pos.y++;

  draw_rect(ctx, pos, CELL_WIDTH - 2, CELL_HEIGHT - 2);
}

void draw_snake(GContext *ctx) {
  for (int i = 0; i < snake.length; i++) {
    GPoint p = get_point_from_position(&snake.body[i]);
    draw_rect(ctx, p, CELL_WIDTH, CELL_HEIGHT);
  }
}

void draw_border(GContext *ctx) {
  unsigned short full_width = WIDTH * (CELL_WIDTH + PADDING) + PADDING + (BORDER_PADDING * 2);
  unsigned short full_height = WIDTH * (CELL_HEIGHT + PADDING) + PADDING + (BORDER_PADDING * 2);

  draw_rect(ctx, GPoint(X_TOP - BORDER_PADDING, Y_TOP - BORDER_PADDING), full_width, full_height);
}

void draw(GContext *ctx) {
  draw_fruit(ctx);
  draw_snake(ctx);
  // Put this in another layer, bad to redraw every time
  draw_border(ctx);
  layer_mark_dirty(&gameLayer);
}

// Wat.
unsigned short random() {
  return rand_cmwc() % NUM_CELLS;
}

// Generate a random position for the fruit that is not over top of the snake.
void gen_fruit_position() {
  unsigned short cell = random();
  fruit.x = cell % WIDTH;
  fruit.y = cell / WIDTH;
}

// From rib to person.
void copy_position(unsigned short to, unsigned short from) {
  snake.body[to].x = snake.body[from].x;
  snake.body[to].y = snake.body[from].y;
}

unsigned short is_same_position(Position *a, Position *b) {
  return (a->x == b->x && a->y == b->y);
}

unsigned short is_out_of_bounds(Position *pos) {
  // TODO if we change from unsigned we'll need to redo that.
  return (pos->x >= WIDTH || pos->y >= HEIGHT);
}

unsigned short is_game_over() {
  // He's out of bounds ref!
  if (is_out_of_bounds(&snake.body[0])) {
    return true;
  }

  // I think quantum physics disallows this.
  unsigned short field[NUM_CELLS] = {0};
  for (int i = 0; i < snake.length; i++) {
    unsigned short pos = snake.body[i].x * WIDTH + snake.body[i].y;

    if (field[pos]) {
      return true;
    }

    field[pos] = true;
  }

  // Is the screen full of snake?
  if (snake.length == SNAKE_MAX_LENGTH) {
    return true;
  }

  return false;
}

// Let there be life.
void tick_game() {
  if (!game_running) {
    return;
  }

  GContext *ctx = app_get_current_graphics_context();

  Position old_position;
  old_position.x = tail(snake).x;
  old_position.y = tail(snake).y;

  for (unsigned short i = snake.length - 1; i > 0; i--) {
    copy_position(i, i - 1);
  }

  if (snake.direction == UP) {
    head(snake).y--;
  } else if (snake.direction == RIGHT) {
    head(snake).x++;
  } else if (snake.direction == DOWN) {
    head(snake).y++;
  } else if (snake.direction == LEFT) {
    head(snake).x--;
  } else {
    // Can't touch this.
  }

  if (is_game_over()) {
    // Let the user know the game is done. Womp womp.
    game_running = false;
    vibes_double_pulse();
  }

  // Is the snake head on top of the fruit?
  if (is_same_position(&head(snake), &fruit)) {
    // Nom.
    vibes_short_pulse();
    gen_fruit_position();

    snake.length++;
    tail(snake) = old_position;
  }

  draw(ctx);
}

// Suit up and get him ready to go.
void init_snake() {
  snake.direction = RIGHT;

  snake.body[0].x = SNAKE_HEAD_START_X;
  snake.body[0].y = SNAKE_HEAD_START_Y;

  snake.body[1].x = SNAKE_HEAD_START_X - 1;
  snake.body[1].y = SNAKE_HEAD_START_Y;

  snake.body[2].x = SNAKE_HEAD_START_X - 2;
  snake.body[2].y = SNAKE_HEAD_START_Y;

  snake.length = 3;
}

void schedule_timer(AppContextRef ctx) {
  app_timer_send_event(ctx, TICK_TIME_MS, TICK_TIME_COOKIE);
}

// Let all be forgiven.
void reset_game() {
  // TODO - Init the score.
 
  // Create the snake.
  init_snake();

  // And god grew the first fruit on the tree of knowledge.
  gen_fruit_position();

  // Open the floodgates.
  game_running = true;
}

// DJ, spin that shit.
void init_game(AppContextRef ctx) {
  reset_game();

  // Let's set up the game timer.
  schedule_timer(ctx);
}

void update_snake_callback(Layer *me, GContext *ctx) {
  (void)me;
  draw(ctx);
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;

  if (cookie == TICK_TIME_COOKIE) {
    tick_game();
    schedule_timer(ctx);
  }
}

void change_snake_direction(short direction) {
  snake.direction = (snake.direction + DIRECTION_COUNT - direction) % DIRECTION_COUNT;
}

void handle_up_button_press(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  change_snake_direction(COUNTER_CLOCKWISE);
}

void handle_down_button_press(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  change_snake_direction(CLOCKWISE);
}

void handle_select_button_press(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  reset_game();
}

void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) handle_up_button_press;
  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) handle_down_button_press;
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) handle_select_button_press;
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  // Create the window.
  window_init(&window, "Snake");
  window_stack_push(&window, true /* Animated */);

  // Create the layers.
  layer_init(&gameLayer, window.layer.frame);
  gameLayer.update_proc = &update_snake_callback;
  layer_add_child(&window.layer, &gameLayer);

  // Register button handlers.
  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);

  init_random_with_time();

  init_game(ctx);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .timer_handler = &handle_timer
  };
  app_event_loop(params, &handlers);
}
