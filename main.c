#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include "globals.h"
#include "organism.h"
#include "map.h"

int bestScore = 0;

static long totalMutations[talliedGens+1][mTypes] = {{0}};

static void tallyMutation(organism *who, int type) {
	totalMutations[0][type]++;
	who->mTally[0][type]++;
}

static void tallyReproduction(organism *who, organism *kid) {
	int i = 0;
	int parentLevel = 1;
	int j;
	for (; i < talliedGens; i++) {
		for (j = mTypes-1; j >= 0; j--)
			totalMutations[parentLevel][j] += who->mTally[i][j];
		parentLevel++;
	}
	char mTally[talliedGens-1][mTypes];
	memcpy(mTally, who->mTally, mTypes*(talliedGens-1));
	memcpy(kid->mTally+1, mTally, mTypes*(talliedGens-1));
	memset(kid->mTally[0], 0, mTypes);
}

static void printTallies() {
	static char * const names[mTypes] = {
		"Additions:     ",
		"Cell Swaps:    ",
		"Removals:      ",
		"Con Removals:  ",
		"Con Additions: ",
		"+Chained:      ",
		"Type Changes:  ",
		"Sim Seeker:    ",
		"Untouched:     "};
	fputs("               Total  |", stdout);
	int i = 0, j;
	for (; i < talliedGens; i++) {
		for (j = 0; j < i; j++) fputc('G', stdout);
		fputc('P', stdout);
		for (; j < 6; j++) fputc(' ', stdout);
		fputc('|', stdout);
	}
	for (i = 1; i <= talliedGens; i++) fprintf(stdout, " %%S%d|", i);
	fputc('\n', stdout);
	for (i = 0; i < mTypes; i++) {
		fputs(names[i], stdout);
		for (j = 0; j < talliedGens + 1; j++) {
			printf("%7ld|", totalMutations[j][i]);
		}
		for (j = 0; j < talliedGens; j++) {
			if (totalMutations[j][i])
				printf("%4ld|", totalMutations[j+1][i]*100/totalMutations[j][i]);
			else
				fputs(" NAN|", stdout);
		}
		fputc('\n', stdout);
	}
}

static char writeEverythingToFile(organism *orgs, char *file)
{
	int fd = creat(file, 00666);
	if (fd == -1) {
		puts("aw shit");
		return 1;
	}
	int i = 0;
	write(fd, &bestScore, sizeof(int));
	write(fd, totalMutations, sizeof(totalMutations));
	for (; i < numKids; i++) {
		write(fd, orgs[i].memory+orgs[i].memStart, mem);
		write(fd, &orgs[i].numNeurons, sizeof(int));
		int j = 0;
		for (; j < orgs[i].numNeurons; j++) {
			write(fd, orgs[i].neurons[j].nums, 3*sizeof(int));
			int k = 0;
			for (; k < 3; k++)
				write(fd, orgs[i].neurons[j].sets[k], orgs[i].neurons[j].nums[k]*sizeof(int));
			write(fd, &orgs[i].neurons[j].value, sizeof(int));
		}
		write(fd, orgs[i].mTally, mTypes*talliedGens);
		write(fd, &orgs[i].score, sizeof(int));
		write(fd, &orgs[i].age, sizeof(int));
		write(fd, &orgs[i].myTape, tapeSize);
		write(fd, &orgs[i].whichTape, sizeof(int));
		write(fd, &orgs[i].headIndex, sizeof(int));
		write(fd, &orgs[i].newStep, 1);
		write(fd, &orgs[i].gotPoint, 1);
		write(fd, &orgs[i].myRoom, sizeof(int));
		write(fd, &orgs[i].dir, sizeof(int));
		write(fd, &orgs[i].moveDir, sizeof(int));
		int dif = orgs[i].myLoc - rooms[orgs[i].myRoom].occupants;
		write(fd, &dif, sizeof(int));
	}
	mapWriteEverythingToFile(fd);
	close(fd);
	return 0;
}

