/*
	Sudoku generator

	(C) Copyright 2005 by Mogens Kjaer, mk@lemo.dk

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAXNX 3
#define MAXNY 3
#define MAXNXNY 9
#define SHIFTIT 8
#define SHIFTMASK 255
#define LEVEL_ANY -1 
#define LEVEL_VERYEASY 0
#define LEVEL_EASY 1
#define LEVEL_MEDIUM 2
#define LEVEL_HARD 3
#define LEVEL_DIFFICULT 4
#define NLEVELS 5
#define NSYMMETRICALS 3
#define MOVELIMIT 100000

extern int spimbot_debug;

int nfilled=28;
int bestndigits=0;
int ndigits;
int symmetrical=1;
int level=LEVEL_EASY;
int thislevel=0;
int smallinitial=1;

int displayboard[MAXNXNY][MAXNXNY];
int origdisplayboard[MAXNXNY][MAXNXNY];
int solutionboard[MAXNXNY][MAXNXNY];

int setboard[MAXNXNY][MAXNXNY];
int setboardsave[MAXNXNY][MAXNXNY];
int beforeboard[MAXNXNY][MAXNXNY];
int board[MAXNXNY][MAXNXNY];
int initialpossible[MAXNXNY][MAXNXNY];
int possible[MAXNXNY][MAXNXNY];
int eliminated[MAXNXNY][MAXNXNY];
int nx=3;
int ny=3;
int nxny=9;
int nsol=0;
int ntest=0;

int statistics[MAXNXNY*MAXNXNY][NLEVELS];
char symbollist[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZÆØÅabcdefghijklmnopqrstuvwxyzæøå";
int progstat=0;
int stop_pressed=0;


// int nfilled,newnfilled,bestndigits,ndigits;
// int symmetrical;
// int level;
// int thislevel;
// int nx,ny,nxny;
// int statistics[MAXNXNY*MAXNXNY][NLEVELS];
// char symbollist[];
// 
// int origdisplayboard[MAXNXNY][MAXNXNY];
// int displayboard[MAXNXNY][MAXNXNY];
// int solutionboard[MAXNXNY][MAXNXNY];
// 
// int setboard[MAXNXNY][MAXNXNY];
// int setboardsave[MAXNXNY][MAXNXNY];
// int beforeboard[MAXNXNY][MAXNXNY];
// int initialpossible[MAXNXNY][MAXNXNY];
// int possible[MAXNXNY][MAXNXNY];
// int eliminated[MAXNXNY][MAXNXNY];
// int board[MAXNXNY][MAXNXNY];
// int nsol,ntest,stop_pressed,progstat;
static int *bitmaps=NULL;
static int *bitmapsn;
static int maxbitmap;

static int randomize=1;
static int nmultiple,nsccell,nscnumber,nssc,nds;
static int fastalg=0;
static int nmove,movelimit;

int printout = 0;
// void logprintf(const char *template, ...);

// void logprintf(const char *template, ...) {}
int test_stop() { return 0; }
void main_update() {}
void update_menu() {}
static void move();
void findpossible();
int solve_sudoku(int newprintout);

/* calculate the number of bits set to 1 in p: */

static int nbits(unsigned int n)
{
  n = (0x55555555u & n) + (0x55555555u & (n>> 1));
  n = (0x33333333u & n) + (0x33333333u & (n>> 2));
  n = (0x0f0f0f0fu & n) + (0x0f0f0f0fu & (n>> 4));
  n = (0x00ff00ffu & n) + (0x00ff00ffu & (n>> 8));
  n = (0x0000ffffu & n) + (0x0000ffffu & (n>>16));
  return n;
}

static void permute(int n, int *values)
{
  int i,j,tmp;

  for (i=0;i<(n-1);i++)
  {
    j=random()%(n-i)+i;
    tmp=values[i];
    values[i]=values[j];
    values[j]=tmp;
  }
}

// static int check(int r, int c, int n)
// {
//   int i,j;
//   for (i=0;i<nxny;i++)
//   { if (setboard[i][c]==n) return 0; if (setboard[r][i]==n) return 0; }
//   for (i=0;i<nx;i++) for (j=0;j<ny;j++)
//   { if (setboard[r/nx*nx+i][c/ny*ny+j]==n) return 0; }
//   return 1;
// }

void initplay(int smallinitial)
{
  int i,j;
  for (i=0; i<nxny; i++)
  for (j=0; j<nxny; j++)
  {
    eliminated[i][j] = 0;
    origdisplayboard[i][j] = displayboard[i][j];
  }
  solve_sudoku(0);
  for (i=0; i<nxny; i++)
  {
    for (j=0; j<nxny; j++)
    {
      solutionboard[i][j] = abs(displayboard[i][j]);
      displayboard[i][j] = origdisplayboard[i][j];
      board[i][j] = displayboard[i][j];
    }
  }
  if (smallinitial)
  {
    findpossible();
    for (i=0; i<nxny; i++)
    for (j=0; j<nxny; j++)
    {
      initialpossible[i][j] = possible[i][j];
    }
  }
  else
  {
    for (i=0; i<nxny; i++)
    for (j=0; j<nxny; j++)
    {
      initialpossible[i][j] = maxbitmap;
      possible[i][j] = maxbitmap;
    }
  }
}

