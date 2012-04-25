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

#define __STDC_LIMIT_MACROS
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#ifdef __DARWIN__
#include <stdint.h>
#define SPIMBOT_MAXINT_32 INT32_MAX
#else
#include <values.h>
#define SPIMBOT_MAXINT_32 MAXINT
#endif
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "spim.h"
#include "reg.h"
#include "inst.h"
#include "mem.h"
#include "robot.h"
// #include "walls.h"

void mem_dump_profile();

// defined in xspim.c
extern int spimbot_debug;/* Non-Zero => additional spimbot specific messages */
extern int spimbot_largemap;/* Non-Zero => display large map window */

// Xwindows stuff
Widget mapshell;
Widget mappane;
Widget map = NULL;
GC robot_gc[2];

// for scaling the map window
float drawing_scaling = 1.0;
#define SCALE(X) ((int)(drawing_scaling * (X)))

// to allow user to interrupt (fixme: does this still do anything?)
int USER_STOP = 0;
void spimbot_map_click() { USER_STOP = 1; }

// spimBOT world constants
const int WORLD_SIZE = 300;
const int LABEL_SPACE = 50;
const int BOT_RADIUS = 5;
const int BOT_POINTER_LENGTH = 10;
const double PI_DELTA = .001;

// const double VELOCITY_SCALING_FACTOR = .0002;
const double VELOCITY_SCALING_FACTOR = .00007;
#define WALLS 1  // yes, the walls act like walls

// contest related variables
int SPIMBOT_TOURNAMENT = 0;
int reverse_image = 0;
int cycle = 0;
int cycle_limit = 0;
const int CYCLE_END = 10000000;
int EXIT_WHEN_DONE = 0;
bot_state_t robots[2];

// test whether something happened recently
#define WINDOW 100
int withinWINDOW(int cycle1) { return ((cycle1 <= cycle) && ((cycle1 + WINDOW) >= cycle)); }

// get a random location or edge
#define RANDOM_LOC ((random() % (WORLD_SIZE - 2*BOT_RADIUS)) + BOT_RADIUS)
#define RANDOM_EDGE ((random() & 1) ? (WORLD_SIZE - BOT_RADIUS) : BOT_RADIUS)

// because these aren't always available.
#define max(x, y) ((x > y) ? x : y)
#define min(x, y) ((x < y) ? x : y)

//#include "paint.c"
//#include "token_hunting2.c"
#include "block_pusher.c"
//#include "block_pusher2.c"
//#include "maze.c"

void
bot_initialize(int context, int randomize) {
  bot_state_t *bot = &(robots[context]);
  bot->context = context;
  bot->color = (context ? 0xff : 0xff0000);
  bot->done = 0;
  bot->bonk = 0;
  bot->x = randomize ? RANDOM_LOC : 50 ;
  bot->y = randomize ? RANDOM_LOC : 50 ;
  bot->orientation = randomize ? (M_PI * (random() & 0xff) / (float)0x7f) : 0;
  bot->velocity = 5;
  bot->last_x = 0;
  bot->last_y = 0;
  bot->turn_value = 0;
  bot->timer = SPIMBOT_MAXINT_32;
}

void
handle_wall_collisions(bot_state_t *bot, double delta_x, double delta_y) {
  if (WALLS) {
	 if ((bot->x < BOT_RADIUS) || (bot->x > (WORLD_SIZE - BOT_RADIUS))) {
		bot->x -= 2 * delta_x;
		bot->bonk = 1;
		bot->velocity = 0.0;
	 }
	 if ((bot->y < BOT_RADIUS) || (bot->y > (WORLD_SIZE - BOT_RADIUS))) {
		bot->y -= 2 * delta_y;
		bot->bonk = 1;
		bot->velocity = 0.0;
	 }
  } else {
	 if (bot->x < BOT_RADIUS) {
		bot->x += WORLD_SIZE - 2*BOT_RADIUS;
	 } else if (bot->x > (WORLD_SIZE - BOT_RADIUS)) {
		bot->x -= WORLD_SIZE - 2*BOT_RADIUS;
	 }
	 if (bot->y < BOT_RADIUS) {
		bot->y += WORLD_SIZE - 2*BOT_RADIUS;
	 } else if (bot->y > (WORLD_SIZE - BOT_RADIUS)) {
		bot->y -= WORLD_SIZE - 2*BOT_RADIUS;
	 }
  }
}