#define oldTalliedGens talliedGens
#define oldMTypes mTypes
//If either is modified, best call a 'reset'
static char readEverythingFromFile(organism *orgs, char *file, char reset)
{
	int fd = open(file, O_RDONLY);
	if (fd == -1) {
		puts("aw shit");
		return 1;
	}
	int i = 0;
	read(fd, &bestScore, sizeof(int));
	read(fd, totalMutations, sizeof(long)*oldMTypes*(oldTalliedGens+1));
	if (reset) {
		bestScore = 0;
		memset(totalMutations, 0, sizeof(totalMutations));
	}
	for (; i < numKids; i++) {
		orgs[i].memStart = 0;
		read(fd, orgs[i].memory, mem);
		read(fd, &orgs[i].numNeurons, sizeof(int));
		orgs[i].neurons = malloc(sizeof(neuron)*orgs[i].numNeurons);
		int j = 0;
		for (; j < orgs[i].numNeurons; j++) {
			read(fd, orgs[i].neurons[j].nums, 3*sizeof(int));
			int k = 0;
			for (; k < 3; k++)
				read(fd, orgs[i].neurons[j].sets[k], orgs[i].neurons[j].nums[k]*sizeof(int));
			read(fd, &orgs[i].neurons[j].value, sizeof(int));
		}
		read(fd, orgs[i].mTally, oldMTypes*oldTalliedGens);
		if (reset) {
			memset(orgs[i].mTally, 0, sizeof(orgs[i].mTally));
		}
		read(fd, &orgs[i].score, sizeof(int));
		read(fd, &orgs[i].age, sizeof(int));
		read(fd, &orgs[i].myTape, tapeSize);
		read(fd, &orgs[i].whichTape, sizeof(int));
		read(fd, &orgs[i].headIndex, sizeof(int));
		read(fd, &orgs[i].newStep, 1);
		read(fd, &orgs[i].gotPoint, 1);
		read(fd, &orgs[i].myRoom, sizeof(int));
		read(fd, &orgs[i].dir, sizeof(int));
		read(fd, &orgs[i].moveDir, sizeof(int));
		int dif;
		read(fd, &dif, sizeof(int));
		orgs[i].myLoc = rooms[orgs[i].myRoom].occupants + dif;
		*orgs[i].myLoc = orgs+i;
		rooms[orgs[i].myRoom].numOrganisms++;
	}
	mapReadEverythingFromFile(fd, reset);
	close(fd);
	return 0;
}

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
	tallyReproduction(from, to);
}

static void icmHelper(organism *who, unsigned char (*mat)[16], int N, int base, int target) {
	neuron *ns = who->neurons;
	int nns = who->numNeurons;
	int type;
	int con;
	int n = N - 1;
	for (; n >= 0; n--) {
		for (type = 0; type < 3; type++) {
			for (con = ns[n].nums[type] - 1; con >= 0; con--) {
				if (target == ns[n].sets[type][con]) {
					mat[N*nns + n][4*base + type]++;
					mat[n*nns + N][4*type + base]++;
				}
			}
		}
		if (target == ns[n].value) {
			mat[N*nns + n][4*base + 3]++;
			mat[n*nns + N][12 + base]++;
		}
	}
}

static void initConnectionMatrix(organism* who, unsigned char (*mat)[16]) {
	neuron *ns = who->neurons;
	int i = who->numNeurons - 1;
	int type, con;
	for (; i > 0; i--) {
		for (type = 0; type < 3; type++) {
			for (con = ns[i].nums[type] - 1; con >= 0; con--) {
				icmHelper(who, mat, i, type, ns[i].sets[type][con]);
			}
		}
		icmHelper(who, mat, i, 3, ns[i].value);
	}
}

static int tallyGraphSimilarities(unsigned char *p1, unsigned char *p2) {
	int ret = 0;
	int i = 15;
	int tmp;
	for (; i >= 0; i--) {
		tmp = (p1[i] > p2[i]) ? p2[i] : p1[i];
		if (tmp) ret += 1 +tmp;
	}
	return ret;
}

