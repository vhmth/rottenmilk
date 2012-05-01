/* The SPIMbot software is derived from James Larus's SPIM.  Modifications
	to SPIM are covered by the following license:

 Copyright © 2004, University of Illinois.  All rights reserved.

 Developed by:  Craig Zilles
                Department of Computer Science
					 University of Illinois at Urbana-Champaign
					 http://www-faculty.cs.uiuc.edu/~zilles/spimbot

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal with the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimers.

 Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimers in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the University of Illinois nor the names of its
 contributors may be used to endorse or promote products derived from
 this Software without specific prior written permission.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT.  IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
 SOFTWARE. */


#ifdef SCENARIO_HEADER

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
  int level;
  struct board_s *next;
} board_t;

typedef struct struct_ball_t {
   int num;
	double x, y;
	double drawn_x, drawn_y;
	double velocity;
	double orientation;
	bool visible;
	int disappeared_cycle;
	int kick_cycle[2];
	int no_kick_cycle[2];
	int interrupt_cycle[2];
} ball_t;

typedef struct {
	int energy;
	int score;
	int query_ball;
   int last_clue_time;
   int next_clue_interrupt_time;
	double kick_orientation;
	int kick_ball_number;
	int kick_energy;

  // SUDOKU
  sudoku_t *sudoku_head, *sudoku_tail;
  unsigned sudoku_int;
  int sudoku_inflight;
  int   get_board_num;
  int solve_board_num;

} scenario_bot_state_t;


// #define SPIMBOT_CLUE_ADDR ((mem_addr) 0xffff007c)
// #define SPIMBOT_CLUE_UNLOCK ((mem_addr) 0xffff0090)
// #define SPIMBOT_CLUE_LOCK ((mem_addr) 0xffff0094)

#define SPIMBOT_KICK_BALL ((mem_addr) 0xffff00c0)
#define SPIMBOT_KICK_ORIENTATION ((mem_addr) 0xffff00c4)
#define SPIMBOT_KICK_ENERGY ((mem_addr) 0xffff00c8)

#define SPIMBOT_QUERY_BALL ((mem_addr) 0xffff00d0)
#define SPIMBOT_BALL_X ((mem_addr) 0xffff00d4)
#define SPIMBOT_BALL_Y ((mem_addr) 0xffff00d8)
#define SPIMBOT_BALL_VELOCITY ((mem_addr) 0xffff00dc)
#define SPIMBOT_BALL_EXISTS ((mem_addr) 0xffff00e0)
#define SPIMBOT_BALL_CAN_BE_KICKED ((mem_addr) 0xffff00e4)

#define SPIMBOT_BALL_ACK ((mem_addr) 0xffff0064)
// #define SPIMBOT_CLUE_ACK ((mem_addr) 0xffff0068)

#define SB_BALL_INT_MASK 0x2000
// #define SB_CLUE_INT_MASK 0x4000

#define SPIMBOT_SUDOKU_ACK ((mem_addr) 0xffff0068)
#define SB_SUDOKU_INT_MASK 0x4000
#define SPIMBOT_SUDOKU_GET   ((mem_addr) 0xffff00e8) //write
#define SPIMBOT_SUDOKU_DONE  ((mem_addr) 0xffff00ec) //write

#define SPIMBOT_GET_SCORE       ((mem_addr) 0xffff00b0)
#define SPIMBOT_OP_SCORE        ((mem_addr) 0xffff00b4)
#define SPIMBOT_ENERGY          ((mem_addr) 0xffff00b8) // read

#define SUDOKU_BASE_LATENCY 90000
#define SUDOKU_VAR_LATENCY 20000

#else /* ! SCENARIO_HEADER */

#include <strings.h>

double my_min(double a, double b) {
  return (a<b) ? a : b;
}

double my_max(double a, double b) {
  return (a>b) ? a : b;
}

