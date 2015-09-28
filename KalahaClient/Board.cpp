#include "Board.h"

#include <sstream>
#include <algorithm>

void Board::SetNrOfSeeds(unsigned char amboIndex, unsigned char playerIndex, unsigned char nrOfSeeds) throw(...)
{
	if(amboIndex > AMBO_PLAYER_COUNT)
	{
		throw("First parameter out of bounds, valid values are 0 to 6.");
	}
	else if(playerIndex > 1)
	{
		throw("Second parameter out of bounds, valid values are 0 and 1.");
	}
	// amboIndex = where to start.
	// player index * the number of ambos (per player) + the number of kalah (1) = player offset.
	this->ambos[playerIndex * AMBO_PLAYER_COUNT + playerIndex + amboIndex] = nrOfSeeds;
}

Board::Board()
{
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		this->ambos[i] = AMBO_SEED_COUNT;
	}
	this->ambos[AMBO_PLAYER_COUNT] = 0;
	for(unsigned char i = AMBO_PLAYER_COUNT + 1; i < AMBO_COUNT - 1; i++)
	{
		this->ambos[i] = AMBO_SEED_COUNT;
	}
	this->ambos[AMBO_COUNT - 1] = 0;
}
Board::Board(const Board& copy)
{
	for(unsigned char i = 0; i < AMBO_COUNT; i++)
	{
		this->ambos[i] = copy.ambos[i];
	}
}
Board::Board(unsigned char ambos[AMBO_COUNT])
{
	for(unsigned char i = 0; i < AMBO_COUNT; i++)
	{
		this->ambos[i] = ambos[i];
	}
}
Board::~Board()
{

}
void Board::Swap()
{
	for(unsigned char i = 0; i < AMBO_COUNT / 2; i++)
	{
		unsigned char tmpChar = this->ambos[i];
		unsigned char index = AMBO_PLAYER_COUNT + 1 + i;
		this->ambos[i] = this->ambos[index];
		this->ambos[index] = tmpChar;
	}
}
unsigned char Board::GetNrOfSeeds(unsigned char amboIndex, unsigned char playerIndex) const throw(...)
{
	if(amboIndex > AMBO_PLAYER_COUNT)
	{
		throw("First parameter out of bounds, valid values are 0 to 6.");
	}
	else if(playerIndex > 1)
	{
		throw("Second parameter out of bounds, valid values are 0 and 1.");
	}
	// amboIndex = where to start.
	// player index * the number of ambos (per player) + the number of kalah (1) = player offset.
	return this->ambos[amboIndex + playerIndex * AMBO_PLAYER_COUNT + playerIndex];
}

unsigned char Board::GetNrOfSeedsInKalah(unsigned char playerIndex) const
{
	return this->GetNrOfSeeds(AMBO_PLAYER_COUNT, playerIndex);
}