static void mutateGraphSimilarities(organism *who) {
	int nns = who->numNeurons;
	if (nns < 2) return;
	int pointA = random()%nns;
	int pointB = random()%(nns-1);
	if (pointB >= pointA) pointB++;
	unsigned char (*mat)[16] = calloc(nns*nns, 16);
	initConnectionMatrix(who, mat);
	int *mapping1 = malloc(nns * sizeof(int));
	int *mapping2 = malloc(nns * sizeof(int));
	int *unmapped1 = malloc(nns * sizeof(int));
	int *unmapped2 = malloc(nns * sizeof(int));
	double *strengths = calloc(nns, sizeof(double));
	double *scores = malloc(sizeof(double) * nns * nns);
	double *bestStrs = malloc(sizeof(double) * nns * nns);
	int i = nns - 1;
	int numMapped = 1;
	int numUnmapped = i;
	for (; i >= 0; i--) {
		unmapped1[i] = i;
		unmapped2[i] = i;
	}
	mapping1[0] = pointA;
	mapping2[0] = pointB;
	strengths[0] = 1;
	unmapped1[pointA] = unmapped2[pointB] = nns-1;

	int j, k;
	int prop1, prop2; // Proposed mapping
	while (numUnmapped) {
		for (i = numUnmapped - 1; i >= 0; i--) {
			prop1 = unmapped1[i];
			for (j = numUnmapped - 1; j >= 0; j--) {
				double *score = scores + i*nns + j;
				double *bestStr = bestStrs + i*nns + j;
				*score = 0;
				*bestStr = 0;
				prop2 = unmapped2[j];
				for (k = numMapped - 1; k >= 0; k--) {
					int sim = tallyGraphSimilarities(mat[nns*mapping1[k] + prop1], mat[nns*mapping2[k] + prop2]);
					*score += strengths[k] * sim;
					double str = strengths[k] * sim / (sim + 1);
					if (str > *bestStr) *bestStr = str;
				}
			}
		}
		double bestScore = -1;
		for (i = numUnmapped - 1; i >= 0; i--) {
			double *score = scores + i*nns;
			for (j = numUnmapped - 1; j >= 0; j--) {
				if (score[j] > bestScore) {
					bestScore = score[j];
					prop1 = i;
					prop2 = j;
				}
			}
		}
		i = unmapped1[prop1];
		j = unmapped2[prop2];
		unsigned char *cons1 = mat[pointA*nns + i];
		unsigned char *cons2 = mat[pointB*nns + j];
		int types[16];
		int numTypes = 0;
		for (k = 0; k < 16; k++) {
			if (cons1[k] > cons2[k])
				types[numTypes++] = k;
		}
		if (numTypes) {
			int type = types[random()%numTypes];
			int node;
			if ((type / 4) == 3 || (type & 3) == 3) {
				if ((type & 3) == 3) {
					node = who->neurons[j].value;
					if ((type / 4) == 3) who->neurons[pointB].value = node;
				} else {
					node = who->neurons[pointB].value;
				}
			} else {
				char *used = calloc(mem, 1);
				int numUnused = mem;
				int type2 = type & 3;
				i = who->neurons[j].nums[type2];
				numUnused -= i;
				for (i--; i >= 0; i--)
					used[who->neurons[j].sets[type2][i]] = 1;
				type2 = type / 4;
				for (i = who->neurons[pointB].nums[type2] - 1; i >= 0; i--) {
					k = who->neurons[pointB].sets[type2][i];
					if (!used[k]) {
						used[k] = 1;
						numUnused--;
					}
				}
				if (numUnused == 0) {
					node = -1;
				} else {
					k = random()%numUnused;
					for (node = 0; k >= 0; node++) {
						if (!used[node]) k--;
					}
					node--;
				}
				free(used);
			}
			if (node != -1) {
				int type2 = type & 3;
				if (type2 != 3)
					who->neurons[j].sets[type2][who->neurons[j].nums[type2]++] = node;
				type2 = type / 4;
				if (type2 != 3)
					who->neurons[pointB].sets[type2][who->neurons[pointB].nums[type2]++] = node;
				tallyMutation(who, 7);
			}
			numUnmapped = 1;
		}
		numUnmapped--;
		unmapped1[prop1] = unmapped1[numUnmapped];
		unmapped2[prop2] = unmapped2[numUnmapped];
		mapping1[numMapped] = i;
		mapping2[numMapped] = j;
		strengths[numMapped] = bestStrs[prop1*nns + prop2];
		numMapped++;
	}

	free(mat);
	free(mapping1);
	free(mapping2);
	free(unmapped1);
	free(unmapped2);
	free(strengths);
	free(scores);
	free(bestStrs);
}

