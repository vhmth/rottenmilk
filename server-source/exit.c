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

int exit_x, exit_y; 

void
exit_init() {
  if (random() & 1) {
	 exit_x = RANDOM_LOC;  exit_y = RANDOM_EDGE;
  } else {
	 exit_y = RANDOM_LOC;  exit_x = RANDOM_EDGE;
  }
}

#define EXIT_DELTA 5

void
check_exit(bot_state_t *bot) {
  if ((abs((int)(bot->x - exit_x)) < EXIT_DELTA) && 
		(abs((int)(bot->y - exit_y)) < EXIT_DELTA)) {
	 printf("Bot Exited\n");
	 bot->done = 1;
  }
}

#define EXIT_RADIUS 2
#define EXIT_HALF_LENGTH 5

void
draw_exit(Display *d, Window w, GC gc) {
#ifndef MP1
  XDrawRectangle(d, w, gc,
					  SCALE(exit_x - EXIT_HALF_LENGTH), 
					  SCALE(exit_y - EXIT_HALF_LENGTH), 
					  SCALE(2*EXIT_HALF_LENGTH), 
					  SCALE(2*EXIT_HALF_LENGTH));
#endif
}

  // check_exit(bot);

  // draw_exit(d, W, gc);


//   case SPIMBOT_EXIT_X_ADDR:
// 	 return exit_x;
//   case SPIMBOT_EXIT_Y_ADDR:
// 	 return exit_y;