char Board::IsTerminalState() const
{
	// First check if Max has any seeds left.
	bool maxHasNoSeeds = true;
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		if(this->GetNrOfSeeds(i, 0) > 0)
		{
			maxHasNoSeeds = false;
			break;
		}
	}
	if(maxHasNoSeeds) // If Max has no seeds...
	{
		return 1; // ...return 1.
	}
	else // Else check if Min has any seeds left and...
	{
		for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
		{
			if(this->GetNrOfSeeds(i, 1) > 0)
			{
				return -1; // ...return -1 if Min (both!) has seeds.
			}
		}
	}

	return 0; // Else return 0 (only Max has seeds left).
}
bool Board::MoveSeeds(unsigned char amboIndex, unsigned char playerIndex)
{
	// Save the number of seeds in the selected ambo.
	unsigned char nrOfSeeds = GetNrOfSeeds(amboIndex, playerIndex);
	if(nrOfSeeds > 0)
	{
		// Empty the selected ambo
		this->SetNrOfSeeds(amboIndex, playerIndex, 0); 
		unsigned char index = 0;
		unsigned char indexOfOpponentAmbo = AMBO_PLAYER_COUNT + !playerIndex * AMBO_PLAYER_COUNT + !playerIndex;

		// Increment the seed count in the following ambos.
		for(unsigned char i = 1; i <= nrOfSeeds; i++)
		{
			index = (playerIndex * AMBO_PLAYER_COUNT + playerIndex + amboIndex + i) % AMBO_COUNT;
			// Special case: Skip opponents Kalah.
			if(index != indexOfOpponentAmbo)
			{
				this->ambos[index]++;
			}
			else // Since we skipped the opponents kalah, we need to take one extra step.
			{
				nrOfSeeds++; 
			}
		}

		// Check if the last seed lands in an empty, owned ambo.
		unsigned char mirrorIndex = 0;
		// 'index' is now the index of the ambo where the last seed landed.
		// Range [0, AMBO_PLAYER_COUNT - 1] of index				= max (Max, 0).
		// Range [AMBO_PLAYER_COUNT + 1, AMBO_COUNT - 1] of index	= min (Min, 1).
		nrOfSeeds = this->ambos[index];
		if(nrOfSeeds == 1) // If only the last seed is in the ambo...
		{
			// ... check mirror (opponent) ambo.
			nrOfSeeds = 0; // Reset nrOfSeeds.
			if(index > AMBO_PLAYER_COUNT && index < AMBO_COUNT - 1 && playerIndex == 1) 
			{
				// AMBO_COUNT - 1 = index-end of min's (opponent) row of ambos.
				// -index = the number of steps to make.
				// Note that the last '-1' is the kalah.
				mirrorIndex = AMBO_COUNT - 1 - index - 1; // Range [AMBO_PLAYER_COUNT + 1, AMBO_COUNT - 1].
				nrOfSeeds = this->ambos[mirrorIndex];
			}
			else if(index < AMBO_PLAYER_COUNT && playerIndex == 0)
			{
				// AMBO_PLAYER_COUNT = max's (us) kalah.
				// AMBO_PLAYER_COUNT - index = the number of steps to go from the kalah.
				mirrorIndex = AMBO_PLAYER_COUNT + (AMBO_PLAYER_COUNT - index); // Range [0, AMBO_PLAYER_COUNT - 1].
				nrOfSeeds = this->ambos[mirrorIndex];
			}
			// If mirror ambo is not empty...
			if(nrOfSeeds > 0) 
			{
				// ... then steal the seeds and put them and the last seed in the player's kalah.
				unsigned char kalahIndex = AMBO_PLAYER_COUNT + playerIndex * AMBO_PLAYER_COUNT + playerIndex;
				this->ambos[mirrorIndex] = 0;
				this->ambos[index] = 0;
				this->ambos[kalahIndex] += nrOfSeeds + 1; 
			}
		}

		// Check if board state has become a terminal state.
		char isTerminalState = this->IsTerminalState();
		if(isTerminalState != -1) // If one side is empty...
		{
			// ... move the other side's seeds into the kalah.
			for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
			{
				// Kalaha += seeds in ambo.
				this->ambos[AMBO_PLAYER_COUNT + isTerminalState * AMBO_PLAYER_COUNT + isTerminalState] += this->ambos[i + isTerminalState * AMBO_PLAYER_COUNT + isTerminalState];
				// Ambo = 0.
				this->ambos[i + isTerminalState * AMBO_PLAYER_COUNT + isTerminalState] = 0;
			}
		}
		return true;
	}

	return false;
}


char Board::CanGetExtraTurn(unsigned char playerIndex) const 
{
	unsigned char nrOfSeeds = 0;
	unsigned char lastSeedIndex = 0;
	unsigned char kalahIndex = 0;

	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		nrOfSeeds = this->GetNrOfSeeds(i, playerIndex);
		// i = start.
		// nrOfSeeds = steps to increment (end).
		// playerIndex * AMBO_PLAYER_COUNT + playerIndex = player-offset.
		lastSeedIndex = (i + nrOfSeeds + playerIndex * AMBO_PLAYER_COUNT + playerIndex) % AMBO_COUNT;
		kalahIndex = AMBO_PLAYER_COUNT + playerIndex * AMBO_PLAYER_COUNT + playerIndex;

		// The opponents kalah shall be stepped over, so check how many times we iterate over it.
		
		// i = start index.
		// i + nrOfSeeds = end index.
		// Range [0, AMBO_COUNT-1] of (i + nrOfSeeds), therefore to get correct number of iterations,
		// subtract 1 from AMBO_COUNT and do integer division.
		lastSeedIndex += (i + nrOfSeeds) / (AMBO_COUNT - 1);
		


		// lastSeedIndex now has the correct index of the ambo the last seed is put in.
		if(lastSeedIndex == kalahIndex) // If last seed can be put in own kalah...
		{
			return 1 + (playerIndex * -2); // ...return 1 for Max and -1 for Min.
		}
	}

	return 0;
}