void 
bot_motion_update(bot_state_t *bot) {
	if (bot->done) {
	 return;
  }

  // do scenario-specific stuff
  bot_motion_update_scenario(bot);

  double velocity = VELOCITY_SCALING_FACTOR * bot->velocity;

  double delta_x = cos(bot->orientation) * velocity;
  double delta_y = sin(bot->orientation) * velocity;

  float prev_x = bot->x, prev_y = bot->y;
  bot->x += delta_x;
  bot->y += delta_y;

  // do scenario-specific stuff
  if (bot_motion_update_scenario(bot)) { // if this failed
	 bot->x = prev_x; 
	 bot->y = prev_y;
  }
  handle_wall_collisions_scenario(bot, delta_x, delta_y);
  handle_wall_collisions(bot, delta_x, delta_y);

  if (bot->orientation > (2*M_PI + PI_DELTA)) {
	 bot->orientation -= 2*M_PI;
  } else if (bot->orientation < (0 - PI_DELTA)) {
	 bot->orientation += 2*M_PI;
  }
  assert ((bot->orientation <= (2*M_PI + PI_DELTA)) && 
			 (bot->orientation >= (0 - PI_DELTA)));
}

void 
bot_io_update(bot_state_t *bot) {
  reg_image_t &reg_image = reg_images[bot->context];
  int cause = 0;
  if (bot->bonk
		&& INTERRUPTS_ON(reg_image)
		&& (Status_Reg(reg_image) & SB_BONK_INT_MASK) 
		&& ((Cause(reg_image) & SB_BONK_INT_MASK) == 0)) {
	 bot->bonk = 0;
	 cause |= SB_BONK_INT_MASK;
	 if (spimbot_debug) { 
		printf("Bonk interrupt invoked.\n");
	 }
	 if (!bot->done && spimbot_debug) {
		if (!SPIMBOT_TOURNAMENT) {
		  printf("bonk\n");
		}
	 }
  } 
  
  if ((bot->timer < cycle)
		&& INTERRUPTS_ON(reg_image)
		&& (Status_Reg(reg_image) & SB_TIMER_INT_MASK)) {
	 bot->timer = SPIMBOT_MAXINT_32;
	 cause |= SB_TIMER_INT_MASK;
	 if (spimbot_debug) { 
		printf("Timer interrupt invoked.\n");
	 }
  } 

  // do scenario-specific stuff
  cause |= bot_io_update_scenario(bot);

  if (cause != 0) {
	 if (GRADING) { 
		printf("raising an interrupt/exception; cause = %x\n", cause);
	 }
	 RAISE_EXCEPTION (reg_image, INT_EXCPT, Cause(reg_image) |= cause);
  }
}

void 
bot_update(bot_state_t *bot) {
  bot_motion_update(bot);
  bot_io_update(bot);
}

int
world_update() {
  for (int i = 0 ; i < num_contexts ; ++ i) {
	 // alternate which bot goes first for fairness
	 int index = i ^ (cycle & (num_contexts-1)); 
	 bot_update(&robots[index]);
  }

  cycle ++;
  if ((cycle & 0x3fff) == 0) {
	 redraw_map_window(map, NULL, NULL, NULL);
	 // usleep(400);
  }

  if (USER_STOP) {
	 USER_STOP = 0;
	 return 1;
  }

  //updates scenario specific settings based on cycles
  scenario_setting_update();


  if ((robots[0].done && ((num_contexts == 1) || robots[1].done)) ||
		scenario_done || 
		(cycle_limit && (cycle >= cycle_limit))) { 
	 redraw_map_window(map, NULL, NULL, NULL);
	 int winner = scenario_get_winner();
	 printf("cycles: %d\n", cycle);
	 if (winner == -1) {
		printf("NO WINNER (random winner %s)\n", robots[random()&1].name);
	 } else {
		printf("winner: %s\n", robots[winner].name);
	 }
	 mem_dump_profile();
	 if (EXIT_WHEN_DONE) {
		exit(0);
	 }
	 return 0;
  }

  return -1;
}