#define RANDOM_BALL_LOC ((random() % (WORLD_SIZE - 2*BALL_RADIUS)) + BALL_RADIUS)

int draw_text = 1;

// goal constants
const int GOAL_WIDTH = 3;
const int GOAL_HEIGHT = 100;
const int GOAL_BOX_WIDTH = 50;
const int GOAL_BOX_HEIGHT = 90;

// ball constants
const int BALL_RADIUS = 12;
const int BALL_MASS = 64;

// physics constants
const double AIR_RESISTANCE = 1; // .999999;
// const double DISTANCE_POWERUP = .01;

// scenario cycle level
unsigned CYCLE_LEVEL = 0x0;

const int TOTAL_BALLS = 4;
/* varies during game */ int NUM_BALLS = 1; // NUM_BALLS_SIDE * 2 + 1;
const int REAPPEAR_CYCLE = 30000;
const int KICKABLE_CYCLE = 1000;
const int NOT_KICKABLE_CYCLE = 5000;

// SUDOKU
const int ALL_VALUES = (1 << 9) - 1;
extern int thislevel;
extern int setboard[9][9];
extern int    board[9][9];
void make_board_init();
void sudoku_run();
int solve();

int ball_init_x[TOTAL_BALLS] = { 150, 150, 150, 150 };
int ball_init_y[TOTAL_BALLS] = { 150, 120, 200, 50 };
int ball_colors[TOTAL_BALLS] = { 0x00ff00, 0xffff00, 0xff00ff, 0x008000 };

// bot initial values
int bot_init_x = 50;
int bot_init_y = 150;
int bot_init_angle = 0;

ball_t balls[TOTAL_BALLS];

/*************************************************
	initialization methods
*************************************************/
extern bool g_randommap;
extern bool g_random_map;

void
ball_initialize() {
	for (int i = 0 ; i < TOTAL_BALLS ; ++ i) {
	  balls[i].num = i;
	  balls[i].drawn_x = balls[i].x = (g_randommap || g_random_map) ? RANDOM_BALL_LOC : ball_init_x[i];
	  balls[i].drawn_y = balls[i].y = (g_randommap || g_random_map) ? RANDOM_BALL_LOC : ball_init_y[i];
	}

	for (int i = 0 ; i < TOTAL_BALLS ; ++ i) {
	  balls[i].velocity = 0;
	  balls[i].orientation = 0;
	  balls[i].visible = (i < NUM_BALLS);
	  balls[i].disappeared_cycle = 0;
	  for (int j = 0 ; j < 2 ; j ++) {
		 balls[i].kick_cycle[j] = 0;
		 balls[i].no_kick_cycle[j] = 0;
		 balls[i].interrupt_cycle[j] = 0;
	  }
	}
}

void
bot_scenario_init(bot_state_t *bot) {
  bot->x = bot->context ? (WORLD_SIZE - bot_init_x) : bot_init_x;
  bot->y = bot->context ? (WORLD_SIZE - bot_init_y) : bot_init_y;
  bot->orientation = bot->context ? (bot_init_angle + M_PI) : bot_init_angle;
  bot->scenario.query_ball = bot->context ? (NUM_BALLS-1) : 0;
  bot->scenario.last_clue_time = -1000000;

  bot->scenario.score = 0;
  bot->scenario.kick_energy = 0;
  bot->scenario.energy = 100;

  // SUDOKU
  bot->scenario.get_board_num = 0;
  bot->scenario.solve_board_num = 0;
  bot->scenario.sudoku_head = NULL;
  bot->scenario.sudoku_tail = NULL;
  bot->scenario.sudoku_int = 0;
}

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
  if (SPIMBOT_TOURNAMENT) {
	 srandom(time(NULL) + clock());
  }

  sudoku_run();
  board_t *new_board = (board_t *)malloc(sizeof(board_t));
  new_board->number = g_next_board_number ++;
  new_board->latency = SUDOKU_BASE_LATENCY + (random() % SUDOKU_VAR_LATENCY);
  init_board_from_set_board(new_board->puzzle,   setboard);
  init_board_from_set_board(new_board->solution, board);
  new_board->level = thislevel;
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
  reverse_image = 1;// robot 0 code should work for robot 1
  // srandom(time(NULL));

  make_board_init();
  make_another_board();

  bot_initialize(0, /* random */ 0);
  bot_scenario_init(&robots[0]);

  bot_initialize(1, /* random */ 0);
  bot_scenario_init(&robots[1]);

  ball_initialize();
  //bot_compute_next_block_interrupt_time(&robots[0]);
  //bot_compute_next_block_interrupt_time(&robots[1]);
}

