#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "globals.h"
#include "organism.h"
#include "main.h"

typedef struct room{
	char typeMask;
	char buttonType;
	struct room* butonDest;
	organism *(occupants[numKids]);
	char roomTape[tapeSize];
	int numOrganisms;
	char doorRight, doorDown;
}room;

static room* rooms;

char hasDoor(int r, int dir){
	switch(dir){
		case 0:
			return rooms[r].doorRight;
		case 1:
			return rooms[r].doorDown;
		case 2:
			if(r%mapSize == 0) return 0;
			return rooms[r-1].doorRight;
		default:
			if(r<mapSize) return 0;
			return rooms[r-mapSize].doorDown;

	}
}

int dirs[4] = {1, mapSize, -1, -mapSize};

void printMap(){
	char* letters = "@ABCDEFGHIJKLMNOPQRSTUVWXY";
	char* numbers = "+12345678";
	int i, j;
	for(i=0; i < mapSize; i++){
		for(j=0; j<mapSize; j++){
			int num = rooms[i*mapSize+j].numOrganisms;
			if(rooms[i*mapSize+j].roomTape[0]){
				fputc(num>=26?'Z':letters[num], stdout);
			}else{
				fputc(num>=9?'9':numbers[num], stdout);
			}
			fputc(hasDoor(i*mapSize+j, 0)?'-':' ', stdout);
		}
		fputc('\n', stdout);
		for(j=0; j<mapSize; j++){
			fputc(hasDoor(i*mapSize+j, 1)?'|':' ', stdout);
			fputc(' ', stdout);
		}
		fputc('\n', stdout);
	}
}

int mapVersion = 0;

#define numRooms (mapSize*mapSize)

static char connects(int r, int d){
	if(d){
		rooms[r].doorDown = 0;
	}else{
		rooms[r].doorRight = 0;
	}
	int R = r;
	int D = d+1;
	int dest = R+dirs[d];
	do{
		if(hasDoor(R, D)){
			R += dirs[D];
			D=(D+3)%4;
		}else D=(D+1)%4;
	}while((r!=R || D!=d) && R!=dest);
	if(R==dest) return 1;
	if(d)
		rooms[r].doorDown=1;
	else
		rooms[r].doorRight=1;
	return 0;
}

void changeMap(){
	mapVersion++;
	int i = 0;
	for(; i < numRooms; i++) rooms[i].roomTape[0] = 0;
	for(i=0; i<numMines; i++){
		int where;
		do{
			where = random()%numRooms;
		}while(rooms[where].roomTape[0]);
		rooms[where].roomTape[0] = 1;
	}
	while(1){
		int r = random()%numRooms;
		int d = random()%2;
		if((d && r>=numRooms-mapSize) || (!d && (r+1)%mapSize == 0)) continue;
		if(hasDoor(r, d)){
			if(random()%2) continue; // So we slightly favor adding doors, to keep the balance.
			if(connects(r, d)) return;
		}else{
			if(d){
				rooms[r].doorDown = 1;
			}else{
				rooms[r].doorRight = 1;
			}
			return;
		}
	}
}

void initMap(){
	bestScore = 0;
	//mapVersion++;
	rooms = malloc(sizeof(room)*mapSize*mapSize);
	int i = 0;
	for(; i < numRooms; i++){
		rooms[i].typeMask = 0;
		rooms[i].buttonType = 0;
		rooms[i].numOrganisms = 0;
		rooms[i].doorRight = rooms[i].doorDown = 1;
		memset(rooms[i].roomTape, 0, tapeSize);
	}
	for(i=mapSize-1; i < numRooms; i+=mapSize) rooms[i].doorRight = 0;
	for(i=numRooms-mapSize; i<numRooms; i++) rooms[i].doorDown = 0;
	int numWalls = 0;
	while(numWalls < numRooms/2){
		int r = random()%numRooms;
		int d = random()%2;
		if(!hasDoor(r, d)) continue;
		if(connects(r, d))
			numWalls++;
	}
	changeMap();
}

static void addGuy(organism* who){
	room* r = rooms+who->myRoom;
	who->myLoc = r->occupants + (r->numOrganisms++);
	*who->myLoc = who;
}

static void removeGuy(organism* who){
	organism** loc = who->myLoc;
	room* r = rooms+who->myRoom;
	*loc = r->occupants[--r->numOrganisms];
	(*loc)->myLoc = loc;
}

static void newStepGuy(organism* who){
	who->newStep = 1;
	who->whichTape = who->headIndex = 0;
	who->moveDir = -1;
}

void spawnGuy(organism* who){
	who->myRoom = 0;
	who->dir = 0;
	who->gotPoint = 0;
	addGuy(who);

	newStepGuy(who);
}

void doInputGuy(organism* who){
	char* memory = who->memory+who->memStart;
	memory[1]=hasDoor(who->myRoom, who->dir);
	room* r = rooms + who->myRoom;
	if(who->gotPoint){
		memory[2] = 1;
		who->gotPoint = 0;
	}
	if(who->newStep){
		memory[3] = 1;
		who->newStep = 0;
	}
	if(who->whichTape > r->numOrganisms) who->whichTape -= r->numOrganisms+1;
	else if(who->whichTape < 0) who->whichTape += r->numOrganisms+1;
	if(who->whichTape == 0){
		memory[4] = 1;
		memory[5] = who->myTape[who->headIndex];
	}else if(who->whichTape == 1){
		memory[5] = r->roomTape[who->headIndex];
	}else{
		int i = who->myLoc - r->occupants + who->whichTape - 1;
		if(i >= r->numOrganisms) i -= r->numOrganisms;
		memory[5] = r->occupants[i]->myTape[who->headIndex];
	}
}

static void givePointGuy(organism* who){
	who->gotPoint = 1;
	who->score++;
	who->age -= 15;
	if(who->score > bestScore){
		bestScore = who->score;
		char string[15];
		sprintf(string, "best%d.org", mapVersion);
		exportOrganism(who, string);
	}
}

char doStepGuy(organism* who){
	int d = who->moveDir;
	if(d!=-1 && hasDoor(who->myRoom, d)){
		removeGuy(who);
		who->myRoom += dirs[d];
		addGuy(who);
	}

	newStepGuy(who);

	who->age+=10;

	if(who->age >= 2000){
		removeGuy(who);
		return 1;
	}
	return 0;
}

void doStepMap(){
	int i = 0;
	for(; i < numRooms; i++){
		if(rooms[i].roomTape[0]){
			int num = 1+abs(3-rooms[i].numOrganisms);
			int j = 0;
			for(; j < rooms[i].numOrganisms; j++){
				if(0 == random()%num) givePointGuy(rooms[i].occupants[j]);
			}
		}
	}
}

void doOutputGuy(organism* who){
	char* memory = who->memory+who->memStart;
	if(memory[10]!=memory[11]) who->dir = (who->dir+(memory[10]?3:1))%4;

	if(memory[12]) who->moveDir = who->dir;
	if(memory[4]){ // Only set by default if we're looking at our own tape
		who->myTape[who->headIndex] = memory[5];
	}
	if(memory[13] || memory[15]){
		who->headIndex = 0;
		if(memory[13] != memory[15]){
			if(memory[13])
				who->whichTape++;
			else
				who->whichTape--;
		}
	}
	if(memory[14]) who->headIndex = (who->headIndex+1)%tapeSize;
}

void quitMap(){
	free(rooms);
}
