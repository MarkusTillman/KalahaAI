#include "Minimax.h"

#include "Node.h"
#include "Board.h"

#include <limits.h>

using namespace std;


char Minimax::Evaluation(const Board const* board, bool minTurn)
{
	// Difference in max & min's current scores.
	char utilityDiff = board->GetNrOfSeedsInKalah(0) - board->GetNrOfSeedsInKalah(1);  
	
	// Check if current player can steal the opponent's seeds and...
	char nrOfStolenSeeds = board->CanGetOpponentSeeds(minTurn);
	if(nrOfStolenSeeds > 0)
	{
		utilityDiff += nrOfStolenSeeds;
	}
	else
	{
		// ... only check if current player can get more turns, this turn, if not.
		utilityDiff += UTILITY_EXTRA_TURN_CONSTANT * board->CanGetExtraTurn(minTurn);
	}

	return utilityDiff;
}

char Minimax::UtilityFunction(const Board const* board)
{
	// Check who won.
	if(board->GetNrOfSeedsInKalah(0) > board->GetNrOfSeedsInKalah(1))
	{
		// Max won, return max's score.
		return board->GetNrOfSeedsInKalah(0);
	}

	// Min won, return min's (negative) score.
	return -board->GetNrOfSeedsInKalah(1);
}
void Minimax::DeAllocate(Node* currentNode)
{
	// Delete the nodes recursively using depth-first search.
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		if(currentNode->children[i])
		{
			DeAllocate(currentNode->children[i]);
		}
	}
	delete currentNode;	
	currentNode = nullptr;
}

Minimax::Minimax()
{
	this->startTime = 0;
	this->timeLimitMS = 0;
}
Minimax::~Minimax()
{
	
}

char Minimax::Generate(Node* currentNode, unsigned char maxDepth, bool minTurn, unsigned short int time, char alpha, char beta)
{
	if(maxDepth == 0 || time > this->timeLimitMS)
	{
		if(currentNode->board->IsTerminalState() != -1)
		{
			return this->UtilityFunction(currentNode->board);
		}

		return this->Evaluation(currentNode->board, minTurn);
	}


	// Expand the tree if maximum depth has not yet been reached.
	unsigned char nrOfSeeds[AMBO_PLAYER_COUNT];
	Board childBoard;
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		childBoard = *currentNode->board; // Reset/copy board.
		Node* newNode = nullptr;
		nrOfSeeds[i] = childBoard.GetNrOfSeeds(i, minTurn); 
		bool possibleMove = childBoard.MoveSeeds(i, minTurn);
		if(possibleMove) 
		{
			newNode = new Node(childBoard);
		}
		currentNode->children[i] = newNode; // Always set the child pointers. null pointers are handled.
	}

	if(currentNode->IsLeaf())
	{
		return this->UtilityFunction(currentNode->board);
	}


	// If node is not a terminal node, propagate the utility value (highest/lowest depending on whose turn it is)
	// of children to parent node up to root node.
	char utilityValue = 0;
	unsigned short int timeElapsed = 0;
	
	if(minTurn)
	{
		currentNode->utility = UTILITY_BEST_OPPONENT;
		for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
		{
			minTurn = true; 
			if(currentNode->children[i])
			{
				// Check if last seed was put in the kalah.
				// i = start.
				// nrOfSeeds = steps to increment (end).
				// playerIndex * AMBO_PLAYER_COUNT + playerIndex = player-offset.
				unsigned char lastSeedIndex = (i + nrOfSeeds[i] + 1 * AMBO_PLAYER_COUNT + 1) % AMBO_COUNT;

				// The opponents kalah shall be stepped over, so check how many times we iterate over it.
				// i = start index.
				// i + nrOfSeeds = end index.
				// Range [0, AMBO_COUNT-1] of (i + nrOfSeeds), therefore to get correct number of iterations,
				// subtract 1 from AMBO_COUNT and do integer division.
				lastSeedIndex += (i + nrOfSeeds[i]) / (AMBO_COUNT - 1);
				if(lastSeedIndex == AMBO_COUNT - 1)
				{
					minTurn = false;
				}
				
				// Send child node, reduce the depth-meter by one and change whose turn it is as parameters to this function.
				timeElapsed = (unsigned short int)(timeGetTime() - this->startTime);
				utilityValue = Generate(currentNode->children[i], maxDepth - 1, !minTurn, timeElapsed, alpha, beta);
				beta = min(beta, utilityValue);
				// If alpha is greater (or equal) to beta, it means we've found a branch that is worse.
				if(beta <= alpha)
				{
					break;
				}
			}
		}
		currentNode->utility = beta;
		return currentNode->utility; 
	}
	else
	{
		currentNode->utility = UTILITY_BEST_PLAYER; 
		for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
		{
			minTurn = false;
			if(currentNode->children[i])
			{
				// Check if last seed was put in the kalah.
				// i = start.
				// nrOfSeeds = steps to increment (end).
				// playerIndex * AMBO_PLAYER_COUNT + playerIndex = player-offset.
				unsigned char lastSeedIndex = (i + nrOfSeeds[i]) % AMBO_COUNT;

				// The opponents kalah shall be stepped over, so check how many times we iterate over it.
				// i = start index.
				// i + nrOfSeeds = end index.
				// Range [0, AMBO_COUNT-1] of (i + nrOfSeeds), therefore to get correct number of iterations,
				// subtract 1 from AMBO_COUNT and do integer division.
				lastSeedIndex += (i + nrOfSeeds[i]) / (AMBO_COUNT - 1);
				if(lastSeedIndex == AMBO_PLAYER_COUNT)
				{
					minTurn = true;
				}

				// Send child node, reduce the depth-meter by one and change whose turn it is as parameters to this function.
				timeElapsed = (unsigned short int)(timeGetTime() - this->startTime); 
				utilityValue = Generate(currentNode->children[i], maxDepth - 1, !minTurn, timeElapsed, alpha, beta);
				alpha = max(alpha, utilityValue);
				// If alpha is greater (or equal) to beta, it means we've found a branch that is worse.
				if(beta <= alpha)
				{
					break; 
				}
			}
		}
		currentNode->utility = alpha;
		return currentNode->utility;
	}
}