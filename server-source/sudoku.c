typedef struct sudoku {
  int sudoku_number, timer;
  mem_addr addr;
  struct sudoku *next;
} sudoku_t;

typedef struct board_s {
  int number;
  int latency;
  int puzzle[9][9];
  int solution[9][9];
  struct board_s *next;
} board_t;

typedef struct {
  int waypoints_visited;
  waypoint_t waypoint[NUM_WAYPOINTS];
  unsigned time_of_first_waypoint;

  sudoku_t *sudoku_head, *sudoku_tail;
  unsigned sudoku_int;
  int sudoku_inflight;
  int   get_board_num;
  int solve_board_num;
  unsigned waypoint_loc;
  int credit;
  
  speed_t *speed_head, *speed_tail;
} scenario_bot_state_t;

#define SPIMBOT_GET_WAYPOINT    ((mem_addr) 0xffff0050)
#define SPIMBOT_OP_WAYPOINT     ((mem_addr) 0xffff0054)
#define SPIMBOT_GET_SCORE       ((mem_addr) 0xffff0058)
#define SPIMBOT_OP_SCORE        ((mem_addr) 0xffff005c)

#define SPIMBOT_ENEMY_X   ((mem_addr) 0xffff00a0)
#define SPIMBOT_ENEMY_Y   ((mem_addr) 0xffff00a4)

#define SPIMBOT_SUDOKU_ACK ((mem_addr) 0xffff0064)

#define SB_SUDOKU_INT_MASK 0x2000

#define SPIMBOT_SUDOKU_GET   ((mem_addr) 0xffff00e0) //write
#define SPIMBOT_SUDOKU_DONE  ((mem_addr) 0xffff00e4) //write
#define SPIMBOT_SPEND_CREDIT ((mem_addr) 0xffff00f0) //write
#define SPIMBOT_CREDIT       ((mem_addr) 0xffff00f4) // read

#else /* ! SCENARIO_HEADER */

#define SUDOKU_BASE_LATENCY 90000
#define SUDOKU_VAR_LATENCY 20000
#define SPEED_BUMP_TIME 100000

#include <math.h>

int draw_text = 1;
// scan_t *scans_to_draw = NULL;

const int ALL_VALUES = (1 << 9) - 1;
extern int thislevel;
extern int setboard[9][9];
extern int    board[9][9];
void make_board_init();
void sudoku_run();
int solve();

const int WP_RADIUS = 1.5*BOT_RADIUS;

void
waypoint_initialize(waypoint_t *w, int number, int prev_x, int prev_y, int distance) {
  do {
	 float angle = random() * .0001;
	 distance *= .85 + .003*(random() % 100);
	 w->x = prev_x + (distance * cos(angle));
	 w->y = prev_y + (distance * sin(angle));
	 // printf("%d: %f %f %f\n", number, angle, (float) cos(angle), (float) sin(angle));
  } while ((w->x < BOT_RADIUS) || (w->x > (WORLD_SIZE - BOT_RADIUS)) ||
			  (w->y < BOT_RADIUS) || (w->y > (WORLD_SIZE - BOT_RADIUS)));

  if (spimbot_debug) {
	 printf("waypoint[%d]: (%d, %d)\n", number, (int)w->x, (int)w->y);
  }
}

void 
bot_scenario_init(bot_state_t *bot) {
  int prev_x = bot->x, prev_y = bot->y;
  bot->scenario.waypoints_visited = 0;
  for (int i = 0 ; i < NUM_WAYPOINTS ; i ++) {
	 waypoint_initialize(&bot->scenario.waypoint[i], i, prev_x, prev_y, DEFAULT_DISTANCE);
	 prev_x = bot->scenario.waypoint[i].x;
	 prev_y = bot->scenario.waypoint[i].y;
  }
  bot->scenario.time_of_first_waypoint = (unsigned) -1;
  bot->scenario.credit = 10; 
  bot->scenario.get_board_num = 0;
  bot->scenario.solve_board_num = 0;
  bot->scenario.sudoku_head = NULL;
  bot->scenario.sudoku_tail = NULL;
  bot->scenario.sudoku_int = 0;

  bot->scenario.speed_head = NULL;
  bot->scenario.speed_tail = NULL;

  bot->scenario.waypoint_loc = ((random() % 300) << 16) + (random() % 300);
}