void findpossible()
{
  int i,j,i1,j1,p;

  for (i=0; i<nxny; i++)
  {
    for (j=0; j<nxny; j++)
    {
      if (board[i][j])
      {
		  p=0;
      }
      else
      {
	p=maxbitmap;
	for (i1=0; i1<nxny; i1++)
	{
	  p &= ~(1<<board[i][i1]);
	  p &= ~(1<<board[i1][j]);
	}
	for (i1=0;i1<nx;i1++)
	for (j1=0;j1<ny;j1++)
	  p &= ~(1<<board[i/nx*nx+i1][j/ny*ny+j1]);
      }
      possible[i][j] = p;
    }
  }
}

static void test_cell(int *nindices, int *indices, int *r, int *c)
{
  int best, bestp, i, j, p, n;
  best = 999;
  bestp = 0;
  *nindices=0;

  for (i=0; i<nxny; i++)
  {
    for (j=0; j<nxny; j++)
    {
      if (board[i][j]==0)
      {
	p=possible[i][j];
	n = nbits(p);
	if (n<best)
	{
	  best=n;
	  bestp=p;
	  /* if n is zero, we've reached a dead end */
	  if (n==0)
	  {
	    *nindices=-1;
	    /* printf("Dead end.\n"); */
	    return;
	  }
	  *r = i;
	  *c = j;
	}
      }
    }
  }
  for (i=1;bestp;i++)
  {
    bestp >>= 1;
    if (1 & bestp)
    {
      indices[(*nindices)++]=i;
    }
  }
  /* printf("test_cell: r: %d c: %d nindices: %d\n", *r, *c, *nindices); */
}

static int test_number(int *nindices, int *indices, int *r, int *c)
{
  int i,j,p,k,k1,n,l,l1,l2;
  for (i=0;i<nxny;i++) {
    for (j=0; j<nxny; j++) {
      if ((p=possible[i][j])) {
		  k = 2;
		  for (k1=1;k1<=nxny;k1++) {
			 if (k & p) {
				n=0;
				for (l=0;l<nxny;l++) {
				  if (possible[i][l]&k)   n++;
				}
				if (n==1) {
			//	      if (printout) logprintf("B%2.2d%2.2d%2.2d%c", i+1, j+1, i+1, symbollist[k1]);
				  *r=i;
				  *c=j;
				  indices[0]=k1;
				  *nindices=1;
				  return 1;
				}
				n=0;
				for (l=0;l<nxny;l++) {
				  if (possible[l][j]&k) n++;
				}
				if (n==1) {
	  //	      if (printout) logprintf("C%2.2d%2.2d%2.2d%c", i+1, j+1, j+1, symbollist[k1]);
				  *r=i;
				  *c=j;
				  indices[0]=k1;
				  *nindices=1;
				  return 1;
				}
				n=0;
				for (l1=0;l1<nx;l1++)
				  for (l2=0;l2<ny;l2++) {
					 if (possible[i/nx*nx+l1][j/ny*ny+l2]&k)  n++;
				  }
				if (n==1) {
			//	      if (printout) logprintf("D%2.2d%2.2d%2.2d%2.2d%c", i+1, j+1, i/nx+1, j/ny+1, symbollist[k1]);
				  *r=i;
				  *c=j;
				  indices[0]=k1;
				  *nindices=1;
				  return 1;
				}
			 }
			 k = k<<1;
		  }
      }
    }
  }
  return 0;
}

