#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int parseNum(int val)
{
	int c = getc(stdin);
	if (c < '0' || c > '9')
		return val;
	return parseNum(val * 10 + c - '0');
}

typedef struct {
	float x, y;
} point;

typedef struct {
	point loc;
	char value;
	int mutators;
	int numReading;
} cell;

typedef struct {
	int sizes[3];
	int *(cells[3]);
	int value;
	char valueStable;
	point loc;
} neuron;

cell cells[256];
neuron* neurons;
int numNeurons;

void paintCell(int ix)
{
	float x = cells[ix].loc.x;
	float y = cells[ix].loc.y;
#define w 0.04
	glBegin(GL_LINE_LOOP);
	glVertex3f(x-w, y+w, 0);
	glVertex3f(x+w, y+w, 0);
	glVertex3f(x+w, y-w, 0);
	glVertex3f(x-w, y-w, 0);
	glEnd();
	if (cells[ix].value) {
		glBegin(GL_TRIANGLES);
		glVertex3f(x-w, y-w, 0);
		glVertex3f(x+w, y+w, 0);
		glVertex3f(x-w, y+w, 0);
		glEnd();
	}
	if ((ix & 64)<<1 ^ (ix & 128)) {
		glBegin(GL_TRIANGLES);
		glVertex3f(x+w, y+w, 0);
		glVertex3f(x-w, y-w, 0);
		glVertex3f(x+w, y-w, 0);
		glEnd();
	}
}

void addLine(int n, int c, int X, int Y)
{
	glVertex3f(neurons[n].loc.x, neurons[n].loc.y, 0);
	glVertex3f(cells[c].loc.x + X*w, cells[c].loc.y + Y*w, 0);
}

void drawGroup(int n, int *cells, int numCells, char color, int X, int Y)
{
	glColor3f(color & 4, color & 2, color & 1);
	glBegin(GL_LINES);
	for (numCells--; numCells >= 0; numCells--) {
		addLine(n, cells[numCells], X, Y);
	}
	glEnd();
}

void paintNeuron(int n) {
	drawGroup(n, neurons[n].cells[0], neurons[n].sizes[0], 3, 1, 1);
	drawGroup(n, neurons[n].cells[1], neurons[n].sizes[1], 1, 1, -1);
	drawGroup(n, neurons[n].cells[2], neurons[n].sizes[2], 4, -1, -1);
	glColor3f(1, 1, 1);
	glBegin(GL_LINES);
	addLine(n, neurons[n].value, -1, 1);
	glEnd();
	float x = neurons[n].loc.x;
	float y = neurons[n].loc.y;
	glColor3f(1, 1, 0);
	glBegin(GL_TRIANGLES);
	glVertex3f(x, y+w, 0);
	glVertex3f(x-w*.866, y-w/2, 0);
	glVertex3f(x+w*.866, y-w/2, 0);
	glEnd();
}

void paint(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	int i = 0;
	for (; i < numNeurons; i++)
		paintNeuron(i);
	glColor3f(1.0, 1.0, 1.0);
	for (i = 0; i < 256; i++)
		paintCell(i);
	glutSwapBuffers();
}

void freeStuff()
{
	int i = numNeurons-1, j;
	for (; i >= 0; i--) {
		for (j = 0; j < 3; j++) {
			free(neurons[i].cells[j]);
		}
	}
	free(neurons);
}

float toGlSpace(int val, char y)
{
	return (float)(val-400) / (y?-400:400);
}

point *getNearest(int X, int Y)
{
	float x = toGlSpace(X, 0);
	float y = toGlSpace(Y, 1);
	float best = w*3;
	point* ret = NULL;
	int i = 0;
	for (; i < numNeurons; i++) {
		float score = fabsf(x-neurons[i].loc.x) + fabsf(y-neurons[i].loc.y);
		if (score < best) {
			best = score;
			ret = &neurons[i].loc;
		}
	}
	for (i = 0; i < 256; i++) {
		float score = fabsf(x-cells[i].loc.x) + fabsf(y-cells[i].loc.y);
		if (score < best) {
			best = score;
			ret = &cells[i].loc;
		}
	}
	return ret;
}

void allDemReglarKeys(unsigned char code, int x, int y)
{
	if (code == 'd') {
		point *loc = getNearest(x, y);
		if (loc) {
			loc->x = -1;
			loc->y = 1;
			paint();
		}
	}
	if (code == 'A' || code == 27) {
		freeStuff();
		exit(0);
	}
}

point *grabbed = NULL;
int moveCounter = 0;

void allDemMouseButtons(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN) {
		moveCounter = 0;
		grabbed = getNearest(x, y);
	} else {
		paint();
	}
}

void allDatMouseMotion(int x, int y)
{
	if (grabbed) {
		grabbed->x = toGlSpace(x, 0);
		grabbed->y = toGlSpace(y, 1);
		if (++moveCounter == 5) {
			moveCounter = 0;
			paint();
		}
	}
}

char globalStuffChangedFlag = 1;
int count = 0;