#define MAXRATE 5

board_t *g_board_head = NULL, *g_board_tail = NULL;
int g_next_board_number = 0;

void
init_board_from_set_board(int to_board[9][9], int from_board[9][9]) {
  for (int i = 0 ; i < 9 ; ++ i) {
	 for (int j = 0 ; j < 9 ; ++ j) {
		int c = from_board[i][j];
		int value = (c == 0) ? ALL_VALUES : (1 << (c - 1));  
		to_board[i][j] = value;
	 }
  }
}

void
make_another_board() {
  sudoku_run();
  board_t *new_board = (board_t *)malloc(sizeof(board_t));
  new_board->number = g_next_board_number ++;
  new_board->latency = SUDOKU_BASE_LATENCY + (random() % SUDOKU_VAR_LATENCY);
  init_board_from_set_board(new_board->puzzle,   setboard);
  init_board_from_set_board(new_board->solution, board);
  new_board->next = NULL;

  // insert into list
  if (g_board_tail == NULL) {
	 assert(g_board_head == NULL);
	 g_board_head = new_board;
  } else {
	 assert(g_board_tail->next == NULL);
	 g_board_tail->next = new_board;
  }
  g_board_tail = new_board;
}

board_t *get_board(int board_num) {
  if (board_num == g_next_board_number) {
	 make_another_board();
	 assert(g_board_tail->number == board_num);
	 return g_board_tail;
  }		

  board_t *board = g_board_head; 
  while (board != NULL && board->number != board_num) {
	 // assert(board->next != NULL); // better be another board
	 board = board->next;
  }
  return board;
}

void
world_initialize() {
  srandom(time(NULL));
  make_board_init();
  
  make_another_board();

  bot_initialize(0, /* random */ 1);
  bot_scenario_init(&robots[0]);

  bot_initialize(1, /* random */ 1);
  bot_scenario_init(&robots[1]);
}

void
world_update_scenario() {  
  // nothing moves on its own in this scenario
}

void 
bot_motion_update_scenario(bot_state_t *bot) {
  // bot_waypoint_detection
  waypoint_t *current_waypoint = 
	 &bot->scenario.waypoint[bot->scenario.waypoints_visited % NUM_WAYPOINTS];
  
  if ((fabs(bot->x - current_waypoint->x) < 2) && 
		(fabs(bot->y - current_waypoint->y) < 2)) {
	 // found our own next waypoint
	 int prev_waypoint_index = 
		(bot->scenario.waypoints_visited + NUM_WAYPOINTS - 1) % NUM_WAYPOINTS;
	 waypoint_t *prev_waypoint = &bot->scenario.waypoint[prev_waypoint_index];
	 waypoint_initialize(current_waypoint, 
								bot->scenario.waypoints_visited + NUM_WAYPOINTS, 
								prev_waypoint->x, prev_waypoint->y, DEFAULT_DISTANCE);
	 if (spimbot_debug) {
		printf("WAYPOINT %d\n", bot->scenario.waypoints_visited);
	 }
	 bot->scenario.waypoints_visited ++;
	 if (SPIMBOT_TOURNAMENT && 
		  (bot->scenario.waypoints_visited > 
			  robots[!bot->context].scenario.waypoints_visited) && 
		  robots[!bot->context].done) {
		bot->done = 1;
	 }
  }
	 
  waypoint_t *opponent_waypoint = robots[!bot->context].scenario.waypoint;
  for (int i = 0 ; i < NUM_WAYPOINTS ; ++ i) {
	 if ((fabs(bot->x - opponent_waypoint[i].x) < 2) && 
		  (fabs(bot->y - opponent_waypoint[i].y) < 2)) {
		// got to an opponent's token; figure out if it is the next or not.
		if ((robots[!bot->context].scenario.waypoints_visited / NUM_WAYPOINTS) == i) {
		  // if so, move it far from the robot
		  waypoint_initialize(&opponent_waypoint[i], -i, 
									 robots[!bot->context].x, robots[!bot->context].y, LONG_DISTANCE);
		} else {
		  // if not, move it far from the previous waypoint.
		  waypoint_t *prev_waypoint = &opponent_waypoint[(i + NUM_WAYPOINTS) / NUM_WAYPOINTS];
		  waypoint_initialize(&opponent_waypoint[i], -i, 
									 prev_waypoint->x, prev_waypoint->y, LONG_DISTANCE);
		}
	 }
  }
}