static int test_ssc()
{
  /*
  	Find single sector candidates
  */
  int i,i1,i2,j,j1,j2,k,k1,foundany,sector,noccur,isector,jsector;  // ,foundthispass

  foundany=0;
  for (k=2,k1=1;k1<=nxny;k<<=1,k1++)
  {
    /* check rows i for occurance of k1's in a single sector */
    for (i=0;i<nxny;i++)
    {
      sector = -1;
      noccur = -1;
      for (j=0;j<nxny;j++) {
		  if (possible[i][j]&k) {
			 if (sector == -1) {
				/* first occurance of k in row i: */
				sector = j/ny;
				noccur = 1;
			 } else if (sector == j/ny) {
				/* occurs in same sector */
				noccur++;
			 } else {
				/* occurs in another sector; not candidate */
				goto NOT_CANDIDATE1;
			 }
		  }
      }
      if (noccur>1) {
		  /* eliminating other occurences */
		  isector=i/nx*nx;
		  jsector=sector*ny;
		  for (i1=0;i1<nx;i1++) {
			 if (isector+i1!=i) {
				for (i2=0;i2<ny;i2++) {
				  if (possible[isector+i1][jsector+i2]&k) {
			  //		if (printout) logprintf("E%2.2d%2.2d%2.2d%c", isector+i1+1, jsector+i2+1, i+1, symbollist[k1]);
					 possible[isector+i1][jsector+i2] &= ~k;
					 foundany=1;
				  }
				}
			 }
		  }
		  if (foundany) return 1;
      }
NOT_CANDIDATE1: ;
    }
    /* check columns j for occurance of k1's in a single sector */
    for (j=0;j<nxny;j++)
    {
      sector = -1;
      noccur = -1;
      for (i=0;i<nxny;i++)
      {
	if (possible[i][j]&k)
	{
	  if (sector == -1)
	  {
	    /* first occurance of k in column j: */
	    sector = i/nx;
	    noccur = 1;
	  }
	  else if (sector == i/nx)
	  {
	    /* occurs in same sector */
	    noccur++;
	  }
	  else
	  {
	    /* occurs in another sector; not candidate */
	    goto NOT_CANDIDATE2;
	  }
	}
      }
      if (noccur>1)
      {
	/* eliminating other occurences */
	isector=sector*nx;
	jsector=j/ny*ny;
	for (i1=0;i1<nx;i1++)
	{
	  for (i2=0;i2<ny;i2++)
	  {
	    if (jsector+i2!=j)
	    {
	      if (possible[isector+i1][jsector+i2]&k)
	      {
			  //		if (printout) logprintf("F%2.2d%2.2d%2.2d%c", isector+i1+1, jsector+i2+1, j+1, symbollist[k1]);
		possible[isector+i1][jsector+i2] &= ~k;
		foundany=1;
	      }
	    }
	  }
	}
	if (foundany) return 1;
      }
NOT_CANDIDATE2: ;
    }
    for (i1=0;i1<ny;i1++)
    for (i2=0;i2<nx;i2++)
    {
      sector = -1;
      noccur = -1;
      for (j1=0;j1<nx;j1++)
      for (j2=0;j2<ny;j2++)
      {
	if (possible[i1*nx+j1][i2*ny+j2]&k)
	{
	  if (sector == -1)
	  {
	    /* first occurance of k in box[i1,j2]: */
	    sector = j1;
	    noccur = 1;
	  }
	  else if (sector == j1)
	  {
	    /* occurs in same row */
	    noccur++;
	  }
	  else
	  {
	    /* occurs in another row; not candidate */
	    goto NOT_CANDIDATE3;
	  }
	}
      }
      if (noccur>1)
      {
	/* eliminating other occurences */
	isector=sector+i1*nx;
	for (j=0;j<nxny;j++)
	{
	  if (j/ny != i2)
	  {
	    if (possible[isector][j]&k)
	    {
			//	      if (printout) logprintf("G%2.2d%2.2d%2.2d%2.2d%c", isector+1, j+1, i1+1, i2+1, symbollist[k1]);
	      possible[isector][j] &= ~k;
	      foundany=1;
	    }
	  }
	}
	if (foundany) return 1;
      }
NOT_CANDIDATE3: ;
    }
    for (i1=0;i1<ny;i1++)
    for (i2=0;i2<nx;i2++)
    {
      sector = -1;
      noccur = -1;
      for (j1=0;j1<nx;j1++)
      for (j2=0;j2<ny;j2++)
      {
	if (possible[i1*nx+j1][i2*ny+j2]&k)
	{
	  if (sector == -1)
	  {
	    /* first occurance of k in box[i1,j2]: */
	    sector = j2;
	    noccur = 1;
	  }
	  else if (sector == j2)
	  {
	    /* occurs in same column */
	    noccur++;
	  }
	  else
	  {
	    /* occurs in another column; not candidate */
	    goto NOT_CANDIDATE4;
	  }
	}
      }
      if (noccur>1)
      {
	/* eliminating other occurences */
	jsector=sector+i2*ny;
	for (i=0;i<nxny;i++)
	{
	  if (i/nx != i1)
	  {
	    if (possible[i][jsector]&k)
	    {
			//	      if (printout) logprintf("H%2.2d%2.2d%2.2d%2.2d%c", i+1, jsector+1, i1+1, i2+1, symbollist[k1]);
	      possible[i][jsector] &= ~k;
	      foundany=1;
	    }
	  }
	}
	if (foundany) return 1;
      }
NOT_CANDIDATE4: ;
    }
  }
  return 0;
}

void
qs_recurse(int left,int right)
{
  int i,j,x,tmp;

  i=left;
  j=right;
  x=bitmapsn[(left+right)/2];
  do
  {
    while (bitmapsn[i]<x && i<right) i++;
    while (x<bitmapsn[j] && j>left) j--;
    if (i<=j)
    {
      tmp=bitmapsn[i];
      bitmapsn[i]=bitmapsn[j];
      bitmapsn[j]=tmp;
      tmp=bitmaps[i];
      bitmaps[i]=bitmaps[j];
      bitmaps[j]=tmp;
      i++;
      j--;
    }
  } while (i<=j);
  if (left<j) qs_recurse(left,j);
  if (i<right) qs_recurse(i,right);
}

void
qs(int array_size)
{
  qs_recurse(0,array_size-1);
}


void init_bitmaps(int nset)
{
  int i,j,n;    // int k,k1,i,j,n;

  maxbitmap = 0;
  j=2;
  for (i=0;i<nset;i++)
  {
    maxbitmap |= j;
    j = j<<1;
  }

  if (bitmaps != NULL) free((char *) bitmaps);
  bitmaps = (int *) malloc(sizeof(*bitmaps)*maxbitmap);
  bitmapsn = (int *) malloc(sizeof(*bitmaps)*maxbitmap);

  n=0;
  for (i=2; i<=(maxbitmap & ~2); i+=2)
  {
    j=nbits(i);
    if (j>1)
    {
      bitmaps[n]=i;
      bitmapsn[n]=j;
      n++;
    }
  }
  bitmaps[n]=0;
  qs(n);
  free((char *) bitmapsn);
}

