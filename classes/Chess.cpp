#include "Chess.h"
#include <limits>
#include <cmath>
#include "../Application.h"
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
    //FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    FENtoBoard("2rq3r/pp1bk3/4p2p/n6R/2nRNB2/2Q3P1/P4PB1/2K5");
    
    
    //pre-compute an array of all valid knight moves
    for(int i = 0; i < 64; i++) { //temporary change for debugging
        _knightboards[i] = generateKnightMovesBitboard(i);
        _kingboards[i] = generateKingMovesBitboard(i);
    }

    //generate all of the moves at the start of the game. Going to have to do that more often when pawns are implimented.
    std::string state = stateString();
    _moves = generateAllMoves(state);
    //print all moves for testing
    Logger *L = Logger::getInstance();
    L->LogGameEvent("All moves at start of game: ");
    for(int i = 0; i < _moves.size(); i++) {
        L->LogInfo("Piece " + to_string(_moves[i].piece) + " from " + to_string(_moves[i].from) + " to " + to_string(_moves[i].to));
    }

    _gameOver = false;
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
    //L->LogGameEvent("Initalizing board from fen string.");
    while(boardIndex < 64) { //i think it doesn't matter if boardIndex is different from bitboards and the statestring
        int file = boardIndex % 8; //x cord. x cord is the same between fen and statestring
        char n = fen[stringIndex];
        //L->LogInfo("Looking at board index space " + to_string(boardIndex) + ". rank: " + to_string(rank) + " file: " + to_string(file));
        //L->LogInfo("Looking at fen[" + to_string(stringIndex) + "] = " + n);
        int piecesIndex = (int) pieces.find(n); //index of char within string pieces
        if(isdigit(n)) { //check if it is a digit
            //if it is skip forward that amount
            int digit = atoi(&n);
            //L->LogInfo("n is a digit, skipping forward " + to_string(digit) + " spaces.");
            boardIndex = boardIndex + digit;
            stringIndex++;
        } else if(piecesIndex != std::string::npos) {
            int piecenum = piecesIndex % 7;
            if(piecenum == 0) { //if it is a /, decrement rank and then go on to the next char without doing anything to the board
                rank--;
                stringIndex++;
                //L->LogInfo("n is /, decreming row.");
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
                //L->LogInfo("creating piece of num " + to_string(piecenum) + " for player " + to_string(playernum));
                //L->LogInfo("Assigning it to square at file = " + to_string(file) + " rank: " + to_string(rank));
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
            //Logger *L = Logger::getInstance();
            //L->LogError("Invalid character found in fen string at index " + to_string(stringIndex) + ": " + n + ". Character will be ignored.");
            stringIndex++;
            //currently all the extra stuff at the end is just ignored, doesn't even throw an error
        }
    }
    string s = stateString();
    //L->LogGameEvent("state string: " + s);
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}


//new overrides
void Chess::endTurn() { //copy of the version from Game, just with new calculation of moves

	_gameOptions.currentTurnNo++;
	std::string startState = stateString();
    _moves = generateAllMoves(startState);
	Turn *turn = new Turn;
	turn->_boardState = stateString();
	turn->_date = (int)_gameOptions.currentTurnNo;
	turn->_score = _gameOptions.score;
	turn->_gameNumber = _gameOptions.gameNumber;
	_turns.push_back(turn);
	ClassGame::EndOfTurn();
}

//traverse all of the squares included in _highlightIndexes and unhilight them
void Chess::clearBoardHighlights() {
    //Logger *L = Logger::getInstance();
    //L->LogGameEvent("clearing highlights");
    for(int i = 0; i < _highlightedIndexes.size(); i++) {
        int index = _highlightedIndexes[i];
        //L->LogInfo("clearing at index " + to_string(index));
        ChessSquare *square = _grid->getSquareByIndex(index);
        square->setHighlighted(false);
    }
    _highlightedIndexes.clear();
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
    //capturing logic to go here
    //this is called before the move is complete, and dst still holds the old bit
    //dst.destroyBit();
    endTurn();
}