#ifdef __STDC__
void
create_map_display (void)
#else
void
create_map_display ()
#endif
{
  if (spimbot_largemap) {
	 // drawing_scaling = 2.25;
	 drawing_scaling = 2;
	 // drawing_scaling = 1.6;
  }

  String trans = "<Expose>:   redraw_map_window()\n\
                  <Ctrl>C: control_c_seen()\n\
                  <Btn1Down>: spimbot_map_click()";
  map = XtVaCreateManagedWidget ("map", coreWidgetClass, mappane,
											XtNtranslations, XtParseTranslationTable(trans),
											XtNwidth, SCALE(WORLD_SIZE) + 1, XtNheight, SCALE(WORLD_SIZE) + (draw_text ? LABEL_SPACE : 0) + 1, NULL);
  XtPopup (mapshell, XtGrabNone);

  Display *d = XtDisplay(map);
  Window W = XtWindow(map);
  GC gc = DefaultGCOfScreen(XtScreen(map));

  scenario_map_init(d, W, gc);
}

void
undraw_bot(Display *d, Window w, GC gc, bot_state_t *bot) {
  if (!bot->done) {
	 int delta_x = (int) (cos(bot->last_orientation) * BOT_POINTER_LENGTH);
	 int delta_y = (int) (sin(bot->last_orientation) * BOT_POINTER_LENGTH);
	 XFillArc(d, w, gc,
				 SCALE(bot->last_x - BOT_RADIUS), 
				 SCALE(bot->last_y - BOT_RADIUS), 
				 SCALE(2*BOT_RADIUS), SCALE(2*BOT_RADIUS), 0, 360*64);
	 XDrawLine(d, w, gc, SCALE(bot->last_x), SCALE(bot->last_y), 
				  SCALE(bot->last_x + delta_x), SCALE(bot->last_y + delta_y)); 
	 // XClearArea(d, w,
	 // 				SCALE(bot->last_x - (BOT_POINTER_LENGTH + 1)), 
	 // 				SCALE(bot->last_y - (BOT_POINTER_LENGTH + 1)), 
	 // 				SCALE(2*(BOT_POINTER_LENGTH + 1)), 
	 // 				SCALE(2*(BOT_POINTER_LENGTH + 1)), 
	 // 				0);
  }
}

void
draw_bot(Display *d, Window w, GC gc, bot_state_t *bot) {
  if (bot->done) {
	 return;
  }
  int delta_x = (int) (cos(bot->orientation) * BOT_POINTER_LENGTH);
  int delta_y = (int) (sin(bot->orientation) * BOT_POINTER_LENGTH);

  XFillArc(d, w, gc,
			  SCALE(bot->x - BOT_RADIUS), 
			  SCALE(bot->y - BOT_RADIUS), 
			  SCALE(2*BOT_RADIUS), SCALE(2*BOT_RADIUS), 0, 360*64);
  XDrawLine(d, w, gc, SCALE(bot->x), SCALE(bot->y), 
				SCALE(bot->x + delta_x), SCALE(bot->y + delta_y)); 
  bot->last_x = (int)(bot->x);
  bot->last_y = (int)(bot->y);
  bot->last_orientation = bot->orientation;
}

extern XtAppContext app_context;

void
redraw_map_window(Widget w, XEvent *event, 
						String *params, Cardinal *num_params) {
  Display *d = XtDisplay(w);
  Window W = XtWindow(w);
  GC gc = DefaultGCOfScreen(XtScreen(w));

  // erase robots
  XSetForeground(d, gc, 0xffffff);
  for (int i = 0 ; i < num_contexts ; ++ i) {
	 undraw_bot(d, W, gc, &robots[i]);
  }

  // do scenario specific stuff
  redraw_scenario(d, W, gc);

  // erase text
  // XClearArea(d, W, 10, SCALE(300), 30, 40, 0);
  XClearArea(d, W, 0, SCALE(300), SCALE(300), SCALE(LABEL_SPACE), 0);

  // redraw border
  XSetForeground(d, gc, 0);
  XDrawRectangle(d, W, gc, SCALE(0), SCALE(0), SCALE(WORLD_SIZE), SCALE(WORLD_SIZE));

  // re-draw bots and associated text
  for (int i = 0 ; i < num_contexts ; ++ i) {
	 XSetForeground(d, gc, robots[i].color);
	 draw_bot(d, W, gc, &robots[i]);
	 if (draw_text) {
		XDrawString(d, W, gc, 180, SCALE(WORLD_SIZE) + 20 + (i*20), robots[i].name, strlen(robots[i].name));
		char str[200];
		scenario_set_bot_string(str, i);
		XDrawString(d, W, gc, 10, SCALE(WORLD_SIZE) + 20 + (i*20), str, strlen(str));
	 }
  }

  // make sure this gets displayed, rather than cached
  XFlush(d);
  write_output (message_out, "");
}