char Board::CanGetOpponentSeeds(unsigned char playerIndex) const
{
	unsigned char nrOfSeeds = 0;
	unsigned char lastSeedIndex = 0;
	unsigned char mirrorIndex = 0;
	unsigned char maxNrOfSeeds = 0;
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		nrOfSeeds = this->GetNrOfSeeds(i, playerIndex);
		if(nrOfSeeds > 0)
		{
			// playerIndex * AMBO_PLAYER_COUNT + playerIndex = where to start, depending on the player.
			// i = where to start (ambo Index).
			// nrOfSeeds = how many steps to increment.
			lastSeedIndex = (playerIndex * AMBO_PLAYER_COUNT + playerIndex + i + nrOfSeeds) % AMBO_COUNT; // Range [0, AMBO_COUNT-1].
			// First check if last seed lands in an owned, empty ambo.
			nrOfSeeds = this->ambos[lastSeedIndex];
			if(nrOfSeeds == 0) 
			{
				// Range [0, AMBO_PLAYER_COUNT - 1] of lastSeedIndex = max (us, 0).
				// Range [AMBO_PLAYER_COUNT + 1, AMBO_COUNT - 1] of lastSeedIndex = min (opponent, 1).
				if(lastSeedIndex > AMBO_PLAYER_COUNT && lastSeedIndex < AMBO_COUNT - 1 && playerIndex == 1) 
				{
					// ... check mirror (opponent) ambo.
					// AMBO_COUNT - 1 = index-end of min's (opponent) row of ambos.
					// Note that the last "-1" is the kalah.
					mirrorIndex = AMBO_COUNT - 1 - lastSeedIndex - 1; // Range [AMBO_PLAYER_COUNT + 1, AMBO_COUNT - 1].
					nrOfSeeds = this->ambos[mirrorIndex];
				}
				else if(lastSeedIndex < AMBO_PLAYER_COUNT && playerIndex == 0)
				{
					// ... check mirror (opponent) ambo.
					// AMBO_PLAYER_COUNT = max's (us) kalah.
					// AMBO_PLAYER_COUNT - lastSeedIndex = the number of steps to go from the kalah.
					mirrorIndex = AMBO_PLAYER_COUNT + (AMBO_PLAYER_COUNT - lastSeedIndex); // Range [0, AMBO_PLAYER_COUNT - 1].
					nrOfSeeds = this->ambos[mirrorIndex];
				}
				// If not empty...
				if(nrOfSeeds > 0)
				{
					// ... then save the number of seeds in that ambo.
					// Do this for all to determine the maximum amount of seeds that can be stolen.
					maxNrOfSeeds = max(maxNrOfSeeds, nrOfSeeds); 
				}
			}
		}
	}
	// The maximum amount of seeds that can be stolen is now determined.
	// Don't forget to add the seed that made the capture possible.
	if(maxNrOfSeeds > 0)
	{
		maxNrOfSeeds++;
	}

	// Return the amount of seeds that can be stolen. Negative if Min is the one who can steal seeds.
	return maxNrOfSeeds + (playerIndex * maxNrOfSeeds * -2);
}

string Board::ToString() const
{
	stringstream ss;
	ss << "  ";
	for(char i = AMBO_COUNT - 2; i > AMBO_PLAYER_COUNT; i--)
	{
		ss << "(" << (short int)this->ambos[i] << ")";
	}
	ss << endl;
	ss << "(" << (short int)this->ambos[AMBO_COUNT - 1] << ")" <<  "                " << "(" << (short int)this->ambos[AMBO_PLAYER_COUNT] << ")";
	ss << endl;
	ss << "  ";
	for(char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		ss << "(" << (short int)this->ambos[i] << ")";
	}
	ss << endl;

	return ss.str();
}