static int test_ds_help(int type, int r, int c)
{
  int i,j,k,k1,n,nmatch,nor,nb,ibitmap,bitmap,elim,anyeliminated;
  int slice[MAXNXNY];
  char buf1[512], buf2[512];
  int printed;

  anyeliminated=0;

  if (type==0)
  {
    for (j=0;j<nxny;j++) slice[j]=possible[r][j];
  }
  else if (type==1)
  {
    for (i=0;i<nxny;i++) slice[i]=possible[i][c];
  }
  else
  {
    for (i=0;i<nx;i++)
    for (j=0;j<ny;j++) slice[i*ny+j]=possible[r*nx+i][c*ny+j];
  }

  for (ibitmap=0; bitmaps[ibitmap] && !anyeliminated; ibitmap++)
  {
    if (ibitmap%10000==0)
    {
      if (test_stop())
      {
        progstat=2;
        return 0;
      }
    }
    bitmap=bitmaps[ibitmap];
    nb=nbits(bitmap);
    nmatch=0;
    nor=0;
    for (i=0;i<nxny;i++)
    {
      if (slice[i]&bitmap)
      {
        nmatch++;
	nor |= (slice[i]&bitmap);
      }
    }
    if (nmatch==nb && nor==bitmap)
    {
      if (printout)
      {
        n=nbits(bitmap);
	sprintf(buf1, "I%2.2d", n);
	for (k=2,k1=1;k1<=nxny;k<<=1,k1++)
	{
	  if (k&bitmap)
	  {
	    sprintf(buf2, "%c", symbollist[k1]);
	    strcat(buf1, buf2);
	  }
	}
	for (i=0;i<nxny;i++)
	{
	  if (slice[i]&bitmap)
	  {
	    if (type==0) sprintf(buf2, "%2.2d%2.2d", r+1, i+1);
	    else if (type==1) sprintf(buf2, "%2.2d%2.2d", i+1, c+1);
	    else sprintf(buf2, "%2.2d%2.2d", r*nx+i/ny+1, c*ny+i%ny+1);
	    strcat(buf1, buf2);
	  }
	}
	printed=0;
      }
      for (i=0;i<nxny;i++)
      {
	if (slice[i]&bitmap)
	{
	  elim=slice[i]&(~bitmap);
	  if (elim)
	  {
	    anyeliminated=1;
	    if (printout)
	    {
	      for (k=2,k1=1;k1<=nxny;k<<=1,k1++)
	      {
		if (k&elim)
		{
		  if (!printed)
		  {
			 //		    logprintf("%s", buf1);
		    printed=1;
		  }
		  //		  if (type==0) logprintf("J%2.2d%2.2d%c", r+1, i+1, symbollist[k1]);
		  //	     else if (type==1) logprintf("J%2.2d%2.2d%c", i+1, c+1, symbollist[k1]);
		  //		  else logprintf("J%2.2d%2.2d%c", r*nx+i/ny+1, c*ny+i%ny+1, symbollist[k1]);
		}
	      }
	    }
	    slice[i] &= bitmap;
	  }
	}
      }
    }
  }
  if (anyeliminated)
  {
    if (type==0)
    {
      for (j=0;j<nxny;j++) possible[r][j]=slice[j];
    }
    else if (type==1)
    {
      for (i=0;i<nxny;i++) possible[i][c]=slice[i];
    }
    else
    {
      for (i=0;i<nx;i++)
      for (j=0;j<ny;j++) possible[r*nx+i][c*ny+j]=slice[i*ny+j];
    }
  }
  return anyeliminated;
}

static int test_ds()
{
  int i,j;
  for (i=0;i<nxny;i++)
  {
    if (test_ds_help(0, i, 0)) goto ELIMINATED;
    if (stop_pressed) return 0;
  }
  for (j=0;j<nxny;j++)
  {
    if (test_ds_help(1, 0, j)) goto ELIMINATED;
    if (stop_pressed) return 0;
  }
  for (i=0;i<ny;i++)
  for (j=0;j<nx;j++)
  {
    if (test_ds_help(2, i, j)) goto ELIMINATED;
    if (stop_pressed) return 0;
  }
  return 0;
ELIMINATED:
  return 1;
}

static int test_guess(int *nindices, int *indices, int r, int c)
{
  int iindex,i,j,saveprintout,n,savensol;
  int localpossible[MAXNXNY][MAXNXNY];
  int localboard[MAXNXNY][MAXNXNY];

  saveprintout=printout;
  savensol=nsol;
  printout=0;

  for (i=0;i<nxny;i++)
  {
    for (j=0;j<nxny;j++)
    {
      localpossible[i][j]=possible[i][j];
      localboard[i][j]=board[i][j];
    }
  }
  for (iindex=0; iindex<*nindices; iindex++)
  {
    board[r][c]=indices[iindex];
    n=~(1<<indices[iindex]);
    for (i=0;i<nxny;i++)
    {
      possible[r][i] &= n;
      possible[i][c] &= n;
    }
    for (i=0;i<nx;i++)
    for (j=0;j<ny;j++)
      possible[r/nx*nx+i][c/ny*ny+j] &= n;
    possible[r][c]=0;
    nsol=1;
    movelimit=0;
    move();
    for (i=0;i<nxny;i++)
    {
      for (j=0;j<nxny;j++)
      {
	board[i][j]=localboard[i][j];
	possible[i][j]=localpossible[i][j];
      }
    }
    if (stop_pressed) return 0;
    if (nsol>1)
    {
      *nindices = 1;
      indices[0] = indices[iindex];
      printout = saveprintout;
      nsol = savensol;
      return 1;
    }
  }
  printout = saveprintout;
  nsol = savensol;
  //  if (printout) logprintf("Y");
  return 0;
}