static void mutateAddNeuron(organism* who){ // Copy from an existing neuron
	who->neurons = realloc(who->neurons, sizeof(neuron)*(++who->numNeurons));
	neuron* tmp = who->neurons+who->numNeurons-1;
	if(who->numNeurons-1){
		int i = random()%(who->numNeurons-1);
		memcpy(tmp, who->neurons+i, sizeof(neuron));
		/*i = random()%mem;
		int j, set=0;
		for(set = 0; set < 3; set++){
			for(j=0; j < tmp->nums[set]; j++){
				tmp->sets[set][j] = (tmp->sets[set][j]+i)%mem;
			}
		}
		tmp->value = (tmp->value+i)%mem;*/
	}else{
		tmp->nums[0] = tmp->nums[1] = tmp->nums[2] = 0;
		tmp->value = random()%mem;
	}
}

static void mutateAddConnection(neuron *n, int type, int candidate)
{
	int* tmp = n->sets[type];
	int i = n->nums[type]-1;
	for(; i>=0; i--){
		if(tmp[i] == candidate) return; // Potential spot for improvement - should I keep the memory locations sorted?
	}
	tmp[n->nums[type]++] = candidate;
}

static char mutateChangeNeuron(organism* who, neuron* n){
	int type = random()%8;
	if(type >= 7){ // Then ur dead lol
		memcpy(n, who->neurons+(--who->numNeurons), sizeof(neuron));
		who->neurons = realloc(who->neurons, sizeof(neuron)*who->numNeurons);
		tallyMutation(who, 2);
		return 0;
	}else if(type/3 == 0){
		if(n->nums[type]){
			n->nums[type]--;
			n->sets[type][random()%(1+n->nums[type])] = n->sets[type][n->nums[type]];
			tallyMutation(who, 3);
		}
	}else if(type/3 == 1){
		int candidate = random() % mem;
		type -= 3;
		mutateAddConnection(n, type, candidate);
		tallyMutation(who, 4);
		if (random() % 2) { // Then we're going to connect candidate to another neuron too, thereby forming a connection
			if (type == 2)
				type = random() % 2;
			else
				type = 2;
			mutateAddConnection(who->neurons + random() % who->numNeurons, type, candidate);
			tallyMutation(who, 5);
		}
	} else {
		n->value = random()%mem;
		tallyMutation(who, 6);
	}
	return 1;
}

#define swapMacro(a, b, c) \
if (a == b) {\
	if (random() < RAND_MAX/2)\
		a = c;\
} else if (a == c) {\
	if (random() < RAND_MAX/2)\
		a = b;\
}
static void mutateSwapCells(organism *who) {
	int c1 = random()%mem;
	int c2 = random()%mem;
	int i = who->numNeurons - 1;
	int j, k;
	neuron *n;
	for (; i >= 0; i--) {
		n = who->neurons + i;
		for (j = 0; j < 3; j++) {
			for (k = n->nums[j] - 1; k >= 0; k--) {
				swapMacro(n->sets[j][k], c1, c2);
			}
		}
		swapMacro(n->value, c1, c2);
	}
}

