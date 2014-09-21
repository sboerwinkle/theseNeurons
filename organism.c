#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "globals.h"

void prepOrganismMemory(organism *who){
	char* memory = who->memory+who->memStart;
	int i = 0;
	for(; i < mem/4; i++){
		memory[i] = 0;
		memory[i+mem/4] = 1;
	}
}

void tickOrganism(organism *who){
	char* memory = who->memory+who->memStart;
	char* nextMemory = who->memory+(mem-who->memStart);
	char* tmp2;

	neuron* neurons = who->neurons;
	neuron* end = neurons + who->numNeurons;
	neuron* tmp;
	
	int i;

	for(tmp = neurons; tmp < end; tmp++) tmp->used = 0;
	char flag = 1;
	int val;
	while(flag){
		memcpy(nextMemory, memory, mem);
		flag = 0;
		for(tmp = neurons; tmp < end; tmp++){
			if(tmp->used) continue;
			val = 0;
			for(i = tmp->nums[0]-1; i >= 0; i--){
				if(memory[tmp->sets[0][i]]) val++;
			}
			for(i = tmp->nums[1]-1; i >= 0; i--){
				if(memory[tmp->sets[1][i]]) val--;
			}
			if(val <= 0) continue;
			flag = memory[tmp->value];
			for(i = tmp->nums[2]-1; i >= 0; i--){
				nextMemory[tmp->sets[2][i]] = flag;
			}
			tmp->used = 1;
			flag = 1;
		}
		tmp2 = memory;
		memory = nextMemory;
		nextMemory = tmp2;
	}
	who->memStart = memory-who->memory;
}

void importOrganism(organism *who, char *file){
	FILE* fp = fopen(file, "r");
	char* memory = who->memory;
	int j = mem/2;
	for(; j < mem-mem/4; j++){
		memory[j] = 0;
		memory[j+mem/4]=1;
	}
	who->memStart = 0;
	fscanf(fp, "%*d %d", &who->numNeurons);
	who->neurons = malloc(sizeof(neuron)*who->numNeurons);

	int k = 0;
	neuron *n;
	for(; k < who->numNeurons; k++){
		n = who->neurons+k;
		int i=0;
		for(; i<3; i++){
			fscanf(fp, "%d ", n->nums+i);
			for(j=n->nums[i]-1; j>=0; j--) fscanf(fp, "%d ", n->sets[i]+j);
		}
		fscanf(fp, "%d\n", &n->value);
	}

	who->score = who->age = 0;
	fclose(fp);
}

void exportOrganism(organism* who, char* file){
	FILE* fp = fopen(file, "w");
	fprintf(fp, "%d %d\n", mem, who->numNeurons);
	int k = 0;
	neuron *n;
	for(; k < who->numNeurons; k++){
		n = who->neurons+k;
		int i=0, j;
		for(; i<3; i++){
			fprintf(fp, "%d ", n->nums[i]);
			for(j=n->nums[i]-1; j>=0; j--) fprintf(fp, "%d ", n->sets[i][j]);
			fputc('\n', fp);
		}
		fprintf(fp, "%d\n", n->value);
	}
	fclose(fp);
}