static void move()
{
  int r,c,i,j,n,i1,j1;  //   int r,c,i,j,p,k,n,k1,i1,j1;
  int indices[MAXNXNY+1];
  int nindices;
  int localpossible[MAXNXNY][MAXNXNY];
  // int saveprintout;
  // int anyeliminated,eliminatedthis; 
  char buf1[MAXNXNY+10];
#ifdef UAKK
  for (i=0;i<nxny;i++)
  {
    for (j=0;j<nxny;j++)
    {
      if (board[i][j]!=0)
      {
	printf("\t=%c", symbollist[board[i][j]]);
      }
      else
      {
        printf("\t");
	for (k=2,k1=1;k1<=nxny;k<<=1,k1++)
	  if (possible[i][j]&k) printf("%c", symbollist[k1]);
      }
    }
    printf("\n");
  }
#endif
  nmove++;
  if (movelimit>0 && nmove>movelimit)
  {
    nsol=2;
    return;
  }
  if ((nmove%10000)==0)
  {
    if (test_stop())
    {
      progstat=2;
      return;
    }
  }
  r = -1;
  c = -1;
  test_cell(&nindices, indices, &r, &c);
#ifdef UAKK
  printf(" - r: %d c: %d nindices: %d\n", r+1, c+1, nindices);
#endif
  /* if nindices is -1, we've reached a dead end */
  if (nindices == -1)
  {
#ifdef UAKK
    printf("Dead end.\n");
#endif
    return;
  }
  /* if r is -1, all cells are filled, and we're done */
  if (r== -1)
  {
    nsol++;
#ifdef UAKK
    printf("Move done. nsol: %d\n", nsol);
#endif
    return;
  }
  if (nindices>1 && fastalg==0)
  {
    if (test_number(&nindices, indices, &r, &c))
    {
#ifdef UAKK
      printf("test_number: %d\n", indices[0]);
#endif
      nscnumber++;
      goto FOUND2;
    }
    while(test_ssc())
    {
      /* eliminated ssc, call test_cell and test_number again */ 
      test_cell(&nindices, indices, &r, &c);
      if (nindices==1)
      {
        //  if (printout) logprintf("K%2.2d%2.2d%c", r+1, c+1, symbollist[indices[0]]);
        nssc++;
	goto FOUND2;
      }
      if (test_number(&nindices, indices, &r, &c))
      {
        //  if (printout) logprintf("K%2.2d%2.2d%c", r+1, c+1, symbollist[indices[0]]);
        nssc++;
	goto FOUND2;
      }
    }
    while(test_ds())
    {
      /* eliminated ds, call test_cell and test_number again */ 
      test_cell(&nindices, indices, &r, &c);
      if (nindices==1)
      {
        //  if (printout) logprintf("L%2.2d%2.2d%c", r+1, c+1, symbollist[indices[0]]);
        nds++;
	goto FOUND2;
      }
      if (test_number(&nindices, indices, &r, &c))
      {
        //  if (printout) logprintf("L%2.2d%2.2d%c", r+1, c+1, symbollist[indices[0]]);
        nds++;
	goto FOUND2;
      }
    }
    if (stop_pressed) return;
    if (printout)
    {
      sprintf(buf1, "M%2.2d%2.2d%2.2d", nindices, r+1, c+1);
      for (i=0; i<nindices; i++) buf1[i+7]=symbollist[indices[i]];
      buf1[nindices+7] = '\0';
      // logprintf("%s", buf1);
    }
    if (test_guess(&nindices, indices, r, c))
    {
      // if (printout) logprintf("N%2.2d%2.2d%c", r+1, c+1, symbollist[indices[0]]);
      nmultiple++;
      goto FOUND2;
    }
  }
  else if (nindices==1)
  {
    nsccell++;
  }
  if (printout)
  {
    if (nindices==1)
    {
      // logprintf("A%2.2d%2.2d%c", r+1, c+1, symbollist[indices[0]]);
    }
    else
    {
      sprintf(buf1, "O%2.2d%2.2d%2.2d", nindices, r+1, c+1);
      for (i=0; i<nindices; i++) buf1[i+7]=symbollist[indices[i]];
      buf1[nindices+7] = '\0';
      // logprintf("%s", buf1);
    }
  }
  if (randomize) permute(nindices, indices);
FOUND2:
  for (i=0;i<nxny;i++)
  {
    for (j=0;j<nxny;j++)
    localpossible[i][j]=possible[i][j];
  }
  for (i=0;i<nindices;i++)
  {
    board[r][c]=indices[i];
    n=~(1<<indices[i]);
    for (i1=0;i1<nxny;i1++)
    {
      possible[r][i1] &= n;
      possible[i1][c] &= n;
    }
    for (i1=0;i1<nx;i1++)
    for (j1=0;j1<ny;j1++)
      possible[r/nx*nx+i1][c/ny*ny+j1] &= n;
    possible[r][c]=0;
    move();
    if (stop_pressed) return;
    if (nsol>1) return;
    board[r][c]=0;
    for (i1=0;i1<nxny;i1++)
    {
      for (j1=0;j1<nxny;j1++)
      possible[i1][j1]=localpossible[i1][j1];
    }
  }
}

