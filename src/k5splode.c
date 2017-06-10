/* k5splode.c - source for generating Keen 5 galaxy explosion starfield data
 ** 
 ** Copyright (c)2016-2017 by Owen Pierce
 **
 **
 ** This software is provided 'as-is', without any express or implied warranty.
 ** In no event will the authors be held liable for any damages arising from
 ** the use of this software.
 ** Permission is granted to anyone to use this software for any purpose, including
 ** commercial applications, and to alter it and redistribute it freely, subject
 ** to the following restrictions:
 **    1. The origin of this software must not be misrepresented; you must not
 **       claim that you wrote the original software. If you use this software in
 **       a product, an acknowledgment in the product documentation would be
 **       appreciated but is not required.
 **    2. Altered source versions must be plainly marked as such, and must not be
 **       misrepresented as being the original software.
 **    3. This notice may not be removed or altered from any source distribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "bmp256.h"
#include "utils.h"
#include "evald.h"

#define		NUM_POINTS		4000
#define		SCREEN_W		320
#define		SCREEN_H		200
#define		SCREEN_AREA		(SCREEN_W * SCREEN_H)

#define		SCR_PRIME		22943
#define		MAX_POINTSETS	256

typedef struct point_t {
	short x;
	short y;
} Point;

typedef struct {
	int points;
	char filename[32];
	char Xcomponent[128];
	char Ycomponent[128];
} PointSet;


unsigned	short	MSCposX[NUM_POINTS];
signed		short	MSCdelX[NUM_POINTS];
unsigned	short	MSCposY[NUM_POINTS];
signed		short	MSCdelY[NUM_POINTS];
PointSet 	pointsets[MAX_POINTSETS];

void destroy_point(Point *p);
void import_points(BITMAP256 *bmp, unsigned int start, unsigned int pnum);
void import_motions(char *xcomp, char *ycomp, int start, int pnum); 
void export_MSC(char *outfile);
void parse_instructions(char *infile);
void parse_switches(void);

//convert screen coords to cartesion plane with origin at screen centre
#define X_scr(x) (((x)+SCREEN_W/2)*128)
#define X_car(x) ((x)/128-SCREEN_W/2)
#define Y_scr(y) ((-(y)+SCREEN_H/2)*128)
#define Y_car(y) (SCREEN_H/2-(y)/128)

/* 
import_points
places num starting points beginning at index start using data from bmp
*/
void import_points(BITMAP256 *bmp, unsigned int start, unsigned int pnum) {

	unsigned int pcnt,i,j, *allpoints;
	char screengrid[SCREEN_AREA];
	
	/* 1st pass: put all non-black points into point array */
	memset(&screengrid[0], 0, SCREEN_AREA);
	for (i = pcnt = 0; i < SCREEN_AREA; i++) {
		if (bmp256_getpixel(bmp, i%SCREEN_W, i/SCREEN_W)) {
			screengrid[i]++;
			pcnt++;
		}
	}

	allpoints = (unsigned int*)malloc(pcnt * sizeof(int));

	for (i = 0, j = 0; i < SCREEN_AREA; i++) {
		if (screengrid[i]) allpoints[j++] = i;
	}

	/* 2nd pass; randomly choose points from point array, put in msc array */
	for (i = 0; i < pnum && i < pcnt; i++) {
		j = rand() % pcnt;
		for (j = rand()%pcnt; allpoints[j] == 0; j = (j+1)%pcnt) continue;

		MSCposX[start+i] = allpoints[j] % SCREEN_W * 128;
		MSCposY[start+i] = allpoints[j] / SCREEN_W * 128;
		allpoints[j] = 0;
	}

	free(allpoints);
	printf("Put %d points at %d. (Found %d)\n",
	 (pcnt<pnum?pcnt:pnum), start, pcnt);
		
	return;

}