unsigned lower_bound = 0;
unsigned upper_bound = 0;

void
checked_set_mem_word(int context, unsigned address, unsigned value) {
//  if ((address < lower_bound) || (address >= upper_bound)) {
//	 printf("overflow: %x %d %d\n", address, lower_bound, upper_bound);
//  }
  
  SET_MEM_WORD(context, address, value);
}

void
write_list(int my_context, int wp_context, int base_addr) {
  int used[64];
  bzero((void *)used, 64 * sizeof(int));

  unsigned offset = 0;
  for (int i = 0 ; i < 5 ; i ++) {
	 used[offset] = used[offset+1] = used[offset+2] = 1;
	 checked_set_mem_word(my_context, base_addr + 4*offset + 0,
								 robots[wp_context].scenario.waypoint[i].x);
	 checked_set_mem_word(my_context, base_addr + 4*offset + 4,
								 robots[wp_context].scenario.waypoint[i].y);
	 unsigned next_offset = 0;
	 if (i != NUM_WAYPOINTS) {
		do { 
		  next_offset = random() & 63;
		} while (next_offset >= 62 || (used[next_offset] != 0) || 
					(used[next_offset+1] != 0) || (used[next_offset+2] != 0));
	 }
	 checked_set_mem_word(my_context, base_addr + 4*offset + 8,
								 base_addr + 4*next_offset);
	 offset = next_offset;
  }
}

int
bot_io_update_scenario(bot_state_t *bot) {
  reg_image_t &reg_image = reg_images[bot->context];
  int cause = 0;
  if ((bot->scenario.speed_head != NULL) &&  
		(bot->scenario.speed_head->timer < cycle)) {
	 // remove the speed token
	 speed_t *s = bot->scenario.speed_head;
	 if (bot->scenario.speed_tail == s) {
		bot->scenario.speed_tail = NULL;
	 }
	 bot->scenario.speed_head = s->next;
	 free(s);

	 // reduce the velocity if it exceeds the max
	 int max_velocity = 10;
	 for (speed_t *s = bot->scenario.speed_head ; s != NULL ; s = s->next) {
		max_velocity ++;
	 }
	 bot->velocity = min(bot->velocity, max_velocity);
  }

  if ((bot->scenario.sudoku_head != NULL) &&  
		(bot->scenario.sudoku_head->timer < cycle)) {
	 // do sudoku here, first find the board
	 board_t *board = get_board(bot->scenario.sudoku_head->sudoku_number);
	 if (board != NULL) {
		// copy the data into memory
		mem_addr saddr = bot->scenario.sudoku_head->addr;
		for (int i = 0 ; i < 9 ; ++ i) {
		  for (int j = 0 ; j < 9 ; ++ j) {
			 int value = board->puzzle[i][j];
			 checked_set_mem_word(bot->context, saddr, value);
			 saddr += 4;
		  }
		}
	 } else if (!SPIMBOT_TOURNAMENT) {
	    printf("Sudoku puzzle has been attempted to be solved before getting.\n");
	 }


	 // enable the interrupt
	 bot->scenario.sudoku_int = 1;

	 // remove from the list
	 sudoku_t *s = bot->scenario.sudoku_head;
	 if (bot->scenario.sudoku_tail == s) {
		bot->scenario.sudoku_tail = NULL;
	 }
	 bot->scenario.sudoku_head = s->next;
	 free(s);
  }

  if (bot->scenario.sudoku_int && 
		INTERRUPTS_ON(reg_image) &&
		(Status_Reg(reg_image) & SB_SUDOKU_INT_MASK)) {
	 cause |= SB_SUDOKU_INT_MASK;
	 bot->scenario.sudoku_int = 0;
  }

  return cause;
}

#define scenario_done (cycle > 1200000000)   