static int findcategory()
{
  if (nmultiple==0 && nds==0 && nssc==0 && nscnumber==0) return LEVEL_VERYEASY;
  else if (nmultiple==0 && nds==0 && nssc==0 && nscnumber>0) return LEVEL_EASY;
  else if (nmultiple==0 && nds==0 && nssc>0) return LEVEL_MEDIUM;
  else if (nmultiple==0 && nds>0) return LEVEL_HARD;

  return LEVEL_DIFFICULT;
}

void findlevel()
{
  int i1,j1,savefastalg;
  for (i1=0; i1<nxny; i1++)
  for (j1=0; j1<nxny; j1++)
  board[i1][j1] = setboard[i1][j1];
  nmultiple=0;
  nsccell=0;
  nscnumber=0;
  nssc=0;
  nds=0;
  savefastalg=fastalg;
  fastalg=0;
  findpossible();
  nmove=0;
  movelimit=0;
  move();
  fastalg=savefastalg;
  thislevel=findcategory();
}

static int testdigits() {
  int i,j;  //   int i,j,i1,j1, temp;

  findlevel();
  if (stop_pressed) return 0;
  statistics[ndigits][thislevel]++;

  if (spimbot_debug) {
	 printf("Level %d board:\n", thislevel);
  }
//   for (i=0; i<nxny; i++) {
//     for (j=0; j<nxny; j++) {
// 		if ((temp = setboard[i][j]) == 0) {
// 		  printf("*");
// 		} else {
// 		  printf("%d", temp);
// 		}
// 	 }
// 	 printf("\n");
//   }

  if (level!=thislevel && level!=LEVEL_ANY) return 0;

  if (ndigits<=bestndigits || bestndigits==0)
  {
    for (i=0; i<nxny; i++)
    for (j=0; j<nxny; j++)
    displayboard[i][j] = setboard[i][j];
    bestndigits=ndigits;
    main_update();
    update_menu();
    if (bestndigits<=nfilled)
    {
      progstat=2;
      update_menu();
      return 1;
    }
  }
  return 0;
}
void sudoku_refine()
{
  int i,j,i1,j1;
  int i2; // ,j2,k2;
  int i3,j3;
  int originalndigits;
  int indices[MAXNXNY*MAXNXNY],*pindex,nindices;
  int solution[MAXNXNY][MAXNXNY];

  originalndigits=0;
  for (i=0; i<nxny; i++)
  for (j=0; j<nxny; j++)
  {
    setboardsave[i][j] = board[i][j];
    if (board[i][j]) originalndigits++;
  }

  randomize=0;
  nsol=1;
  findpossible();
  nmove=0;
  movelimit=MOVELIMIT;
  move();
  for (i=0; i<nxny; i++)
  for (j=0; j<nxny; j++)
  solution[i][j] = board[i][j];

  /* For each white cell, try to insert the solution
     and see if we can remove other numbers: */

  for (i3=0;i3<nxny;i3++)
  for (j3=0;j3<nxny;j3++)
  {
    if (!setboardsave[i3][j3])
    {
      pindex=indices;
      nindices=0;
      for (i=0;i<nxny;i++)
      for (j=0;j<nxny;j++)
      {
        if (i!=i3 && j!=j3 && setboardsave[i][j])
	{
	  *pindex++ = (i<<SHIFTIT)|j;
	  nindices++;
	}
      }
      permute(nindices,indices);
      ndigits=originalndigits+1;
      for (i1=0; i1<nxny; i1++)
      for (j1=0; j1<nxny; j1++)
      {
	setboard[i1][j1] = setboardsave[i1][j1];
      }
      setboard[i3][j3] = solution[i3][j3];
      fastalg=1;
      for (i2=0;i2<nindices;i2++)
      {
	i = indices[i2]>>SHIFTIT;
	j = indices[i2]&SHIFTMASK;
	for (i1=0; i1<nxny; i1++)
	for (j1=0; j1<nxny; j1++)
	board[i1][j1] = setboard[i1][j1];
	board[i][j] = 0;
	nsol=0;
	findpossible();
	nmove=0;
	movelimit=MOVELIMIT;
	move();
	if (stop_pressed) return;
	if (nsol==1)
	{
	  setboard[i][j] = 0;
	  ndigits--;
	}
      }
      if (ndigits<originalndigits)
      {
	printf("i3: %d, j3: %d, Original: %d, better: %d\n", i3, j3, originalndigits, ndigits);
	bestndigits=ndigits;
	symmetrical=0;
	for (i=0; i<nxny; i++)
	for (j=0; j<nxny; j++)
	displayboard[i][j] = setboard[i][j];
	findlevel();
	main_update();
      }
    }
  }
}