void analyzeNeuron(int N)
{
	neuron *n = neurons + N;
	int *curList;
	if (cells[n->value].mutators == 0 && n->valueStable == 0) {
		globalStuffChangedFlag = 1;
		n->valueStable = 1;
		int value = cells[n->value].value;
		int i = n->sizes[2] - 1;
		curList = n->cells[2];
		for (; i >= 0; i--) {
			if (cells[curList[i]].value == value)
				cells[curList[i]].mutators--;
		}
	}
	int i, j = 0;
	for (; j < 2; j++) {
		i = n->sizes[j] - 1;
		curList = n->cells[j];
		for (; i >= 0; i--) {
			if (cells[curList[i]].value == 0 && cells[curList[i]].mutators == 0) {
				globalStuffChangedFlag = 1;
				count++;
				cells[curList[i]].numReading--;
				curList[i] = curList[--(n->sizes[j])];
			}
		}
	}
	i = n->sizes[0] - 1;
	j = n->sizes[1] - 1;
	curList = n->cells[0];
	int *otrList = n->cells[1];
	for (; i >= 0; i--) {
		if (cells[curList[i]].value == 1 && cells[curList[i]].mutators == 0) {
			for (; j >= 0 && (cells[otrList[j]].value != 1 || cells[otrList[j]].mutators != 0); j--);
			if (j < 0) {
				i = -1;
			} else {
				globalStuffChangedFlag = 1;
				count += 2;
				cells[curList[i]].numReading--;
				cells[otrList[j]].numReading--;
				curList[i] = curList[--(n->sizes[0])];
				otrList[j] = otrList[--(n->sizes[1])];
				j--;
			}
		}
	}
	i = n->sizes[1] - 1;
	j = 0;
	curList = n->cells[1];
	for (; i >= 0; i--) {
		if (cells[curList[i]].value == 1 && cells[curList[i]].mutators == 0) {
			j++;
		}
	}
	if (j >= n->sizes[0])
		puts("Yippee ki yay!");
	i = n->sizes[2] - 1;
	curList = n->cells[2];
	for (; i >= 0; i--) {
		if (cells[curList[i]].numReading == 0 || cells[curList[i]].mutators == 0) {
			globalStuffChangedFlag = 1;
			count++;
			if (!n->valueStable || cells[curList[i]].value != cells[n->value].value) {
				cells[curList[i]].mutators--;
			}
			curList[i] = curList[--(n->sizes[2])];
		}
	}
}

int main(int argc, char **argv)
{
	int i = 0, j;
	for (; i < 16; i++) {
		for (j = 0; j < 16; j++) {
			cells[16*i + j].loc.x = (1+2*i)/16.0 - 1;
			cells[16*i + j].loc.y = (1+2*j)/16.0 - 1;
			cells[16*i + j].value = (i / 4) % 2;
			cells[16*i + j].mutators = 0;
			cells[16*i + j].numReading = 0;
		}
	}
	for (i = 1; i <= 5; i++) {
		cells[i].mutators = 1;
	}
	for (i = 10; i < 16; i++) {
		cells[i].numReading = 1;
	}
	cells[4].numReading = 1;
	parseNum(0);
	numNeurons = parseNum(0);
	neurons = calloc(numNeurons, sizeof(neuron));
	for (i = numNeurons - 1; i >= 0; i--) {
		int j = 0, k;
		for (; j < 3; j++) {
			neurons[i].sizes[j] = parseNum(0);
			neurons[i].cells[j] = malloc(neurons[i].sizes[j] * sizeof(int));
			for (k = 0; k < neurons[i].sizes[j]; k++) {
				int c = parseNum(0);
				neurons[i].cells[j][k] = c;
				if (j == 2) {
					cells[c].mutators++;
				} else {
					cells[c].numReading++;
				}
			}
			int c = getc(stdin);
			if (c != '\n')
				ungetc(c, stdin);
		}
		int c = parseNum(0);
		neurons[i].value = c;
		cells[c].numReading++;
		neurons[i].valueStable = 0;
	}
	while (globalStuffChangedFlag) {
		globalStuffChangedFlag = 0;
		int i = numNeurons-1;
		for (; i >= 0; i--) {
			analyzeNeuron(i);
		}
	}
	for (i = 0; i < 256; i++) {
		if (cells[i].numReading == 0 && cells[i].mutators == 0) {
			cells[i].loc.x = -1;
			cells[i].loc.y = 1;
		}
	}
	printf("%d %d\n", count, numNeurons);
	int k = 0;
	neuron *n;
	for (; k < numNeurons; k++) {
		n = neurons + k;
		for(i = 0; i < 3; i++){
			printf("%d ", n->sizes[i]);
			for (j = n->sizes[i] - 1; j >= 0; j--) printf("%d ", n->cells[i][j]);
			fputc('\n', stdout);
		}
		printf("%d\n", n->value);
	}
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(-1, -1);
	glutInitWindowSize(800, 800);
	glutCreateWindow("GLUTtony");
	glutKeyboardFunc(allDemReglarKeys);
	glutMouseFunc(allDemMouseButtons);
	glutMotionFunc(allDatMouseMotion);
	glutDisplayFunc(paint);
	glutMainLoop();
	return 0;
}