int 
scenario_get_winner() {
  int winner = -1;
  printf("cycle: %d\n", cycle);
  winner = (robots[0].scenario.waypoints_visited > 
				robots[1].scenario.waypoints_visited) ? 0 : winner;
  winner = (robots[1].scenario.waypoints_visited > 
				robots[0].scenario.waypoints_visited) ? 1 : winner;
  // if ((winner == -1) && 
  // 		(robots[1].scenario.time_of_first_token != 
  // 		 (robots[0].scenario.time_of_first_token))) {
  // 	 winner = (robots[1].scenario.time_of_first_token < 
  // 				  robots[0].scenario.time_of_first_token) ? 1 : 0;
  // }

  return winner;
}


void
scenario_map_init(Display *d, Window W, GC gc) {
}

void
draw_waypoints(Display *d, Window w, GC gc, bot_state_t *bot) {
  for (int i = 0 ; i < NUM_WAYPOINTS ; ++ i) {
	 waypoint_t *wp = &bot->scenario.waypoint[i];
	 int wp_number = (i < (bot->scenario.waypoints_visited % NUM_WAYPOINTS)) ? i + NUM_WAYPOINTS : i;
	 wp_number = (wp_number + ((bot->scenario.waypoints_visited/NUM_WAYPOINTS) * NUM_WAYPOINTS)) % 10;

	 char name_string[2] = "0";
	 name_string[0] = '0' + wp_number;

	 XDrawArc(d, w, gc,
				 SCALE(((int)wp->x) - WP_RADIUS), SCALE(((int)wp->y) - WP_RADIUS), 
				 SCALE(2*WP_RADIUS), SCALE(2*WP_RADIUS), 0, 360*64);
	 XDrawString(d, w, gc, SCALE((int)wp->x - WP_RADIUS/3), SCALE((int)wp->y + 3*WP_RADIUS/4), name_string, 1);
  }
}

void
redraw_scenario(Display *d, Window W, GC gc) {
  XSetForeground(d, gc, 0xffffff);
  XSetForeground(d, gc, 0);
  for (int i = 0 ; i < num_contexts ; ++ i) {
	 XSetForeground(d, gc, robots[i].color);
	 draw_waypoints(d, W, gc, &robots[i]);
	 int velocity_excess = max(0, robots[i].velocity - 10);
	 if (velocity_excess > 10) {
		printf("velocity excess = %d\n", velocity_excess);
	 }
	 XFillRectangle(d, W, gc, SCALE(WORLD_SIZE - 10*velocity_excess), 
						 SCALE(WORLD_SIZE) + 20 + (i*20), 
						 SCALE(10*velocity_excess), SCALE(20));
  }
}

void
scenario_set_bot_string(char *str, int i) {
  sprintf(str, "%d $%d", robots[i].scenario.waypoints_visited, robots[i].scenario.credit);
}