/*************************************************
	kicking and collision physics methods
*************************************************/

void
kick_ball(ball_t *ball, int energy, double orientation, int context) {
  if (spimbot_debug) {
	 printf("bot %d kick %d ball %d cycle: %d\n", context, energy, ball->num, cycle);
  }
	bot_state_t *bot = &robots[context];
	int bot_x = ((bot->context == 1) ? (WORLD_SIZE - bot->x) : bot->x);
	double v_x = ball->velocity * cos(ball->orientation);
	double v_y = ball->velocity * sin(ball->orientation);
	double v2 = (1.0 * energy / BALL_MASS) * (150 + bot_x) / 300.0;
	if (bot->context == 1) {
		v2 *= -1;
	}
	double v2_x = v2 * cos(orientation);
	double v2_y = v2 * sin(orientation);
	v_x += v2_x;
	v_y += v2_y;
	ball->velocity = sqrt(v_x * v_x + v_y * v_y);
	ball->orientation = atan2(v_y, v_x);
}

void ball_velocity_update() {
	for (int i = 0 ; i < NUM_BALLS ; ++ i)	{
		balls[i].velocity *= AIR_RESISTANCE;
		//printf("Air Resistance applied\n");
	}
}


/*************************************************
	collision handle methods
*************************************************/


const double epsilon = .02;

bool
bot_collide_ball(bot_state_t *bot, ball_t *ball) {
	if (ball->visible) {
		double diff_x = bot->x - ball->x;
		double diff_y = bot->y - ball->y;
		double distSquared = diff_x * diff_x + diff_y * diff_y;
		// double radius_minus = BOT_RADIUS + BALL_RADIUS - epsilon;
		double radius_plus = BOT_RADIUS + BALL_RADIUS + epsilon;
		// if (radius_minus * radius_minus < distSquared &&
		if (distSquared < radius_plus * radius_plus) {
			if (ball->no_kick_cycle[bot->context] == 0) {
			  ball->kick_cycle[bot->context] = 1;  // reset kick timer
			}
			return true;
		}
	}
	return false;
}

void goal_box_energy_update(bot_state_t *bot) {
	double x = ((bot->context == 1) ? (WORLD_SIZE - bot->x) : bot->x);
	if (x < GOAL_BOX_WIDTH && bot->y > (WORLD_SIZE - GOAL_BOX_HEIGHT) / 2 &&
		bot->y < (WORLD_SIZE + GOAL_BOX_HEIGHT) / 2)
	{
	  // if (bot->scenario.energy) bot->scenario.energy -= 1;
		//printf("Goal Box entered\n");
	}
}

