#pragma once

#include "Board.h"

class Node
{
	public: 
		Board*			board;		
		char			utility;	
		unsigned char	nrOfChildren;	
		Node*			children[AMBO_PLAYER_COUNT];
																		
	public:
		Node();
		Node(const Board& board);
		Node(const Node& node);
		virtual~Node();

		/*
			Checks whether or not this node has any children.
			Returns true if node is a parent, else false.
		*/
		bool IsLeaf() const;
};