//also handles highlighting
bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    //prevent movement when the game is over
    if(_gameOver) {
        return false;
    }
    
    //Logger *L = Logger::getInstance();
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) {
        //if the piece is yours
        bool ret = false;
        ChessSquare* square = (ChessSquare *)&src;
        if(square) {
            int squareIndex = square->getSquareIndex();
            for(auto move : _moves) { //check each move
                if(move.from == squareIndex) {
                    ret = true;
                    auto dest = _grid->getSquareByIndex(move.to);
                    dest->setHighlighted(true);
                    //L->LogInfo("adding " + to_string(move.to) + " to _highlightIndexes");
                    _highlightedIndexes.push_back(move.to);
                }
            }
        }
        return ret;
    }
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{

    //prevent movement when the game is over
    if(_gameOver) {
        return false;
    }    
    //convert to chess squares
    ChessSquare* Sdst = (ChessSquare *)&dst;
    ChessSquare* Ssrc = (ChessSquare *)&src;
    if(Sdst) {
        int dstIndex = Sdst->getSquareIndex();
        int srcIndex = Ssrc->getSquareIndex();
        for(auto move : _moves) {
            if(move.to == dstIndex && move.from == srcIndex) {
                return true;
            }
            
        }
    }
    
    return false;
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

//winner only checks when the turn ends, so currently not tested
Player* Chess::checkForWinner()
{
    //Logger *L = Logger::getInstance();
    //0 = white, 1 = black
    string s = stateString();
    //use find to check for kings existing
    if(s.find('K') == string::npos) { //look for white king
        //if no white king:
        //L->LogGameEvent("There is no white king");
        Player* winner = getPlayerAt(1);
        _gameOver = true;
        return winner;
    } else if( s.find('k') == string::npos) { 
        //if no black king:
        //L->LogGameEvent("There is no black king");
        Player* winner = getPlayerAt(0);
        _gameOver = true;
        return winner;
    } else {
        //L->LogGameEvent("both kings exist");
        return nullptr; 
    }
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

//MOVE GENERATION FUNCTIONS

 std::vector<Chess::BitMove> Chess::generateAllMoves(std::string &state) {
    std::vector<BitMove> moves;
    constexpr uint64_t oneBit = 1; //in lecture, these occupancy boards have 0 for where the units are, which I don't get

    //when these boards are passed by values they are automatically converted to BitboardElements
    uint64_t whiteKnights = 0LL;
    uint64_t whitePawns = 0LL;
    uint64_t whiteRooks = 0LL;
    uint64_t whiteBishops = 0LL;
    uint64_t whiteQueen = 0LL;
    uint64_t whiteKing = 0LL;

    uint64_t blackKnights = 0LL;
    uint64_t blackPawns = 0LL;
    uint64_t blackRooks = 0LL;
    uint64_t blackBishops = 0LL;
    uint64_t blackQueen = 0LL;
    uint64_t blackKing = 0LL;
    //complete occupancy bitboards even if move generation isn't there yet
    for(int i = 0; i < 64; i++) {
        if(state[i] == 'N') {
            whiteKnights |= oneBit << i;
        }
        else if(state[i] == 'P') {
            whitePawns |= oneBit << i;
        } else if(state[i] == 'R') {
            whiteRooks |= oneBit << i;
        } else if(state[i] == 'B') {
            whiteBishops |= oneBit << i;
        } else if(state[i] == 'Q') {
            whiteQueen |= oneBit << i;
        } else if(state[i] == 'K') {
            whiteKing |= oneBit << i;
        } else if(state[i] == 'n') {
            blackKnights |= oneBit << i;
        } else if(state[i] == 'p') {
            blackPawns |= oneBit << i;
        } else if(state[i] == 'r') {
            blackRooks |= oneBit << i;
        } else if(state[i] == 'b') {
            blackBishops |= oneBit << i;
        } else if(state[i] == 'q') {
            blackQueen |= oneBit << i;
        } else if(state[i] == 'k') {
            blackKing |= oneBit << i;
        }
        
    }
    //cout << "after turn " << _turns.size() << " black king bitboard: " << blackKing;
    uint64_t whiteOccupancy = whiteKnights | whitePawns | whiteRooks | whiteBishops | whiteQueen;
    uint64_t blackOccupancy = blackKnights | blackPawns | blackRooks | blackBishops | blackQueen;
    uint64_t occupancy = whiteOccupancy | blackOccupancy; //keep updating as I add new functions
    //each generate only works on a side at the time, will probably be useful for making the AI
    generateKnightMoves(moves, whiteKnights, ~whiteOccupancy); //only send spaces that are occupied by allies
    generateKingMoves(moves, whiteKing, ~whiteOccupancy);
    generateKnightMoves(moves, blackKnights, ~blackOccupancy);
    generateKingMoves(moves, blackKing, ~blackOccupancy);
    //generate pawn is different wanting occupancy rather than free spaces
    generatePawnMoves(moves, whitePawns, whiteOccupancy, blackOccupancy, White);
    generatePawnMoves(moves, blackPawns, blackOccupancy, whiteOccupancy, Black);
    return moves; //doesn't directly modify the member variable _moves so it can be reused
}

//generate bitboard of all possible locations a knight at square could do
BitboardElement Chess::generateKnightMovesBitboard(int square) {
    BitboardElement bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    //cout << "generating bitboard for rank " << to_string(rank) << " file " << to_string(file) << "\n";
    //all possible knight moves from square
    std::pair<int, int> knightOffsets[] = {
        {2, 1}, {1, 2}, {2, -1}, {-1, 2}, {-1, -2}, {-2, -1}, {-2, 1}, {1, -2}
    };

    constexpr uint64_t oneBit = 1;
    //apply all of the offsets
    for(auto [dr, df] : knightOffsets) {
        int r = rank + dr; 
        int f = file + df;
        //cout << "found move at rank " << to_string(r) << " file " << to_string(f) << "\n";
        //check if in range and then add to bitboard
        if(r >= 0 && r < 8 && f >= 0 && f < 8) {
            //cout << "valid to add to the bitboard\n";
            bitboard |= oneBit  << (r * 8 + f);
        }
    }
    //cout << "final bitboard for " << to_string(square) << "\n";
    //bitboard.printBitboard();
    return bitboard;
}

//generate bitboard of all possible locations a king at a square could do
//only like this for two kinds of pieces so redunacy isn't a big deal
BitboardElement Chess::generateKingMovesBitboard(int square) {
    BitboardElement bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    //cout << "generating bitboard for rank " << to_string(rank) << " file " << to_string(file) << "\n";
    //all possible knight moves from square
    std::pair<int, int> kingOffsets[] = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0}
    };

    constexpr uint64_t oneBit = 1;
    //apply all of the offsets
    for(auto [dr, df] : kingOffsets) {
        int r = rank + dr; 
        int f = file + df;
        //cout << "found move at rank " << to_string(r) << " file " << to_string(f) << "\n";
        //check if in range and then add to bitboard
        if(r >= 0 && r < 8 && f >= 0 && f < 8) {
            //cout << "valid to add to the bitboard\n";
            bitboard |= oneBit  << (r * 8 + f);
        }
    }
    //cout << "final bitboard for " << to_string(square) << "\n";
    //bitboard.printBitboard();
    return bitboard;
}