void
write_spimbot_IO(int context, mem_addr addr, mem_word value) {
  int success = scenario_write_spimbot_IO(context, addr, value);
  if (success) {
	 return;
  }

  bot_state_t *bot = &robots[context];
  switch (addr) {
  case SPIMBOT_BONK_ACK:
	 if (GRADING || spimbot_debug) { 
		printf("Bonk interrupt acknowledged.\n");
	 }
	 Cause(reg_images[bot->context]) &= ~SB_BONK_INT_MASK;
	 break;
  case SPIMBOT_TIMER_ACK:
	 if (spimbot_debug) { 
		printf("Timer interrupt acknowledged.\n");
	 }
	 Cause(reg_images[bot->context]) &= ~SB_TIMER_INT_MASK;
	 break;
  case SPIMBOT_CYCLE:
	 bot->timer = value;
	 if (GRADING || spimbot_debug) { 
		printf("Timer %d\n", value - cycle);
	}
	 break;
  case SPIMBOT_VEL_ADDR:
	 bot->velocity = min(10, max(-10, (int)value));
	 break;
  case SPIMBOT_TURN_VALUE_ADDR:
	 if ((value > 360) || (value < -360)) { 
		// printf("turn value out of range (%d)\n", value);
	 } else {
		bot->turn_value = value;
	 }
	 break;
  case SPIMBOT_TURN_CONTROL_ADDR:
	 switch (value) {
	 case SPIMBOT_RELATIVE_ANGLE_COMMAND:
		bot->orientation += bot->turn_value * (M_PI/180.0);
//	        printf("orientation: %d %f\n", (int)bot->turn_value, bot->orientation);
		break;
	 case SPIMBOT_ABSOLUTE_ANGLE_COMMAND: {
		int turn_amount = bot->turn_value;
		if ((bot->context == 1) && reverse_image) {
		  turn_amount = turn_amount + 180;
		  if (turn_amount > 360) {
			 turn_amount -= 360;
		  }
		}
		bot->orientation = turn_amount * (M_PI/180.0);
//		printf("orientation: %d %f\n", (int)bot->turn_value, bot->orientation);
		break;
	 }
	 default:
		if (spimbot_debug) {
		  printf("Unexpected angle command: %d\n", value);
		}
	 }
	 break;
  case SPIMBOT_PRINT_INT:
	 if (!SPIMBOT_TOURNAMENT) {
		printf("%d\n", value);
	 }
	 break;
  case SPIMBOT_PRINT_HEX:
	 if (!SPIMBOT_TOURNAMENT) {
		printf("%x\n", value);
	 }
	 break;
  case SPIMBOT_PRINT_FLOAT: {
	 float *f = (float *)&value;
	 if (!SPIMBOT_TOURNAMENT) {
		printf("%f\n", *f);
	 }
	 break;
  }
  default:
	 run_error ("Write to unused memory-mapped IO address (0x%x)\n", addr);
  }
}

mem_word
read_spimbot_IO(int context, mem_addr addr) {
  int success = 0;
  mem_word ret_val = scenario_read_spimbot_IO(context, addr, &success);
  if (success) {
	 return ret_val;
  }

  bot_state_t *bot = &robots[context];
  switch (addr) {
  case SPIMBOT_X_ADDR: {
	 mem_word x = (mem_word) 
		(((bot->context == 1) && reverse_image) ? (WORLD_SIZE - bot->x) : bot->x);
 	 return x;
 	 //return (mem_word)(bot->x);
  }
  case SPIMBOT_Y_ADDR: {
	 mem_word y = (mem_word)
		(((bot->context == 1) && reverse_image) ? (WORLD_SIZE - bot->y) : bot->y);
	 return y;
 	 // return (mem_word)(bot->y);
  }
  case SPIMBOT_VEL_ADDR:
         return mem_word(int(bot->velocity));
  case SPIMBOT_TURN_VALUE_ADDR:
//printf("%d %f\n", round(180.0/M_PI*bot->orientation), bot->orientation);
         return mem_word(int(round(bot->orientation*180.0/M_PI)));
  case SPIMBOT_CYCLE:
 	 return cycle;
  default:
	 run_error ("Read from unused memory-mapped IO address (0x%x)\n", addr);
	 return (0);
  }
}
