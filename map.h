
typedef struct room{
	char typeMask;
	organism *(occupants[numKids]);
	char roomTape[tapeSize];
	int numOrganisms;
	char doorRight, doorDown;
}room;

room* rooms;

extern void printMap();
extern void initMap();
extern void quitMap();
extern void changeMap();
extern void spawnGuy(organism *who);
extern void doInputGuy(organism* who);
extern void doOutputGuy(organism* who);
extern char doStepGuy(organism* who);
extern void doStepMap();

extern void mapWriteEverythingToFile(int fd);
extern void mapReadEverythingFromFile(int fd);
