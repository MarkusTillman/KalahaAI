#pragma once

#include <string> 
using namespace std; 


static const char AMBO_SEED_COUNT = 6; // The number of (start-)seeds in an ambo.
static const char AMBO_PLAYER_COUNT = 6; // The number of ambos per player, excluding the kalah.
static const char AMBO_COUNT = (AMBO_PLAYER_COUNT + 1) * 2; // The total number of ambos, including the kalahs.

class Board
{
	private:
		unsigned char ambos[AMBO_COUNT]; // Contains the number of seeds in each ambo/house/store/Kalah. Range[0,AMBO_PLAYER_COUNT] = player 1. Range[AMBO_PLAYER_COUNT + 1,AMBO_COUNT-1] = player 2.

	private:
		void SetNrOfSeeds(unsigned char amboIndex, unsigned char playerIndex, unsigned char nrOfSeeds) throw(...);

	public:
		Board();
		Board(const Board& copy);
		Board(unsigned char ambos[AMBO_COUNT]);
		virtual~Board();

		/*
			Swaps the sides of the board with each other.
		*/
		void Swap();
		/*	
			Returns the number of seeds in a given ambo. 
			Throws an exception if one of the parameter values are out of bounds.	
		*/
		unsigned char GetNrOfSeeds(unsigned char amboIndex, unsigned char playerIndex) const throw(...);
		unsigned char GetNrOfSeedsInKalah(unsigned char playerIndex) const;
		/*
			Checks if current board state is a terminal state.
			Returns 0 if only max has seeds left.
			Returns 1 if only min has seeds left.
			Returns -1 if current board state is NOT a terminal state.
		*/
		char IsTerminalState() const;
		/*	
			Moves seeds from the given ambo and increments the seed count in the following ambos (and player-owned kalah).
			Returns false if the selected ambo is empty. */
		bool MoveSeeds(unsigned char amboIndex, unsigned char playerIndex);
		/*
			Check whether or not an extra turn can be gained by moving seeds so that the last seed lands in the player's kalah.
			Returns 1 if max (us) or -1 if min (opponent) can gain an extra turn.
			Returns 0 if no extra turn can be gained.
		*/
		char CanGetExtraTurn(unsigned char playerIndex) const;
		/*
			Checks whether or not seeds can be stolen.
			Returns the maximum number of seeds (+ the last seed placed) that can be stolen. 
			Note that this number is negative if min (the opponent) can steal from max (us).
		*/
		char CanGetOpponentSeeds(unsigned char playerIndex) const;
		/*
			Returns a string representing/visualizing the board. 
		*/
		string ToString() const;
};