void sudoku_run()
{
  int i,j,i1,j1;
  int i2; // ,j2,k2;
  // int walktype;
  int indices[MAXNXNY*MAXNXNY],*pindex,nindices;

  progstat=1;
  fastalg=1;

  ntest++;

  for (i=0; i<nxny; i++)
  for (j=0; j<nxny; j++)
  board[i][j] = 0;

  randomize=1;

  nsol=0;
  findpossible();
  nmove=0;
  movelimit=0;
  move();
  if (stop_pressed) return;
  if (nsol==0)
  {
    fprintf(stderr, "Hm, can't happen.\n");
    exit(0);
  }

  for (i=0; i<nxny; i++)
  for (j=0; j<nxny; j++)
  setboardsave[i][j] = board[i][j];

  randomize=0;

  pindex=indices;
  nindices=0;
  if (symmetrical==1)
  {
    for (i=0;i<((nxny+1)/2);i++)
    for (j=0;j<((nxny+1)/2);j++)
    {
      *pindex++ = (i<<SHIFTIT)|j;
      nindices++;
    }
  }
  else if (symmetrical==2)
  {
    for (i=0;i<(nxny/2);i++)
    for (j=0;j<nxny;j++)
    {
      *pindex++ = (i<<SHIFTIT)|j;
      nindices++;
    }
    if ((nxny&1)==1)
    {
      for (j=0;j<((nxny+1)/2);j++)
      {
	*pindex++ = ((nxny/2)<<SHIFTIT)|j;
	nindices++;
      }
    }
  }
  else
  {
    for (i=0;i<nxny;i++)
    for (j=0;j<nxny;j++)
    {
      *pindex++ = (i<<SHIFTIT)|j;
      nindices++;
    }
  }
  permute(nindices,indices);
  for (i1=0; i1<nxny; i1++)
  for (j1=0; j1<nxny; j1++)
  setboard[i1][j1] = setboardsave[i1][j1];
  ndigits=nxny*nxny;
  fastalg=2;
  for (i2=0;i2<nindices;i2++)
  {
    i = indices[i2]>>SHIFTIT;
    j = indices[i2]&SHIFTMASK;
    for (i1=0; i1<nxny; i1++)
    for (j1=0; j1<nxny; j1++)
    board[i1][j1] = setboard[i1][j1];
    if (symmetrical==1)
    {
      board[(nxny/2)+i][(nxny/2)+j] = 0;
      board[(nxny/2)+i][((nxny-1)/2)-j] = 0;
      board[((nxny-1)/2)-i][(nxny/2)+j] = 0;
      board[((nxny-1)/2)-i][((nxny-1)/2)-j] = 0;
    }
    else if (symmetrical==2)
    {
      board[i][j] = 0;
      board[nxny-1-i][nxny-1-j] = 0;
    }
    else 
    {
      board[i][j] = 0;
    }
    nsol=0;
    findpossible();
    nmove=0;
    movelimit=MOVELIMIT;
    move();
    if (stop_pressed) return;
    if (nsol==1)
    {
      if (symmetrical==1)
      {
	if (setboard[(nxny/2)+i][(nxny/2)+j])
	{
	  setboard[(nxny/2)+i][(nxny/2)+j] = 0;
	  ndigits--;
	}
	if (setboard[(nxny/2)+i][((nxny-1)/2)-j])
	{
	  setboard[(nxny/2)+i][((nxny-1)/2)-j] = 0;
	  ndigits--;
	}
	if (setboard[((nxny-1)/2)-i][(nxny/2)+j])
	{
	  setboard[((nxny-1)/2)-i][(nxny/2)+j] = 0;
	  ndigits--;
	}
	if (setboard[((nxny-1)/2)-i][((nxny-1)/2)-j])
	{
	  setboard[((nxny-1)/2)-i][((nxny-1)/2)-j] = 0;
	  ndigits--;
	}
      }
      else if (symmetrical==2)
      {
	setboard[i][j] = 0;
	ndigits--;
	if (setboard[nxny-1-i][nxny-1-j])
	{
	  setboard[nxny-1-i][nxny-1-j] = 0;
	  ndigits--;
	}
      }
      else
      {
	setboard[i][j] = 0;
	ndigits--;
      }
      if (ndigits<=nfilled) break;
    }
  }
  if (testdigits()) return;
}

void unsolve_sudoku()
{
  int i,j;
  for (i=0; i<nxny; i++)
  for (j=0; j<nxny; j++)
  displayboard[i][j] = beforeboard[i][j];
  progstat=2;
}

int solve_sudoku(int newprintout)
{
  int i,j;
  fastalg=1;
  for (i=0; i<nxny; i++)
  for (j=0; j<nxny; j++)
  {
    board[i][j] = displayboard[i][j];
    beforeboard[i][j] = displayboard[i][j];
  }
  randomize=0;
  nsol=0;
  findpossible();
  movelimit=0;
  move();
  if (nsol!=1)
  {
    if (nsol>1)
    {
      for (i=0; i<nxny; i++)
      for (j=0; j<nxny; j++)
      displayboard[i][j] = beforeboard[i][j]?board[i][j]:-board[i][j];
      progstat=4;
    }
    return nsol;
  }

  fastalg=0;
  for (i=0; i<nxny; i++)
  for (j=0; j<nxny; j++)
  {
    board[i][j] = displayboard[i][j];
    beforeboard[i][j] = displayboard[i][j];
  }
  randomize=0;
  nsol=1;
  nmultiple=0;
  nsccell=0;
  nscnumber=0;
  nssc=0;
  nds=0;
  findpossible();
  printout=newprintout;
  movelimit=0;
  move();
  // logprintf("Z");
  thislevel=findcategory();
  printout=0;
  if (nsol>0)
  {
    for (i=0; i<nxny; i++)
    for (j=0; j<nxny; j++)
    displayboard[i][j] = beforeboard[i][j]?board[i][j]:-board[i][j];
    progstat=4;
  }
  return 1;
}