// this method also handles scoring
void
handle_collisions_balls() {

	for (int i = 0 ; i < NUM_BALLS ; ++ i) {
		if (balls[i].x < BALL_RADIUS || balls[i].x > WORLD_SIZE - BALL_RADIUS) {
			if (balls[i].visible &&
				 (((WORLD_SIZE - GOAL_HEIGHT) / 2) < balls[i].y) &&
				 (balls[i].y < (WORLD_SIZE - (WORLD_SIZE - GOAL_HEIGHT) / 2))) {
				balls[i].visible = false;
				balls[i].y = RANDOM_BALL_LOC;
				if (balls[i].x < BALL_RADIUS) {
					++ robots[1].scenario.score;
				   balls[i].x = 150 + (RANDOM_BALL_LOC/4);
				} else {
					++ robots[0].scenario.score;
				   balls[i].x = 150 - (RANDOM_BALL_LOC/4);
				}
			}

			balls[i].orientation = M_PI - balls[i].orientation;
		}

		if (balls[i].y < BALL_RADIUS || balls[i].y > WORLD_SIZE - BALL_RADIUS)
			balls[i].orientation = - balls[i].orientation;

		// the following code makes sure that orientation is always kept between 0 and 2*pi
		if (balls[i].orientation > 2*M_PI + PI_DELTA)
			balls[i].orientation -= 2*M_PI;
		if (balls[i].orientation < 0 - PI_DELTA)
			balls[i].orientation += 2*M_PI;

		assert((balls[i].orientation <= (2*M_PI + PI_DELTA)) && (balls[i].orientation >= (0 - PI_DELTA)));
	}
}

void
handle_wall_collisions_scenario(bot_state_t *bot, double delta_x, double delta_y) {

}

int
handle_collisions_bot(int which_bot) {
  reg_image_t &reg_image = reg_images[which_bot];
  if (!INTERRUPTS_ON(reg_image)
		|| !(Status_Reg(reg_image) & SB_BALL_INT_MASK)
		|| ((Cause(reg_image) & SB_BALL_INT_MASK) != 0)) {
	 return 0;
  }

	for (int i = 0 ; i < NUM_BALLS ; ++ i)
	  if ((bot_collide_ball(&robots[which_bot], &balls[i])) &&
			(balls[i].no_kick_cycle[which_bot] == 0)  &&
			(balls[i].interrupt_cycle[which_bot] == 0)) {
		 robots[which_bot].velocity = 0;
		 balls[i].interrupt_cycle[which_bot] = KICKABLE_CYCLE;
		 //printf("Kick interrupt invoked.\n");
		 return SB_BALL_INT_MASK;
	  }
	return 0;
}





/*************************************************
	motion update methods
*************************************************/


void
ball_motion_update(){
	for (int i = 0 ; i < NUM_BALLS ; ++ i) {
	  for (int j = 0 ; j < 2 ; ++ j) {
		 if (balls[i].kick_cycle[j] != 0)
			if (balls[i].kick_cycle[j] > KICKABLE_CYCLE)
			  balls[i].kick_cycle[j] = 0;
			else
			  ++balls[i].kick_cycle[j];
		 if (balls[i].no_kick_cycle[j] != 0)
			if (balls[i].no_kick_cycle[j] > NOT_KICKABLE_CYCLE)
				balls[i].no_kick_cycle[j] = 0;
			else
				++balls[i].no_kick_cycle[j];
		 if (balls[i].interrupt_cycle[j] > 0)
			balls[i].interrupt_cycle[j] --;
	  }

		balls[i].x += VELOCITY_SCALING_FACTOR * balls[i].velocity * cos(balls[i].orientation);
		balls[i].y += VELOCITY_SCALING_FACTOR * balls[i].velocity * sin(balls[i].orientation);
		if (!balls[i].visible)
			if (balls[i].disappeared_cycle == REAPPEAR_CYCLE) {
				balls[i].visible = true;
				balls[i].disappeared_cycle = 0;
				balls[i].velocity = 0;
			}
			else
				++ balls[i].disappeared_cycle;
	}
}

int
bot_motion_update_scenario(bot_state_t *bot) {
  if ((cycle % 100000) == 0) {
	 bot->scenario.energy += 1;
  }

	handle_collisions_balls();
	// handle_collisions_bot(0);
	// handle_collisions_bot(1);
	ball_motion_update();

	ball_velocity_update();

	//decrements energy if bot is in goal box
	goal_box_energy_update(bot);

	if (bot->velocity == 0) {
		return 0;  // SUCCESS
	}
  /*int failed = handle_collisions(bot->x, bot->y, BALL_RADIUS + BOT_RADIUS - 1, -1, bot->context);

  if (failed) {
	 // bonk
	 // printf("bonk\n");
	 bot->bonk = 1;
	 bot->velocity = 0.0;
  }
  return failed;*/
	return 0;
}