//take bitboard of all locations of that side's knights and all the squares that are empty
void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares) {
    if(knightBoard.getData() == 0) {
        return;
    }
    knightBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_knightboards[fromSquare].getData() & emptySquares); //return the bitboard for
        //std::cout << "Bitboard for " << to_string(fromSquare) << ":\n";
        //moveBitboard.printBitboard();
        //that knight but only the oens that are empty
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

//same code but for king
void Chess::generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares) {
    //no need for return if empty because if a side doesn't have a king, there is no more game
    kingBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_kingboards[fromSquare].getData() & emptySquares); //return the bitboard for
        //std::cout << "Bitboard for " << to_string(fromSquare) << ":\n";
        //moveBitboard.printBitboard();
        //that knight but only the oens that are empty
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

//notes on pawn move generation:
//pawns << 8 up one rank
//pawns << 16 up two ranks
//for black reverse bit shift
//singlePush = pawns << 8
//singlePush & ~occupied //make sure it empty
//pawn movement cases:
//1. move forward 1
//2. move forward 2 only if in starting rank
//3. capture diagnonally


//onSecondRank = singlePush & RANK_3 //only pawns that will be on rank 3 after moving 1, which means they can move twice
//doublePush = onSecondRank  << 8
//attack left << 7
//attack right << 9
//invert direction of shift for black, keep numbers the same for same direction, the left/rights of white player