static char logtext[1024];

#define NUMBER(p) (((*(p))-'0')*10+((*((p)+1))-'0'))

int logtext_en(char *p, char **txt)
{
  int i,n;
  char buf1[512];
  *txt = logtext;
  switch(*p)
  {
  case 'A':
    sprintf(logtext, "%c is the only candidate in (%d,%d)", p[5], NUMBER(p+1), NUMBER(p+3));
    return 6;
  case 'B':
    sprintf(logtext, "%c in (%d,%d) is the only candidate in row %d", p[7], NUMBER(p+1), NUMBER(p+3), NUMBER(p+5));
    return 8;
  case 'C':
    sprintf(logtext, "%c in (%d,%d) is the only candidate in column %d", p[7], NUMBER(p+1), NUMBER(p+3), NUMBER(p+5));
    return 8;
  case 'D':
    sprintf(logtext, "%c in (%d,%d) is the only candidate in box [%d,%d]", p[9], NUMBER(p+1), NUMBER(p+3), NUMBER(p+5), NUMBER(p+7));
    return 10;
  case 'E':
    sprintf(logtext, "Eliminating %c from (%d,%d) because of single sector occurance in row %d", p[7], NUMBER(p+1), NUMBER(p+3), NUMBER(p+5));
    return 8;
  case 'F':
    sprintf(logtext, "Eliminating %c from (%d,%d) because of single sector occurance in column %d", p[7], NUMBER(p+1), NUMBER(p+3), NUMBER(p+5));
    return 8;
  case 'G':
    sprintf(logtext, "Eliminating %c from (%d,%d) because of single row occurance in box [%d,%d]", p[9], NUMBER(p+1), NUMBER(p+3), NUMBER(p+5), NUMBER(p+7));
    return 10;
  case 'H':
    sprintf(logtext, "Eliminating %c from (%d,%d) because of single column occurance in box [%d,%d]", p[9], NUMBER(p+1), NUMBER(p+3), NUMBER(p+5), NUMBER(p+7));
    return 10;
  case 'I':
    n=NUMBER(p+1);
    sprintf(logtext, "The values");
    for (i=0;i<n;i++)
    {
      sprintf(buf1, " %c", p[i+3]);
      strcat(logtext, buf1);
    }
    sprintf(buf1, " occupy the cells");
    strcat(logtext, buf1);
    for (i=0;i<n;i++)
    {
      sprintf(buf1, " (%d,%d)", NUMBER(p+n+i*4+3), NUMBER(p+n+i*4+5));
      strcat(logtext, buf1);
    }
    sprintf(buf1, " in some order");
    strcat(logtext, buf1);
    return n*5+3;
  case 'J':
    sprintf(logtext, "Eliminating %c from (%d,%d)", p[5], NUMBER(p+1), NUMBER(p+3));
    return 6;
  case 'K':
    sprintf(logtext, "The SSC elimination gave us a unique candidate: %c in (%d,%d)", p[5], NUMBER(p+1), NUMBER(p+3));
    return 6;
  case 'L':
    sprintf(logtext, "The Disjoint Subset elimination gave us a unique candidate: %c in (%d,%d)", p[5], NUMBER(p+1), NUMBER(p+3));
    return 6;
  case 'M':
    n=NUMBER(p+1);
    sprintf(logtext, "Guessing: %d candidates for (%d,%d):", n, NUMBER(p+3), NUMBER(p+5));
    for (i=0; i<n; i++)
    {
      sprintf(buf1, " %c", p[i+7]);
      strcat(logtext, buf1);
    }
    return n+7;
  case 'N':
    sprintf(logtext, "Guess result: Using %c for (%d,%d)", p[5], NUMBER(p+1), NUMBER(p+3));
    return 6;
  case 'O':
    n=NUMBER(p+1);
    sprintf(logtext, "%d candidates in (%d,%d):", n, NUMBER(p+3), NUMBER(p+5));
    for (i=0; i<n; i++)
    {
      sprintf(buf1, " %c", p[i+7]);
      strcat(logtext, buf1);
    }
    return n+7;
  case 'Y':
    sprintf(logtext, "No solution");
    return 1;
  case 'Z':
    sprintf(logtext, "All done");
    return 1;
  }
  sprintf(logtext, "Invalid logentry: %c", p[0]);
  return 1;
}

void 
make_board_init() {
//   unsigned int seed;
//   char *env = getenv("SUDOKU_SEED");
//   if (env) {
//     seed=atoi(env);
//   } else {
//     seed=time(NULL);
//   }
//   srandom(seed);

  init_bitmaps(nxny);
  progstat = 0;
}
