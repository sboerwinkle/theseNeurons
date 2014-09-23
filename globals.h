
#define mem 256

#define kidsLog2 8
#define numKids 256

#define CONTINENTS

#define mapSize 20
#define numMines 32

typedef struct{
	int sets[3][mem];
	int nums[3];
	int value;
	char used;
} neuron;

struct room;

#define tapeSize 8
typedef struct organism{
	char memory[mem*2];
	int memStart;
	neuron *neurons;
	int numNeurons;
	int score, age;

	char myTape[tapeSize];
	int whichTape, headIndex;
	char newStep, gotPoint;

	int myRoom;
	int dir;
	int moveDir;
	struct organism** myLoc;
} organism; // So much rides on this word. How many days, weeks, months... years... later will I be reading this line?
