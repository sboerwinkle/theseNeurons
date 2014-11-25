#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	unsigned char fields[3][256];
	int lengths[3];
	unsigned char value;
	int shift;
} neuron;

int parseNum(int val)
{
	int c = getc(stdin);
	if (c < '0' || c > '9')
		return val;
	return parseNum(val * 10 + c - '0');
}

char ui[256];
char uo[256];

int main(int argc, char **argv)
{
	memset(ui, 0, 256);
	memset(uo, 0, 256);
	memset(ui+10, 1, 6);
	ui[4] = 1;
	memset(uo+1, 1, 5);

	parseNum(0);
	int val = parseNum(0);
	neuron* ns = calloc(val, sizeof(neuron));
	int i = val-1;
	for (; i >= 0; i--) {
		int j = 0, k;
		for (; j < 3; j++) {
			ns[i].lengths[j] = parseNum(0);
			for (k = 0; k < ns[i].lengths[j]; k++) {
				int n = parseNum(0);
				if (j < 2)
					ui[n] = 1;
				else
					uo[n] = 1;
				ns[i].fields[j][k] = n;
			}
			int c = getc(stdin);
			if (c != '\n')
				ungetc(c, stdin);
		}
		int n = parseNum(0);
		ui[n] = 1;
		ns[i].value = n;
	}
	printf("0 %d\n", val);
	for (i = val - 1; i >= 0; i--) {
		int j = 0, k;
		for(; j<3; j++){
			for(k = 0; k < ns[i].lengths[j]; k++) {
				int n = ns[i].fields[j][k];
				if (j < 2) {
					//if (uo[n]) {
						printf("%d ", n);
					/*} else {
						if ((n / 64) % 2) {
							ns[i].shift += (j == 1) ? 1 : -1;
						}
					}*/
				} else {
					if (ui[n])
						printf("%d ", n);
				}
			}
			/*if (j == 1 && ns[i].shift) {
				if (ns[i].shift > 0)
					printf("+%d", ns[i].shift);
				else
					printf("-%d", -ns[i].shift);
			}*/
			fputc('\n', stdout);
		}
		//if (uo[ns[i].value])
			printf("%d\n", ns[i].value);
		//else
		//	puts(((ns[i].value / 64) % 2) ? "[1]" : "[0]");
		//puts("--------------");
	}
	free(ns);
	return 0;
}
