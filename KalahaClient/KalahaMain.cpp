/*
	Windows C++ client for playing a Kalaha game using the Kalaha game server.
	The client uses the Wsock32.lib library for connecting to the server.

	Author: Johan Hagelbäck (jhg@bth.se)

	Extended by Markus Tillman.
*/

#include <winsock.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

#include <sstream>
#include <conio.h>
#include <fstream>

using namespace std;

#include "Minimax.h"

#pragma comment(lib, "wsock32.lib")
#ifdef _DEBUG
	#include "vld.h" // Debug, to locate memory leaks.
#endif

void gameLoop(int mySocket, int player);
void makeMove(int mySocket, int player);
void sendMoveCmd(int mySocket, int player, int myMove);
string makeBoardStr(char input[]);
string makeSpaces(string str);
void tokenizeBoard(char input[], vector<string> &tokens);

enum ERROR_CODE
{
	ERROR_GAME_FULL				= 0,
	ERROR_GAME_NOT_FULL			= 1,
	ERROR_ARGLENGTH_NOT_VALID	= 2,
	ERROR_ARGTYPE_NOT_VALID		= 3,
	ERROR_PLAYER_OUT_OF_TURN	= 4,
	ERROR_CMD_NOT_FOUND			= 5,
	ERROR_AMBO_EMPTY			= 6
};

struct Config
{
	int port;
	string address;
	unsigned char startDepth;
	unsigned int sleepTime;
	unsigned short int timeLimit; // In milliseconds.
	unsigned int nrOfGames;
};

Config config;

bool ReadConfigFile(const char* fileName);
void GetBoard(int socket, unsigned char* ambos);
void PrintBoard(int socket);
string ErrorCodeToString(ERROR_CODE errorCode);
int ErrorCode(char input[]); // Return -1 if there's no error.


int main(int a, char *args[]) 
{
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // Debug, to detect memory leaks.
#endif

	// Read configuration file.
	if(!ReadConfigFile("Config.cfg"))
	{
		cout << "Failed to load config file. " << endl;
		cout << "Using default values instead. " << endl;
		config.port = 8888;
		config.address = "127.0.0.1";
		config.startDepth = 8;
		config.sleepTime = 100;
		config.timeLimit = 3000;
		config.nrOfGames = 2;
	}

	//Connection details
	int PORT = config.port;
	const char* IP = config.address.c_str();

	//Initialize that a winsock shall be used
	WSADATA ws;
	WSAStartup(0x0101, &ws);

	//Create and configure the socket
	int mySocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in peer; 
	peer.sin_family = AF_INET;
	peer.sin_port = htons((u_short)PORT);
	peer.sin_addr.s_addr = inet_addr(IP);

	//Connect to server and game
	connect(mySocket, (struct sockaddr*)&peer, sizeof(peer));
	send(mySocket, "HELLO\n", 6, 0);
	
	//Check which player you are
	char input[43]; 
	recv(mySocket, input, sizeof(input), 0);
	if (input[6] != '1' && input[6] != '2')  
	{
		cout << "Error connecting to Kalaha server" << endl;
		cout << "Reason: " << ErrorCodeToString((ERROR_CODE)ErrorCode(input)) << endl;
		system("pause");
		return 0;
	}
	int player = input[6] - '0';

	cout << "Connected to Kalaha server as player " << player << endl;

	gameLoop(mySocket, player);

	system("pause");
}

