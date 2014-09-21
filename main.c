#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "globals.h"
#include "organism.h"
#include "map.h"

int bestScore = 0;

static void copyOrganism(organism* to, organism* from){ // Doesn't actually copy - Rather, births a new one with the same brain layout. Only difference is that memory values are reset - should this be changed???
	to->memStart = 0;
	char* memory = to->memory;
	int i = mem/2;
	for(; i < mem-mem/4; i++){
		memory[i] = 0;
		memory[i+mem/4] = 1;
	}
	neuron* holdThis = from->neurons;
	to->neurons = malloc(sizeof(neuron)* (to->numNeurons=from->numNeurons) );
	memcpy(to->neurons, holdThis, sizeof(neuron)*to->numNeurons);
	to->score = to->age = 0;
	memset(to->myTape, 0, tapeSize);
}

static void mutateAddNeuron(organism* who){ // Copy from an existing neuron
	who->neurons = realloc(who->neurons, sizeof(neuron)*(++who->numNeurons));
	neuron* tmp = who->neurons+who->numNeurons-1;
	if(who->numNeurons-1){
		int i = random()%(who->numNeurons-1);
		memcpy(tmp, who->neurons+i, sizeof(neuron));
		i = random()%mem;
		int j, set=0;
		for(set = 0; set < 3; set++){
			for(j=0; j < tmp->nums[set]; j++){
				tmp->sets[set][j] = (tmp->sets[set][j]+i)%mem;
			}
		}
		tmp->value = (tmp->value+i)%mem;
	}else{
		tmp->nums[0] = tmp->nums[1] = tmp->nums[2] = 0;
		tmp->value = random()%mem;
	}
}

static char mutateChangeNeuron(organism* who, neuron* n){
	int type = random()%8;
	if(type >= 7){ // Then ur dead lol
		memcpy(n, who->neurons+(--who->numNeurons), sizeof(neuron));
		who->neurons = realloc(who->neurons, sizeof(neuron)*who->numNeurons);
		return 0;
	}else if(type/3 == 0){
		if(n->nums[type]){
			n->nums[type]--;
			n->sets[type][random()%(1+n->nums[type])] = n->sets[type][n->nums[type]];
		}
	}else if(type/3 == 1){
		int candidate = random()%mem;
		type-=3;
		int* tmp = n->sets[type];
		int i = n->nums[type]-1;
		for(; i>=0; i--){
			if(tmp[i] == candidate) return 1; // Potential spot for improvement - should I keep the memory locations sorted?
		}
		tmp[n->nums[type]++] = candidate;
	}else n->value = random()%mem;
	return 1;
}

static void mutateOrganism(organism* who){
	while(random() < RAND_MAX/25) mutateAddNeuron(who);
	int i = 0;
	while(i < who->numNeurons){
		if(random() < RAND_MAX/20){
			if(mutateChangeNeuron(who, who->neurons+i)) i++;
		}else i++;
	}
}

static int globalValue1, globalValue2;

static void doInput(organism* who){
	char* memory = who->memory + who->memStart;
	int i = 0;
	for(; i < globalValue2; i++) memory[i] = 1;
}

static char doOutput(organism* who){
	if(who->age==0){// If it hasn't been alive long enough to get the previous number, give it the benefit of the doubt.
		who->age = 1;
		who->score=10;
		return 0;
	}
	char* memory = who->memory + who->memStart;
	int i = 10;
	int out = 0;
	for(; i < 20; i++){
		if(memory[i])
			out++;
	}
	out = abs(out-globalValue1);
	who->age+=1+out;
	who->score += 10-out;

	if(who->age >= 100){
		if(who->score > bestScore){
			bestScore = who->score;
			exportOrganism(who, "best.org");
			printf("%d\n", bestScore);
		}
		return 1;
	}
	return 0;
}

static int *subPopMasks, *subPopSizes;

static void initSubPops(){
	subPopMasks = malloc(sizeof(int)*kidsLog2);
	subPopSizes = malloc(sizeof(int)*kidsLog2);
	int tmp=numKids;
	int tmp2 = 0;
	int i = kidsLog2-1;
	for(; i >= 0; i--){
		subPopSizes[i]=tmp;
		subPopMasks[i]=tmp2;
		tmp /= 2;
		tmp2 |= tmp;
	}
}

//Decides which index to clone to replace the dead. Uses a system of sub-populations (simulating geographic separation, etc.) where the donor is more likely to come from the same "continent", "region" or similar structure. Should allow for more diversity.
static int getDonor(int victim){
	//return random()%subPopSizes[kidsLog2-1];
	int level = random()%kidsLog2;
	return (subPopMasks[level] & victim) + (random() % subPopSizes[level]);
}

