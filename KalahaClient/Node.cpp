#pragma once

#include "Node.h"
#include "Board.h"

Node::Node()
{
	this->board = nullptr;
	this->utility = 0;	
	this->nrOfChildren = 0;	
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		this->children[i] = nullptr;
	}
}
Node::Node(const Board& board)
{
	this->board = new Board(board);
	this->utility = 0;	
	this->nrOfChildren = 0;	
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		this->children[i] = nullptr;
	}
}
Node::Node(const Node& node)
{
	this->board = new Board(*node.board);
	this->utility = node.utility;	
	this->nrOfChildren = node.nrOfChildren;	
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		this->children[i] = node.children[i];
	}
}
Node::~Node()
{
	if(this->board)
	{
		delete this->board;
		this->board = nullptr;
	}
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		if(this->children[i])
		{
			// Don't invoke the destructor of the children as they are deleted externally.
			this->children[i] = nullptr;
		}
	}
}

bool Node::IsLeaf() const
{
	for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
	{
		// As long as one pointer is not null, the node is not a leaf node.
		if(this->children[i] != nullptr)
		{
			return false;
		}
	}

	return true;
}