void gameLoop(int mySocket, int player) 
{
	bool gameRunning = true;
	int opponent;
	if(player == 1)
	{
		opponent = 2;
	}
	else
	{
		opponent = 1;
	}
	char input[43];

	unsigned int nrOfVictories[2] = {0, 0}; 
	unsigned int nrOfGamesCap = config.nrOfGames;
	Minimax minimax = Minimax();
	minimax.SetTimeLimit(config.timeLimit);
	bool once = false;

	while (gameRunning) 
	{
		// Check if anyone has won yet.
		send(mySocket, "WINNER\n", 7, 0);
		recv(mySocket, input, sizeof(input), 0);

		int errorCode = ErrorCode(input);
		if(errorCode != -1)
		{
			cout << ErrorCodeToString((ERROR_CODE)errorCode) << endl;
		}
		else
		{
			char winner = ' ';
			if(input[0] == '-')
			{
				winner = -1;
			}
			else
			{
				winner = input[0] - '0';
			}
			if(winner == -1)
			{
				// Check whose turn it is
				send(mySocket, "PLAYER\n", 7, 0);
				recv(mySocket, input, sizeof(input), 0);

				int errorCode = ErrorCode(input);
				if(errorCode != -1)
				{
					cout << ErrorCodeToString((ERROR_CODE)errorCode) << endl;
				}
				else
				{
					int nextToMove = input[0] - '0';

					// Check if it is our (max's) turn to make a move.
					if(nextToMove == player) 
					{
						once = false;

						unsigned char ambos[AMBO_COUNT];
						GetBoard(mySocket, ambos);
						Board currentBoard = Board(ambos);
						// Max (us) is always assumed to be the first player.
						// Therefore swap the board around if max is the second player.
						if(player == 2)
						{
							currentBoard.Swap();
						}
						Node* currentNode = new Node(currentBoard);
						int myMove = 0;
						unsigned char depth = config.startDepth; // Reset start depth.
						unsigned short int timeElapsed = 0;
						char bestUtility = SCHAR_MIN;

						// Set start time for the search.
						minimax.SetStartTime();
						while(timeElapsed < config.timeLimit && depth < 37) //**Todo: config-variabel och/eller kommentar(motivering) 37**
						{
							// De-allocate memory from previous search.
							minimax.DeAllocate(currentNode);
							currentNode = new Node(currentBoard); // as Minimax::DeAllocate(..) deletes currentNode aswell, so recreate it.
							// Generate and search a new game tree.
							minimax.Generate(currentNode, depth, false, timeElapsed);

							// Check child nodes of root node to select the best move.
							for(unsigned char i = 0; i < AMBO_PLAYER_COUNT; i++)
							{
								if(currentNode->children[i])
								{
									if(bestUtility < currentNode->children[i]->utility)
									{
										bestUtility = currentNode->children[i]->utility;
										myMove = i + 1;
									}
								}
							}

							timeElapsed = (unsigned short int)(timeGetTime() - minimax.GetStartTime()); 
							depth++; // Increase depth for next search.
						}

						// Print the current board (after the opponents move).
						cout << endl;
						cout << "Previous move, board: " << endl;
						PrintBoard(mySocket);

						// Send move command to the Kalaha server.
						sendMoveCmd(mySocket, player, myMove);

						// Print again after our move.
						cout << endl;
						cout << "You have made your move, board: " << endl;
						PrintBoard(mySocket);

						// De-allocate the memory used for the latest search.
						minimax.DeAllocate(currentNode);
					}
					else
					{    
						if(!once)
						{
							ostringstream stream;   
							stream << "Waiting for player: "  << nextToMove << endl;    
							cout << stream.str();

							once = true;
						}
					}
				}

				// Wait a bit
				Sleep(config.sleepTime); 
			}
			else
			{
				cout << endl;
				cout << "Final board state (and score): " << endl;
				PrintBoard(mySocket);

				string winnerStr = "";
				if(winner == player)
				{
					winnerStr = "you";
					nrOfVictories[player - 1]++; // Increment total victories for player (us).
				}
				else if(winner == 0) // Draw.
				{
					winnerStr = "none";
				}
				else
				{
					winnerStr = "opponent";
					nrOfVictories[opponent - 1]++; // Increment total victories for opponent.
				}
				cout << endl;
				cout << "Player " << (int)winner << "(" << winnerStr << ") won this round!" << endl;

				if(nrOfVictories[0] + nrOfVictories[1] < nrOfGamesCap) //**todo: ta bort/göra om allt till turnering**
				{
					// Play again if no final victor has emerged.
					Sleep(3000); // Wait a little before starting the next round.
					send(mySocket, "NEW\n", 4, 0);
					recv(mySocket, input, sizeof(input), 0);
				}
				else
				{
					// Display who won the game. 
					string motivationalSpeech = "Congratulations! You have just won the dumbass award because ";
					winner = (char)opponent;
					if(nrOfVictories[player - 1] > nrOfVictories[opponent - 1]) 
					{
						// Oh you won? *Ahem* I.. uh.. I mean:
						motivationalSpeech = "Congratulations! ";
						winner = (char)player; 
					}
					cout << motivationalSpeech << "Player " << (int)winner << "(" << winnerStr << ") won the game. (By the way, you who is reading this right now just lost the game.)" << endl;
					cout << "Problem?" << endl;
					gameRunning = false;
				}
			}
		}
	}
}