// #define scenario_done (cycle > 1200000)
#define scenario_done (cycle > CYCLE_END)



/*************************************************
	scenario settings update methods
*************************************************/

void scenario_setting_update() {
  if (cycle > CYCLE_END / 4 && !(CYCLE_LEVEL & 0xf))	{
	 if (!SPIMBOT_TOURNAMENT) {
		printf("Phase 2: %d\n", cycle);
	 }
	 CYCLE_LEVEL |= 0xf;
	 balls[NUM_BALLS].visible = true;
	 NUM_BALLS += 1;
  } else if (cycle > (CYCLE_END / 2) && !(CYCLE_LEVEL & 0xf0)) {
	 if (!SPIMBOT_TOURNAMENT) {
		printf("Phase 3: %d\n", cycle);
	 }
	 CYCLE_LEVEL |= 0xf0;
	 balls[NUM_BALLS].visible = true;
	 NUM_BALLS += 1;
  } else if (cycle > (3 * CYCLE_END / 4) && !(CYCLE_LEVEL & 0xf00)) {
	 if (!SPIMBOT_TOURNAMENT) {
		printf("Phase 4: %d\n", cycle);
	 }
	 CYCLE_LEVEL |= 0xf00;
	 balls[NUM_BALLS].visible = true;
	 NUM_BALLS += 1;
  }
}




void
scenario_map_init(Display *d, Window W, GC gc) {
}


/*************************************************
	drawing methods
*************************************************/

void
draw_goals(Display *d, Window w, GC gc) {
	XSetForeground(d, gc, 0);
	XFillRectangle(d, w, gc, SCALE(1), SCALE((WORLD_SIZE - GOAL_HEIGHT) / 2), SCALE(GOAL_WIDTH), SCALE(GOAL_HEIGHT));
	XFillRectangle(d, w, gc, SCALE(WORLD_SIZE - GOAL_WIDTH), SCALE((WORLD_SIZE - GOAL_HEIGHT) / 2), SCALE(GOAL_WIDTH), SCALE(GOAL_HEIGHT));
}

void
draw_goal_box(Display *d, Window w, GC gc) {
	XSetForeground(d, gc, 0);
	XDrawRectangle(d, w, gc, SCALE(1), SCALE((WORLD_SIZE - GOAL_BOX_HEIGHT) / 2), SCALE(GOAL_BOX_WIDTH), SCALE(GOAL_BOX_HEIGHT));
	XDrawRectangle(d, w, gc, SCALE(WORLD_SIZE - GOAL_BOX_WIDTH), SCALE((WORLD_SIZE - GOAL_BOX_HEIGHT) / 2), SCALE(GOAL_BOX_WIDTH), SCALE(GOAL_BOX_HEIGHT));
}

void
draw_balls(Display *d, Window w, GC gc) {
	for (int i = 0 ; i < NUM_BALLS ; ++ i)
		XClearArea(d, w, SCALE(balls[i].drawn_x - BALL_RADIUS - 2), SCALE(balls[i].drawn_y - BALL_RADIUS - 2), SCALE(2 * BALL_RADIUS + 4), SCALE(2 * BALL_RADIUS + 4), 0);

	for (int i = 0 ; i < NUM_BALLS ; ++ i)
		if (balls[i].visible) {
			XSetForeground(d, gc, ball_colors[i]);
			balls[i].drawn_x = balls[i].x;
			balls[i].drawn_y = balls[i].y;
			XFillArc(d, w, gc, SCALE(balls[i].x - BALL_RADIUS), SCALE(balls[i].y - BALL_RADIUS), SCALE(2 * BALL_RADIUS), SCALE(2 * BALL_RADIUS), 0, 360*64);
		}
}

