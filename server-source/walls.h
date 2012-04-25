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

enum wall_coord_t {X0, Y0, X1, Y1, NUM_WALL_COORDS};

typedef struct {
  double coords[NUM_WALL_COORDS];
} wall_t;

int g_num_walls = 0;
wall_t *g_walls = NULL;

void
wall_init(int num_walls) {
  num_walls = 2;
  g_num_walls = num_walls;
  g_walls = (wall_t *) malloc (num_walls * sizeof(wall_t));
  for (int i = 0 ; i < num_walls ; ++ i) {
	 wall_t *w = &g_walls[i];
	 int x = (i & 1), y = !x;
	 w->coords[x] = (random() % (WORLD_SIZE/2)) + (WORLD_SIZE/4);
	 w->coords[x + 2] = (random() % (WORLD_SIZE/2)) + (WORLD_SIZE/4);
	 w->coords[y] = (WORLD_SIZE/4);
	 w->coords[y + 2] = 3 * (WORLD_SIZE/4);
  }
}

void
wall_collision(int i, bot_state_t *bot, double old_x, double old_y) {
}

void
handle_wall_collisions(bot_state_t *bot, double old_x, double old_y) {
  for (int i = 0 ; i < g_num_walls ; ++ i) {
	 wall_collision(i, bot, old_x, old_y);
  }
}