void makeMove(int mySocket, int player) 
{
	//Ask the player for his move
	cout << "\nYou are next! make a move." << endl;
	cout << "[1,2,3,4,5,6] > ";
	int myMove = 0;
	cin >> myMove;

	//Send a move command to the Kalaha server.
	sendMoveCmd(mySocket, player, myMove);
}

void sendMoveCmd(int mySocket, int player, int myMove) 
{
	char input[43];
	char output[9];

	//Generate the command string
	output[0] = 'M';
	output[1] = 'O';
	output[2] = 'V';
	output[3] = 'E';
	output[4] = ' ';
	output[5] = (char)myMove + '0';
	output[6] = ' ';
	output[7] = (char)player + '0';
	output[8] = '\n';
	
	//Send the command
	send(mySocket, output, sizeof(output), 0);
	recv(mySocket, input, sizeof(input), 0);

	int errorCode = ErrorCode(input);
	if(errorCode != -1)
	{
		cout << ErrorCodeToString((ERROR_CODE)ErrorCode(input)) << endl;
	}
}

string makeBoardStr(char input[]) 
{
	//Convert the board datastructure to a vector of ; separated tokens.
	vector<string> myBoard;
	tokenizeBoard(input, myBoard);

	//Generate a nice output of the board.
	string out = "\n[2]";

	for(int i = AMBO_COUNT - 1; i > AMBO_PLAYER_COUNT + 1; i--)
	{
		out += makeSpaces(myBoard.at(i));
	}

	out += "\n" + makeSpaces(myBoard.at(0)) + "                  " + makeSpaces(myBoard.at(7)) + "\n" + "[1]";

	for(int i = 1; i < AMBO_PLAYER_COUNT + 1; i++)
	{
		out += makeSpaces(myBoard.at(i));
	}

	return out;
}

string makeSpaces(string str) 
{
	string res = "";

	//Formats a number (0-99) to a nice string.
	if(str.length() == 2) 
	{
		res = " " + str;
	}
	else if(str.length() == 1) 
	{
		res = "  " + str;
	}
	else 
	{
		res = str;
	}

	return res;
}

void tokenizeBoard(char input[], vector<string> &tokens) 
{
	//Converts a board datastructure to a vector of ; separated tokens.
	char sep[] = ";";
	char *token;
	token = strtok(input, sep);

	while (token != NULL) 
	{
		tokens.push_back(token);
		token = strtok(NULL, sep);
	}
}




bool ReadConfigFile(const char* fileName)
{
	ifstream in;
	in.open(fileName);
	char input[256];
	if(in)
	{
		// Port number.
		in.getline(input, sizeof(input));
		while(input[0] == '#' || input[0] == '\0')
		{
			in.getline(input, sizeof(input));
		}
		config.port = atoi(input);

		// IP-address.
		in.getline(input, sizeof(input));
		while(input[0] == '#' || input[0] == '\0')
		{
			in.getline(input, sizeof(input));
		}
		config.address = input;

		// Start depth of search.
		in.getline(input, sizeof(input));
		while(input[0] == '#' || input[0] == '\0')
		{
			in.getline(input, sizeof(input));
		}
		config.startDepth = (unsigned char)atoi(input);

		// Sleep time (update frequency) in milliseconds.
		in.getline(input, sizeof(input));
		while(input[0] == '#' || input[0] == '\0')
		{
			in.getline(input, sizeof(input));
		}
		config.sleepTime = atoi(input);

		// Time limit of search in milliseconds.
		in.getline(input, sizeof(input));
		while(input[0] == '#' || input[0] == '\0')
		{
			in.getline(input, sizeof(input));
		}
		config.timeLimit = (unsigned short int)atoi(input);

		// Number of games to play.
		in.getline(input, sizeof(input));
		while(input[0] == '#' || input[0] == '\0')
		{
			in.getline(input, sizeof(input));
		}
		config.nrOfGames = (unsigned int)atoi(input);

		in.close();
		return true;
	}
	else
	{
		in.close();
		return false;
	}
}

