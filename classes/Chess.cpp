#include "Chess.h"
#include <limits>
#include <cmath>
using namespace std;
Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, int piece)
{
    //0 = white
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);
    if(playerNumber != 0) {
        bit->setGameTag(piece + 128); //pieceNotation was created with the difference in gameTags for sides being 128
    } else {
        bit->setGameTag(piece); //this is the one that gives the bit its internal value, instead of just visuals
    }
    
    

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    //not including the extra options
    
    //string of all valid characters in a FEN string. index of that character % 7 will give you the value that
    //corresponds to the correct enum value in ChessPiece
    std::string pieces = "/pnbrqk/PNBRQK";
    
    //both indexices actually start at rank 8. boardIndex is basically just stringIndex but without the slashes or 
    //collapsing of empty spaces with numbers
    int stringIndex = 0; //0 = a8
    int boardIndex = 0; //0 = a1
    int rank = 7; //keeps track of the actual row being used. 7 because this shit is zero indexed. Decrement whenever slash
    //is found
    Logger *L = Logger::getInstance();
    L->LogGameEvent("Initalizing board from fen string.");
    while(boardIndex < 64) { //i think it doesn't matter if boardIndex is different from bitboards and the statestring
        int file = boardIndex % 8; //x cord. x cord is the same between fen and statestring
        char n = fen[stringIndex];
        L->LogInfo("Looking at board index space " + to_string(boardIndex) + ". rank: " + to_string(rank) + " file: " + to_string(file));
        L->LogInfo("Looking at fen[" + to_string(stringIndex) + "] = " + n);
        int piecesIndex = (int) pieces.find(n); //index of char within string pieces
        if(isdigit(n)) { //check if it is a digit
            //if it is skip forward that amount
            int digit = atoi(&n);
            L->LogInfo("n is a digit, skipping forward " + to_string(digit) + " spaces.");
            boardIndex = boardIndex + digit;
            stringIndex++;
        } else if(piecesIndex != std::string::npos) {
            int piecenum = piecesIndex % 7;
            if(piecenum == 0) { //if it is a /, decrement rank and then go on to the next char without doing anything to the board
                rank--;
                stringIndex++;
                L->LogInfo("n is /, decreming row.");
            } else {
                //because enum wants constants as input and gives number as output, replaced chessPiece with an int
                //so I can give numbers as input
                //changed pieceforplayer accordingly
                //p = 1, n = 2, b = 3, r = 4, q = 5, k = 6
                //determin white or black
                int playernum;
                if(piecesIndex > 7) { //capital letters = white. white is second chunk in pieces
                    playernum = 0; //piece for player has 0 = white
                } else {playernum = 1;}

                //get the bitholder and bit pieces needed
                L->LogInfo("creating piece of num " + to_string(piecenum) + " for player " + to_string(playernum));
                L->LogInfo("Assigning it to square at file = " + to_string(file) + " rank: " + to_string(rank));
                Bit *piece = PieceForPlayer(playernum, piecenum);
                ChessSquare *square = _grid->getSquare(file, rank);
                //assign them to eachother
                piece->setPosition(square->getPosition());
                square->setBit(piece);

                //afterwards increment both indices
                stringIndex++;
                boardIndex++;

            }
        } else {
            //exception case. boardIndex is not incremented.
            Logger *L = Logger::getInstance();
            L->LogError("Invalid character found in fen string at index " + to_string(stringIndex) + ": " + n + ". Character will be ignored.");
            stringIndex++;
            //currently all the extra stuff at the end is just ignored, doesn't even throw an error
        }
    }
    string s = stateString();
    L->LogGameEvent("state string: " + s);
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    return true;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