static void simulate(){
#ifdef CONTINENTS
	initSubPops();
#endif
	initMap();

	organism steve = {.numNeurons=0};
	importOrganism(&steve, "best.org");
	organism *allMyChildren = malloc(sizeof(organism)*numKids);
	int i;
	for(i = 0; i < numKids; i++){
		copyOrganism(allMyChildren+i, &steve);
		spawnGuy(allMyChildren+i);
	};

	organism *tmp, *end = allMyChildren+numKids;
	neuron* holdThis;

	int j=0;
	while(1){
		for(i=0; i<25; i++){
			for(tmp=allMyChildren; tmp<end; tmp++){
				prepOrganismMemory(tmp);
				doInputGuy(tmp);
				tickOrganism(tmp);
				doOutputGuy(tmp);
			}
		}
		i=0;
		for(tmp=allMyChildren; tmp<end; tmp++){
			if(doStepGuy(tmp)){
				holdThis = tmp->neurons;
#ifdef CONTINENTS
				copyOrganism(tmp, allMyChildren+getDonor(i));
#else
				copyOrganism(tmp, allMyChildren+random()%numKids);
#endif
				spawnGuy(tmp);
				free(holdThis); // Why wait? In case we copy from ourselves.
				mutateOrganism(tmp);
			}
			i++;
		}
		if(++j%2000 == 0){
			printMap();
			fputs("\n\n\n", stdout);
			if(j == 2000*300){
				j = 0;
				quitMap();
				initMap(); // Get a new map
				for(tmp=allMyChildren; tmp<end; tmp++){
					holdThis = tmp->neurons;
					copyOrganism(tmp, tmp); // Clear it out
					free(holdThis);
					spawnGuy(tmp);
				}
			}
		}
	}
#ifdef CONTINENTS
	free(subPopMasks);
	free(subPopSizes);
#endif
	free(allMyChildren);
	quitMap();
}

static void converse(){
	organism alex;
	importOrganism(&alex, "best.org");
	char input[20];
	char* memory = alex.memory;
	while(fgets(input, 20, stdin)){
		prepOrganismMemory(&alex);

		if(*input>='0' && *input<='9'){
			int in = atoi(input);
			for(in--; in>=0; in--) memory[in] = 1;
		}else{
			int i = 0;
			while(input[i]){
				if((input[i]>='a'&&input[i]<='z') || (input[i]>='A'&&input[i]<='Z')) memory[i]=1;
				i++;
			}
		}

		tickOrganism(&alex);

		memory = alex.memory+alex.memStart;
		int i = 10, out = 0;
		for(; i < 20; i++) if(memory[i]) out++;
		printf("%d\n", out);
	}
	free(alex.neurons);
}

static void navigate(){
#ifdef CONTINENTS
	initSubPops();
#endif
	initMap();

	organism steve = {.numNeurons=0};
	organism *allMyChildren = malloc(sizeof(organism)*2);
	int i;
	for(i = 0; i < 2; i++){
		copyOrganism(allMyChildren+i, &steve);
		spawnGuy(allMyChildren+i);
	};

	organism *tmp, *end = allMyChildren+2;
	neuron* holdThis;
	char input[10];

	int j=0;
	while(1){
		for(i=0; i<5; i++){
			for(tmp=allMyChildren; tmp<end; tmp++){
				prepOrganismMemory(tmp);
				doInputGuy(tmp);
				printf("Guy %ld\n", tmp-allMyChildren);

				if(tmp->memory[0]) puts("Door to left");
				if(tmp->memory[1]) puts("Door ahead");
				if(tmp->memory[2]) puts("Door to right");
				if(tmp->memory[3]) puts("A brand new step!");
				if(tmp->memory[4]) puts("Our own");
				printf("Tape reads: %d\n", tmp->memory[5]);
				if(NULL == fgets(input, 10, stdin)) return; // No cleanup, because we're monsters.
				int i = 0;
				for(; i < 5; i++){
					if(input[i]>='a'&&input[i]<='z') tmp->memory[10+i] = 1;
				}
				if(input[5]) tmp->memory[5] = input[5]>='a'&&input[5]<='z';
				doOutputGuy(tmp);
			}
		}
		i=0;
		for(tmp=allMyChildren; tmp<end; tmp++){
			if(doStepGuy(tmp)){
				printf("Oh no guy %d died!\n", i);
				holdThis = tmp->neurons;
#ifdef CONTINENTS
				copyOrganism(tmp, allMyChildren+getDonor(i));
#else
				copyOrganism(tmp, allMyChildren+random()%2);
#endif
				spawnGuy(tmp);
				free(holdThis); // Why wait? In case we copy from ourselves.
				mutateOrganism(tmp);
			}
			i++;
		}
		if(++j%2000 == 0){
			printMap();
			fputs("\n\n\n", stdout);
			if(j == 2000*300){
				j = 0;
				quitMap();
				initMap(); // Get a new map
				for(tmp=allMyChildren; tmp<end; tmp++){
					holdThis = tmp->neurons;
					copyOrganism(tmp, tmp); // Clear it out
					free(holdThis);
					spawnGuy(tmp);
				}
			}
		}
	}
#ifdef CONTINENTS
	free(subPopMasks);
	free(subPopSizes);
#endif
	free(allMyChildren);
	quitMap();
}

int main(int argc, char** argv){
	srandom(time(NULL));
	simulate();
	//converse();
	//navigate();
	return 0;
}
