#pragma once


#include "Node.h"
#include <Windows.h>
#pragma comment(lib, "winmm.lib") // Needed for the timeGetTime()-function.

static const char UTILITY_EXTRA_TURN_CONSTANT = 6; 
static const int UTILITY_BEST_OPPONENT = SCHAR_MAX;
static const int UTILITY_BEST_PLAYER = SCHAR_MIN;

class Minimax
{
	private:
		DWORD startTime;
		short int timeLimitMS;

	private:
		char Evaluation(const Board const* board, bool minTurn);
		char UtilityFunction(const Board const* board);

	public:
		Minimax();
		virtual~Minimax();

		DWORD GetStartTime() { return this->startTime; }
		void SetTimeLimit(short int timeLimit) { this->timeLimitMS = timeLimit; }
		/*
			Sets the start time used for the Generate(...)-function.
		*/
		void SetStartTime() { this->startTime = timeGetTime(); }
		/* 
			OBS! This function relies on the SetStartTime()-function, so be sure to appropriately call it before this function!
			
			Generates a game tree from current game state and calculates or evaluates a utility value of created 
			game states in nodes that are leaf-nodes and propagates this utility value up the tree of nodes.
			
			Parameters:
			currentNode = Node holding the current state of the game.
			maxDepth = The (maximum) depth to traverse down to. (currentNode is on level 0).
			minTurn = Set to true if it is min's turn.
			time = The time spent in this function. When time becomes greater than the set time limit, the expansion of the tree stops.
			alpha = The minimum utility value max (us) is assured of. Initial value set to lowest possible for data type.
			beta = The maximum utility value min (opponent) is assured of. Initial value set to highest possible for data type.
			
			returns the best propagated utility value of child nodes.
		*/		
		char Generate(Node* currentNode, unsigned char maxDepth, bool minTurn, unsigned short int time = 0, char alpha = SCHAR_MIN, char beta = SCHAR_MAX);
		/*
			De-allocates the memory of currentNode and its children using depth-first search.
		*/
		void DeAllocate(Node* currentNode);
};