void GetBoard(int socket, unsigned char* ambos)
{
	// Ask server for current board.
	char input[43];
	send(socket, "BOARD\n", 6, 0);
	recv(socket, input, sizeof(input), 0);

	string tmpStr = input;
	unsigned char amboIndex = 0;
	unsigned char stringStartIndex = 0;
	// Get the number of seeds in player 2's kalah.
	while(input[stringStartIndex] != ';')
	{
		stringStartIndex++;
		if(input[stringStartIndex] == ';')
		{
			ambos[AMBO_COUNT - 1] = (unsigned char)atoi(tmpStr.substr(0, stringStartIndex).c_str());
		}
	}
	// Get the rest.
	stringStartIndex++;
	for(unsigned char i = stringStartIndex; i < 43; i++)
	{
		if(input[i] == ';')
		{
			ambos[amboIndex++] = (unsigned char)atoi(tmpStr.substr(stringStartIndex, i - stringStartIndex).c_str());
			stringStartIndex = i + 1;
		}
	}
}

void PrintBoard(int socket)
{
	char input[43];

	//Ask for current board
	send(socket, "BOARD\n", 6, 0);
	recv(socket, input, sizeof(input), 0);

	//Convert the received board data structure to a printable string
	string out = makeBoardStr(input);

	//Print the board
	cout << out << endl;
}

string ErrorCodeToString(ERROR_CODE errorCode)
{
	switch (errorCode)
	{
		case ERROR_GAME_FULL: 
			return "Can't connect to server: Game is full."; 
		break;
		case ERROR_GAME_NOT_FULL: 
			return "Can't make a move: Only one client connected."; 
		break;
		case ERROR_ARGLENGTH_NOT_VALID: 
			return "Invalid command format. Too few or too many arguments."; 
		break;
		case ERROR_ARGTYPE_NOT_VALID: 
			return "Invalid command format. Usually caused by expecting an integer but received a string."; 
		break;
		case ERROR_PLAYER_OUT_OF_TURN:
			return "Not your turn!"; 
		break;
		case ERROR_CMD_NOT_FOUND:
			return "Unknown command."; 
		break;
		case ERROR_AMBO_EMPTY: 
			return "Can't make a move from an empty ambo!"; 
		break;
		default: return "Unknown. Maybe the server is dead."; break;
	}
}

int ErrorCode(char input[])
{
	string tmp = input;
	
	if(tmp.find("ERROR GAME_FULL") != -1)
	{
		return ERROR_GAME_FULL;
	}
	else if(tmp.find("ERROR GAME_NOT_FULL") != -1)
	{
		return ERROR_GAME_NOT_FULL;
	}
	else if(tmp.find("ERROR ARGLENGTH_NOT_VALID") != -1)
	{
		return ERROR_ARGLENGTH_NOT_VALID;
	}
	else if(tmp.find("ERROR ARGTYPE_NOT_VALID") != -1)
	{
		return ERROR_ARGTYPE_NOT_VALID;
	}
	else if(tmp.find("ERROR PLAYER_OUT_OF_TURN") != -1)
	{
		return ERROR_PLAYER_OUT_OF_TURN;
	}
	else if(tmp.find("ERROR CMD_NOT_FOUND") != -1)
	{
		return ERROR_CMD_NOT_FOUND;
	}
	else if(tmp.find("ERROR AMBO_EMPTY") != -1)
	{
		return ERROR_AMBO_EMPTY;
	}
	else
	{
		return -1;
	}
}