static void mutateOrganism(organism* who){
	while (random() < RAND_MAX/25) {
		mutateAddNeuron(who);
		tallyMutation(who, 0);
	}
	if (random() < RAND_MAX/6) {
		mutateSwapCells(who);
		tallyMutation(who, 1);
	}
	if (random() < RAND_MAX/10) {
		mutateGraphSimilarities(who);
	}
	//Start tallying non-mutations, affect reading/writing, also mutation display
	int i = 0;
	while (i < who->numNeurons) {
		if(random() < RAND_MAX/25) {
			if(mutateChangeNeuron(who, who->neurons+i)) i++;
		} else {
			i++;
		}
	}
	static const unsigned char zs[mTypes-1] = {0};
	if (!memcmp(who->mTally[0], zs, mTypes - 1))
		tallyMutation(who, mTypes-1);
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

static void simulate(int argc, char** argv){
#ifdef CONTINENTS
	initSubPops();
#endif
	initMap();

	organism *allMyChildren = malloc(sizeof(organism)*numKids);
	char reset = 0;
	for (argc--; argc > 0; argc--) {
		if (0 == strcmp(argv[argc], "reset")) {
			puts("reset!");
			reset = 1;
		}
	}
	if (readEverythingFromFile(allMyChildren, "save.dat", reset)) {
#define numSources 5
		organism *sources = malloc(sizeof(organism) * numSources);
		char name[14];
		int i;
		for (i=0; i < numSources; i++) {
			sprintf(name, "source%d.org", i+1);
			importOrganism(sources + i, name);
		}

		for(i = 0; i < numKids; i++){
			copyOrganism(allMyChildren+i, sources + i%numSources);
			spawnGuy(allMyChildren+i);
		};
		for (i = 0; i < numSources; i++)
			free(sources[i].neurons);
		free(sources);
	}

	organism *tmp, *end = allMyChildren+numKids;
	neuron* holdThis;

	int i, j=0;
	while(1){
		doStepMap();
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
		/*
		printMap();
		fputs("\n", stdout);
		fgetc(stdin);
		*/
		if(++j%400 == 0){
			if (j == 2000){
				printTallies();
				printMap();
				fputs("\n", stdout);
			}
			changeMap();
			if (j == 2000) {
				j = 0;
				writeEverythingToFile(allMyChildren, "save.tmp");
				system("mv save.tmp save.dat");
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

static void compare() {
	organism alex, adam;
	importOrganism(&alex, "best.org");
	importOrganism(&adam, "simpler.org");
	char* memory = alex.memory;
	char* memory2 = adam.memory;
	while(1) {
		prepOrganismMemory(&alex);
		prepOrganismMemory(&adam);

		int i = 1;
		for (; i <= 5; i++)
			memory[i] = memory2[i] = random()%2;

		tickOrganism(&alex);
		tickOrganism(&adam);

		memory = alex.memory+alex.memStart;
		memory2 = adam.memory+adam.memStart;

		for (i = 10; i <= 15; i++)
			if (memory[i] != memory2[i])
				puts("Dr. Octagonapus BLAH");
		if (memory[4] != memory2[4])
			puts("AAAAAAAA");
	}
	free(alex.neurons);
	free(adam.neurons);
}

static void converse(){
	organism alex;
	importOrganism(&alex, "best.org");
	char input[20];
	char* memory = alex.memory;
	while(fgets(input, 20, stdin)){
		prepOrganismMemory(&alex);

		int len = strlen(input) - 2; // Ignore newline and null byte
		for (; len >= 0; len--) {
			memory[len] = (input[len] == 'x');
		}

		tickOrganism(&alex);

		memory = alex.memory+alex.memStart;
		printf("%d\n", memory[2]);
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
		doStepMap();
		for(i=0; i<2; i++){
			for(tmp=allMyChildren; tmp<end; tmp++){
				prepOrganismMemory(tmp);
				doInputGuy(tmp);
				printf("Guy %ld\n", tmp-allMyChildren);

				if(tmp->memory[1]) puts("Door ahead");
				if(tmp->memory[2]) puts("Got a point!");
				if(tmp->memory[3]) puts("A brand new step!");
				if(tmp->memory[4]) puts("Our own");
				printf("Tape reads: %d\n", tmp->memory[5]);
				if(NULL == fgets(input, 10, stdin)) return; // No cleanup, because we're monsters.
				int i = 0;
				for(; i < 6; i++){
					if(input[i]>='a'&&input[i]<='z') tmp->memory[10+i] = 1;
				}
				if(input[6]!='\n') tmp->memory[5] = input[6]>='a'&&input[6]<='z';
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
		printMap();
		fputs("\n\n\n", stdout);
		if(++j%200 == 0){
			if(j == 2000){
				j = 0;
			}
			changeMap();
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
	simulate(argc, argv);
	//converse();
	//navigate();
	//compare();
	return 0;
}