void import_motions(char *xcomp, char *ycomp, int start, int pnum) {
	
	double dx, dy, x, y;
	int ecode, epos, i;

	char xbuf[16], ybuf[16];

	for (i = start; i < start + pnum; i++) {
		x = X_car((double)MSCposX[i]);
		y = Y_car((double)MSCposY[i]);
		sprintf(xbuf, "x=%.2f", x);
		sprintf(ybuf, "y=%.2f", y);
		
		/* Update args */
		EvaluateD(xbuf, &dx, &epos); 
		EvaluateD(ybuf, &dy, &epos); 
		
		/* Calc X component */
		ecode = EvaluateD(xcomp,&dx,&epos); 
		if (ecode != E_OK) {
			quit("Improper X component in %s\n"
			 "%s at char %d\n" 
			 "Point=%d X=%d Y=%d\n", xcomp, ErrMsgs[ecode], epos, i, 
			 (int)x,(int)y);
		}

		/* Calc Y component */
		ecode = EvaluateD(ycomp,&dy,&epos);
		if (ecode != E_OK) {
			quit("Improper Y component in %s\n"
			 "%s at char %d\n" 
			 "Point=%d X=%d Y=%d\n", ycomp, ErrMsgs[ecode], epos, i, 
			 (int)x,(int)y);
		}

		MSCdelX[i] = (signed short)(dx * 128);
		MSCdelY[i] = (signed short)(dy * -128);
	}

	printf("Pointset at %d:\n\tX(x,y)=%s\n\tY(x,y)=%s\n", start, xcomp,ycomp);

	return;

}


void export_MSC(char *outfile) {

	FILE *fp;

	fp = fopen(outfile, "wb");
	fwrite(&MSCposX, sizeof(short), NUM_POINTS, fp);
	fwrite(&MSCdelX, sizeof(short), NUM_POINTS, fp);
	fwrite(&MSCposY, sizeof(short), NUM_POINTS, fp);
	fwrite(&MSCdelY, sizeof(short), NUM_POINTS, fp);
	fclose(fp);

	return;
	
}

#define LINEBUF_SIZE 512
#define WHITESPACE " \t\n\r"
void parse_instructions(char *infile) {
	
	FILE *fp;
	char *cp;
	char linebuf[LINEBUF_SIZE];
	int i;
	
	if ((fp = fopen(infile, "r")) == NULL) {
		quit("Could not find Instruction File");
		return;
	}

	i = 0;
	while(!feof(fp) && fgets(linebuf, LINEBUF_SIZE, fp) != NULL) {
		if (strspn(linebuf, WHITESPACE "#")) continue;
		sscanf(linebuf, "%d %s", &pointsets[i].points, pointsets[i].filename);
		cp = strchr(linebuf, '<') + 1;
		strncpy(pointsets[i].Xcomponent, cp, strcspn(cp,";"));
		cp = strchr(linebuf, ';') + 1;
		strncpy(pointsets[i].Ycomponent, cp, strcspn(cp,">"));
		i++;
	}

	fclose(fp);

	return;

}

#if 0
int main(int argc, char *argv[]) {
	
	PointSet *ps;
	int pi;
	char *infile, *outfile;
	
	/*Check for Input, Output files */
	if (argc == 1) {
		printf("K5SPLODE.EXE USAGE:\nK5SPLODE.EXE <infile> <outfile>");
		return 1;
	}
	if (argc != 3) {
		quit("Incorrect number of arguments");
		return 1;
	}

	infile = argv[1];
	outfile = argv[2];

	srand(time(NULL));

	memset(pointsets, 0, sizeof(PointSet) * MAX_POINTSETS);
	parse_instructions(infile);

	pi = 0;
	ps = pointsets;
	while(ps->points) { 
		import_points(bmp256_load(ps->filename),pi ,ps->points);
		import_motions(ps->Xcomponent, ps->Ycomponent, pi, ps->points);
		if ((pi += ps->points) >= 4000) {
			quit("Points limit exceeded!");
			return 1;
		}
		ps++;
	}
	
	export_MSC(outfile);

	return 0;
}
#endif