void
redraw_scenario(Display *d, Window w, GC gc) {
	draw_balls(d, w, gc);
	draw_goals(d, w, gc);
	draw_goal_box(d, w, gc);
}

void
scenario_set_bot_string(char *str, int i) {
	sprintf(str, "Score:%3d  Energy:%4d", robots[i].scenario.score, robots[i].scenario.energy);
}



/*************************************************
	IO methods
*************************************************/

int
scenario_write_spimbot_IO(int context, mem_addr addr, mem_word value) {
    bot_state_t *bot = &robots[context];

    switch (addr) {
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
	    bot->scenario.energy += 100 * (1 + board->level);
	    if (spimbot_debug) {
		printf("Sudoku solution succeeded, energy now %d.\n",
		       bot->scenario.energy);
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

    case SPIMBOT_QUERY_BALL:
	// If (signed)value is bogus, do nothing
	if( (0 <= value) && (value < TOTAL_BALLS) ) {
	    bot->scenario.query_ball = value;
	    bot->scenario.kick_ball_number = value;
	} // else ignore the input
	break;
    case SPIMBOT_BALL_ACK:
	Cause(reg_images[bot->context]) &= ~SB_BALL_INT_MASK;
	//printf("Kick interrupt acknowleged.\n");
	break;
    case SPIMBOT_KICK_ORIENTATION:
	if (-360 <= value && value <= 360)
	    bot->scenario.kick_orientation = value * (M_PI/180.0);
	// FIXME deal the case with the other bot
	break;
// 		case SPIMBOT_KICK_BALL:
// 			if (0 <= value && value < NUM_BALLS)
// 				bot->scenario.kick_ball_number = value;
// 			break;
    case SPIMBOT_KICK_ENERGY:
	// FIXME check for bounds issues;;;  are there any bounds issue?
	if (value > 100) { value = 100; }
	if( value < 0 )  { value = 0;   }

	if (balls[bot->scenario.kick_ball_number].kick_cycle[bot->context] > 0 &&
	    balls[bot->scenario.kick_ball_number].no_kick_cycle[bot->context] == 0) {
	    bot->scenario.kick_energy =
		value < bot->scenario.energy ? value : bot->scenario.energy;
	    bot->scenario.energy -= bot->scenario.kick_energy;
	    if (bot->scenario.kick_energy != 0) {
		kick_ball(&balls[bot->scenario.kick_ball_number],
			  bot->scenario.kick_energy,
			  bot->scenario.kick_orientation, context);
	    }
	    balls[bot->scenario.kick_ball_number].kick_cycle[bot->context] = 0;
	    balls[bot->scenario.kick_ball_number].no_kick_cycle[bot->context] = 1;
	}
	else
	    if (!quiet && !SPIMBOT_TOURNAMENT) {
		printf("Whoops, trying to kick an unkickable ball.\n");
	    }
	break;
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
        return bot->scenario.score;
      case SPIMBOT_OP_SCORE:
        return robots[!context].scenario.score;
	   case SPIMBOT_ENERGY:
		  return bot->scenario.energy;
  		case SPIMBOT_QUERY_BALL:
		  return bot->scenario.query_ball;
		case SPIMBOT_BALL_EXISTS:
		  if ((bot->scenario.query_ball >= NUM_BALLS) ||
				(bot->scenario.query_ball < 0) ||
				(!balls[bot->scenario.query_ball].visible))  {
			  return 0;
			}
		  return 1;
		case SPIMBOT_BALL_X:
			mem_word x;
			if (bot->scenario.query_ball >= NUM_BALLS || bot->scenario.query_ball < 0) {
				x = (mem_word)bot->x;
			} else {
				if (balls[bot->scenario.query_ball].visible)
					x = (mem_word)balls[bot->scenario.query_ball].x;
				else
					x = -1;
			}
			if (bot->context == 1 && x != -1) {
				x = WORLD_SIZE - x;
			}
			return x;
		case SPIMBOT_BALL_Y:
			mem_word y;
			if (bot->scenario.query_ball >= NUM_BALLS || bot->scenario.query_ball < 0) {
				y = (mem_word)bot->y;
			} else {
				if (balls[bot->scenario.query_ball].visible)
					y = (mem_word)balls[bot->scenario.query_ball].y;
				else
					y = -1;
			}
			if (bot->context == 1 && y != -1) {
				y = WORLD_SIZE - y;
			}
			return y;
// 		case SPIMBOT_BALL_VELOCITY:
// 			mem_word v;
// 			if (bot->scenario.query_ball >= NUM_BALLS || bot->scenario.query_ball < 0) {
// 				v = (mem_word)bot->velocity;
// 			} else {
// 				v = (mem_word)balls[bot->scenario.query_ball].velocity;
// 			}
// 			return v;
// 		case SPIMBOT_BALL_ORIENTATION:
// 			double o;
// 			if (bot->scenario.query_ball >= NUM_BALLS || bot->scenario.query_ball < 0) {
// 				o = (mem_word)bot->orientation;
// 			} else {
// 				o = (mem_word)balls[bot->scenario.query_ball].orientation;
// 			}
// 			if (bot->context == 1) {
// 				o += M_PI;
// 				if (o > 2*M_PI + PI_DELTA)
// 					o -= 2*M_PI;
// 				if (o < 0 - PI_DELTA)
// 					o -= 2*M_PI;
// 			}
// 			return o / M_PI * 180;
		case SPIMBOT_BALL_CAN_BE_KICKED:
			if (bot->scenario.query_ball < NUM_BALLS || bot->scenario.query_ball >= 0)
				if(balls[bot->scenario.query_ball].kick_cycle[bot->context] > 0 && balls[bot->scenario.kick_ball_number].no_kick_cycle[bot->context] == 0)
					return 1;
				else
					return 0;
			else
				return 0;
	}
	*success = 0;
	return 0;
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

// void
// write_list(int my_context, int wp_context, int base_addr) {
//   int used[64];
//   bzero((void *)used, 64 * sizeof(int));
//
//   unsigned offset = 0;
//   for (int i = 0 ; i < 5 ; i ++) {
// 	 used[offset] = used[offset+1] = used[offset+2] = 1;
// 	 checked_set_mem_word(my_context, base_addr + 4*offset + 0,
// 								 robots[wp_context].scenario.waypoint[i].x);
// 	 checked_set_mem_word(my_context, base_addr + 4*offset + 4,
// 								 robots[wp_context].scenario.waypoint[i].y);
// 	 unsigned next_offset = 0;
// 	 if (i != NUM_WAYPOINTS) {
// 		do {
// 		  next_offset = random() & 63;
// 		} while (next_offset >= 62 || (used[next_offset] != 0) ||
// 					(used[next_offset+1] != 0) || (used[next_offset+2] != 0));
// 	 }
// 	 checked_set_mem_word(my_context, base_addr + 4*offset + 8,
// 								 base_addr + 4*next_offset);
// 	 offset = next_offset;
//   }
// }

int
bot_io_update_scenario(bot_state_t *bot) {
  reg_image_t &reg_image = reg_images[bot->context];
  int cause = 0;

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

  cause |= handle_collisions_bot(bot->context);

  return cause;
}

/*************************************************
	other methods
*************************************************/

int
scenario_get_winner() {
	printf("Scores: %d %d\n", robots[0].scenario.score, robots[1].scenario.score);
	printf("Final spimbot position: %f, %f\n", robots[0].x, robots[0].y);
	return robots[1].scenario.score > robots[0].scenario.score;
}

#endif /*  SCENARIO_HEADER */