int
scenario_write_spimbot_IO(int context, mem_addr addr, mem_word value) {
  bot_state_t *bot = &robots[context];
  switch (addr) {
  case SPIMBOT_GET_WAYPOINT:
	 if ((bot->scenario.credit >= 1) &&
		  (value >= bot->scenario.waypoints_visited) &&
		  (value < (bot->scenario.waypoints_visited + NUM_WAYPOINTS))) {
		waypoint_t *w = &bot->scenario.waypoint[value % NUM_WAYPOINTS];
		assert((w->y >= 0) && (w->y <= 300));
		assert((w->x >= 0) && (w->x <= 300));
		bot->scenario.waypoint_loc = (((int)w->y) << 16) | ((int)w->x);
		bot->scenario.credit --;
	 } else {
		if (spimbot_debug) {
		  if (bot->scenario.credit <= 0) {
			 printf("request for waypoint location with no credit\n");
		  } else {
			 printf("request for waypoint location with invalid waypoint number\n");
		  }
		}
		bot->scenario.waypoint_loc = ((random() % 300) << 16) + (random() % 300);
	 }
 	 break;
  case SPIMBOT_OP_WAYPOINT:
	 write_list(context, !context, value);
 	 break;

  case SPIMBOT_SUDOKU_ACK:
	 Cause(reg_images[bot->context]) &= ~SB_SUDOKU_INT_MASK;
	 break;

  case SPIMBOT_SUDOKU_GET: {
	 // writing to this address requests a new puzzle to be generated
	 // and placed at location specified by value
 
	 sudoku_t *puzzle = (sudoku_t *) malloc(sizeof(sudoku_t));
	 puzzle->sudoku_number = bot->scenario.get_board_num ++;
	 board_t *board = get_board(puzzle->sudoku_number);
	 puzzle->timer = (board != NULL) ? cycle + board->latency : cycle;
	 puzzle->addr = value;
	 puzzle->next = NULL;
	 if (bot->scenario.sudoku_head == NULL) {
		bot->scenario.sudoku_head = puzzle;
	 } else {
		bot->scenario.sudoku_tail->next = puzzle;
	 }
	 bot->scenario.sudoku_tail = puzzle;
	 
	 break;
  }
 
  case SPIMBOT_SUDOKU_DONE: {
	//value holds pointer to solution
	
	if (bot->scenario.solve_board_num == bot->scenario.get_board_num) {
	  if (!SPIMBOT_TOURNAMENT) {
	    printf("No sudoku puzzle has been requested\n");
	  }
	  break;
	}

	if (value == 0) {
	  if (!SPIMBOT_TOURNAMENT) { 
	    printf("Can't read solution from address 0\n");
	  }
	  break;
	}

	//Read solution
	board_t *board = get_board(bot->scenario.solve_board_num ++);
	mem_addr saddr = value; 
	bool solved = true;
	for (int i = 0 ; solved && (i < 9) ; ++ i) {
	  for (int j = 0 ; j < 9 ; ++ j) {
		 int temp;
		 READ_MEM_WORD(bot->context, temp, saddr);
		 saddr += 4;
		 if (temp != board->solution[i][j]) {
			solved = false;
			if (!SPIMBOT_TOURNAMENT) { 
			  printf("solution failed on a[%d][%d]\n", i, j);
			}
			break;
		 }
	  }
	}

	// handle correct solution
	if (solved) {
	  bot->scenario.credit ++;
	  if (spimbot_debug) {
		 printf("Sudoku solution succeeded, credits now %d.\n",
				  bot->scenario.credit);
	  }
	}
	
	// remove the board when we don't need it anymore.
	if ((num_contexts == 1) || 
		 (robots[!bot->context].scenario.solve_board_num >=
		  bot->scenario.solve_board_num)) {
	  assert(board == g_board_head);
	  g_board_head = board->next;
	  free(board);
	  if (g_board_tail == board) {
		 g_board_tail = NULL;
	  }
	}
	break;
  }

  case SPIMBOT_SPEND_CREDIT: {
	 if (bot->scenario.credit == 0) {
		if (spimbot_debug) {
		  printf("Tried to spend, but no credits available.\n");
		}
	 } else if (bot->velocity >= 20) {
		if (spimbot_debug) {
		  printf("Already going at peak speed.\n");
		}
	 } else {
		speed_t *s = (speed_t *) malloc(sizeof(speed_t));
		s->timer = cycle + SPEED_BUMP_TIME;
		s->next = NULL;
		if (bot->scenario.speed_head == NULL) {
		  bot->scenario.speed_head = s;
		} else {
		  bot->scenario.speed_tail->next = s;
		}
		bot->scenario.speed_tail = s;
		bot->velocity += 1.0;
		bot->scenario.credit --;
	 }
	 break;
  }

  default:
	 return 0;
  }
  return 1;
}

mem_word
scenario_read_spimbot_IO(int context, mem_addr addr, int *success) {
  *success = 1;
  bot_state_t *bot = &robots[context];
  switch (addr) {
  case SPIMBOT_GET_SCORE: 
    return bot->scenario.waypoints_visited;
  case SPIMBOT_OP_SCORE: 
    return robots[!context].scenario.waypoints_visited;
  case SPIMBOT_GET_WAYPOINT:
    return bot->scenario.waypoint_loc;
  case SPIMBOT_ENEMY_X:
    return mem_word(int(robots[!context].x));
  case SPIMBOT_ENEMY_Y:
    return mem_word(int(robots[!context].y));
  case SPIMBOT_CREDIT: 
    return bot->scenario.credit;
  
  }
  *success = 0;
  return 0;
}

void
handle_wall_collisions_scenario(bot_state_t *bot, double delta_x, double delta_y) {
  // do nothing;
}

#endif /*  SCENARIO_HEADER */