//takes a bitboard, its values are destinations. uses provided offset to calcuate sources. adds results to move list
//offset should be the difference from source to destination
void Chess::PawnMovesFromBitboard(std::vector<BitMove>&moves, BitboardElement board, int offset) {
    if(board.getData() == 0) { //return if empty
        return;
    }
    board.forEachBit([&](int toSquare) {
        int fromSquare = toSquare - offset;
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
}

void Chess::generatePawnMoves(std::vector<BitMove>&moves, BitboardElement pawns, BitboardElement selfOccupancy, BitboardElement enemyOccupany, ChessSide side) {
    //masks
    const BitboardElement NOT_FA(0xFEFEFEFEFEFEFEFEULL);
    const BitboardElement NOT_FH(0x7F7F7F7F7F7F7F7FULL);
    const BitboardElement RANK_3(0x0000000000FF0000ULL);
    const BitboardElement RANK_6(0x0000FF0000000000ULL);
    BitboardElement allOccupancy = BitboardElement(selfOccupancy.getData() | enemyOccupany.getData());
    //if no pawns, return
    if(pawns.getData() == 0) {
        return;
    }
    //intalize move bitboards so they have the right scope
    BitboardElement singleMoves;
    BitboardElement doubleMoves;
    BitboardElement leftCaptures;
    BitboardElement rightCaptures;

    //there is no pre-compute for pawns
    if(side == White) {
        //I consider this stuff too complicated for ternary operator
        singleMoves.setData((pawns.getData()) << 8 & ~allOccupancy.getData());
        doubleMoves.setData((singleMoves.getData() & RANK_3.getData()) << 8 & ~allOccupancy.getData());
        leftCaptures.setData((pawns.getData() & NOT_FA.getData()) << 7 & enemyOccupany.getData());
        rightCaptures.setData((pawns.getData() & NOT_FH.getData()) << 9 & enemyOccupany.getData());
    } else {
        //reverse directions for black
        singleMoves.setData((pawns.getData()) >> 8 & ~allOccupancy.getData());
        doubleMoves.setData((singleMoves.getData() & RANK_6.getData()) >> 8 & ~allOccupancy.getData());
        leftCaptures.setData((pawns.getData() & NOT_FH.getData()) >> 7 & enemyOccupany.getData());
        rightCaptures.setData((pawns.getData() & NOT_FA.getData()) >> 9 & enemyOccupany.getData());
    }

    //how much each shift is, for extracting moves from the bitboards that were created
    //                    condition    if true
    //                                       if false
    int shiftForward = (side == White) ? 8 : -8;
    int shiftDouble = (side == White) ? 16 : -16;
    //even though it differs from demo, just changing the sign of the value should be the difference I need, since left and right
    //are based on the white player's prespective
    int captureLeft = (side == White) ? 7 : -7;
    int captureRight = (side == White) ? 9 : -9;

    PawnMovesFromBitboard(moves, singleMoves, shiftForward);
    PawnMovesFromBitboard(moves, doubleMoves, shiftDouble);
    PawnMovesFromBitboard(moves, leftCaptures, captureLeft);
    PawnMovesFromBitboard(moves, rightCaptures, captureRight);








    /*//just checking that the masks work
    BitboardElement left(NOT_FA);
    BitboardElement right(NOT_FH);
    BitboardElement top(RANK_6);
    BitboardElement bottom(RANK_3);
    cout << "NOT_FA:";
    left.printBitboard();
    cout << "NOT_FH:";
    right.printBitboard();
    cout << "RANK_6:";
    top.printBitboard();
    cout << "RANK_3:";
    bottom.printBitboard();*/


}