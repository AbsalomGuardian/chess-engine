#include "Chess.h"
#include <limits>
#include <cmath>
#include "../Application.h"
using namespace std;
#include "MagicBitboards.h"


Chess::Chess()
{
    _grid = new Grid(8, 8);
    //intalize boardLookup with the _bitboards index for each piece
    _boardLookup['0'] = EMPTY;
    _boardLookup['P'] = WHITE_PAWNS;
    _boardLookup['N'] = WHITE_KNIGHTS;
    _boardLookup['B'] = WHITE_BISHOPS;
    _boardLookup['R'] = WHITE_ROOKS;
    _boardLookup['Q'] = WHITE_QUEEN;
    _boardLookup['K'] = WHITE_KING;
    _boardLookup['p'] = BLACK_PAWNS;
    _boardLookup['n'] = BLACK_KNIGHTS;
    _boardLookup['b'] = BLACK_BISHOPS;
    _boardLookup['r'] = BLACK_ROOKS;
    _boardLookup['q'] = BLACK_QUEEN;
    _boardLookup['k'] = BLACK_KING;

    initMagicBitboards();
    
    _gameOptions.AIMAXDepth = 4;
    _AI_COLOR = Black;
    
}

Chess::~Chess()
{
    delete _grid;
    cleanupMagicBitboards();
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
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    //FENtoBoard("2rq3r/pp1bk3/4p2p/n6R/2nRNB2/2Q3P1/P4PB1/2K5");
    
    
    //pre-compute valid moves for applicable pieces
    for(int i = 0; i < 64; i++) {
        _knightboards[i] = generateKnightMovesBitboard(i);
        _kingboards[i] = generateKingMovesBitboard(i);
    }

    //generate all of the moves for the start of the game.
    std::string state = stateString();
    Logger *L = Logger::getInstance();
    //L->LogInfo("Calling generate all moves as setUpBoard()");
    _moves = generateAllMoves(state, White); //obviously first generate white moves
    //print all moves for testing
    //L->LogGameEvent("All moves at start of game: ");
    //for(int i = 0; i < _moves.size(); i++) {
        //L->LogInfo("Piece " + to_string(_moves[i].piece) + " from " + to_string(_moves[i].from) + " to " + to_string(_moves[i].to));
    //}

    _gameOver = false;
    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }
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
//promotion_index will be -1 if there's no promotion to check. Otherwise calls the relevant function.
//promotion handles update of state string
void Chess::endTurn(int promotion_index) { //copy of the version from Game, just with promotion logic added

	_gameOptions.currentTurnNo++;
	std::string startState = stateString();
    if(promotion_index != -1) {
        Promotion(promotion_index, startState, true);
        startState = stateString(); //update state string
    }
    
	Turn *turn = new Turn;
	turn->_boardState = stateString();
	turn->_date = (int)_gameOptions.currentTurnNo;
	turn->_score = _gameOptions.score;
	turn->_gameNumber = _gameOptions.gameNumber;
    int p_num = getCurrentPlayer()->playerNumber(); //p_num will be the number of whose turn has started
    Logger *L = Logger::getInstance();
    //L->LogInfo("Calling generate all moves as end turn");
    if(p_num == 1) {_moves = generateAllMoves(startState, Black);}
    else {_moves = generateAllMoves(startState, White);}
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


//check at index (has to be top or bottom rows) if there's a pawn on the other side in passed string
//if so set to string. Generate all moves will handle the changes in the bitboards
//AI calls with real false, actual move logic is true. It will change the stuff on the actual grid
//0-7 and 57-64 are the end rows
void Chess::Promotion(int index, string &state, bool real) {
    //returns after doing nothing if call is invalid
    if(index >= 0 && index <= 7) { //is black piece getting promoted
        if(state[index] == 'p') {
            state[index] = 'q';
        } else {
            return; //end function if not a pawn
        }
        
    } else if(index >= 57 && index <= 64) { //is white pawn getting promoted
        if(state[index] == 'P') {
            state[index] = 'Q';
        } else {return;}        
    } else {return;} //return if index was invalid, try to avoid making those calls

    if(real) { //only reach this branch if promotion was valid
        BitHolder* square = (BitHolder*) _grid->getSquareByIndex(index);
        Player* p = square->bit()->getOwner();
        Bit* newPiece = PieceForPlayer(p->playerNumber(), 5); //piece for player handles right color
        square->setBit(newPiece);
        newPiece->setPosition(square->getPosition());

    }
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
    //this is called before the move is complete, and dst still holds the old bit
    //because of that can't do the promotion now, endTurn() has to call that, but this can determine if
    //its even nessecary to check and give it the index to check
    ChessSquare* destination = (ChessSquare *)&dst;
    int index = destination->getSquareIndex();
    if((index >= 0 && index <= 7) || (index >= 57 && index <= 64)) {
        endTurn(index);
    } else {
        endTurn(-1);
    }
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

bool Chess::checkForDraw() //statemates not implimented
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

//get an up to date stateString drawn from _grid
std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}


//unused function for setting state string from an .ini file to contuine game after closing program
void Chess::setStateString(const std::string &s) {
    
}

//int bitIndex = _bitboardLookup[state[i]]
//_bitboards[bitIndex] |= 1ULL << i;

//MOVE GENERATION FUNCTIONS

 std::vector<Chess::BitMove> Chess::generateAllMoves(std::string &state, ChessSide side) {
    //Logger *L = Logger::getInstance();
    //L->LogInfo("Generating moves for side " + to_string(side));
    std::vector<BitMove> moves;
    constexpr uint64_t oneBit = 1; //in lecture, these occupancy boards have 0 for where the units are, which I don't get

    //initalize bitboards/reset them to blank
    for(int i = 0; i < NUM_BOARDS; i++) {
        _bitboards[i] = 0LL;
    }

    //no branching bitboard population
    for(int i = 0; i < 64; i++) {
        //_boardLookup takes char as input and returns which board in _bitboard is the occupancy bitboard for the type of piece at i
        int boardIndex = _boardLookup[state[i]]; 
        _bitboards[boardIndex] |= oneBit << i;
    }

    //branching population currently saved as backup
    /**
    for(int i = 0; i < 64; i++) {
        if(state[i] == 'N') {
            _bitboards[WHITE_KNIGHTS] |= oneBit << i;
        }
        else if(state[i] == 'P') {
            _bitboards[WHITE_PAWNS] |= oneBit << i;
        } else if(state[i] == 'R') {
            _bitboards[WHITE_ROOKS] |= oneBit << i;
        } else if(state[i] == 'B') {
             _bitboards[WHITE_BISHOPS] |= oneBit << i;
        } else if(state[i] == 'Q') {
            _bitboards[WHITE_QUEEN] |= oneBit << i;
        } else if(state[i] == 'K') {
            _bitboards[WHITE_KING] |= oneBit << i;
        } else if(state[i] == 'n') {
            _bitboards[BLACK_KNIGHTS] |= oneBit << i;
        } else if(state[i] == 'p') {
            _bitboards[BLACK_PAWNS] |= oneBit << i;
        } else if(state[i] == 'r') {
            _bitboards[BLACK_ROOKS] |= oneBit << i;
        } else if(state[i] == 'b') {
            _bitboards[BLACK_BISHOPS]|= oneBit << i;
        } else if(state[i] == 'q') {
            _bitboards[BLACK_QUEEN] |= oneBit << i;
        } else if(state[i] == 'k') {
            _bitboards[BLACK_KING] |= oneBit << i;
        }
        
    }
    **/

    //cout << "after turn " << _turns.size() << " black king bitboard: " << blackKing;
    _bitboards[ALL_WHITE] = _bitboards[WHITE_PAWNS].getData() | _bitboards[WHITE_KNIGHTS].getData() | _bitboards[WHITE_ROOKS].getData() | _bitboards[WHITE_BISHOPS].getData() | _bitboards[WHITE_QUEEN].getData() | _bitboards[WHITE_KING].getData();
    _bitboards[ALL_BLACK] = _bitboards[BLACK_PAWNS].getData() | _bitboards[BLACK_KNIGHTS].getData() | _bitboards[BLACK_ROOKS].getData() | _bitboards[BLACK_BISHOPS].getData() | _bitboards[BLACK_QUEEN].getData() | _bitboards[BLACK_KING].getData();
    _bitboards[OCCUPANCY] = _bitboards[ALL_WHITE].getData() | _bitboards[ALL_BLACK].getData();
    //_bitboards[EMPTY] = ~_bitboards[OCCUPANCY].getData(); //EMPTY is populated by state string loop
    //can't think of a way to avoid branching, but I do only generate half as many moves now
    if(side == White) {
        generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS], ~_bitboards[ALL_WHITE].getData()); //only send spaces that are occupied by allies
        generateKingMoves(moves, _bitboards[WHITE_KING], ~_bitboards[ALL_WHITE].getData());
        generatePawnMoves(moves, _bitboards[WHITE_PAWNS], _bitboards[ALL_WHITE], _bitboards[ALL_BLACK], White);
        GenerateBishopMoves(moves, _bitboards[WHITE_BISHOPS], _bitboards[OCCUPANCY], _bitboards[ALL_WHITE]);
        GenerateRookMoves(moves, _bitboards[WHITE_ROOKS], _bitboards[OCCUPANCY], _bitboards[ALL_WHITE]);
        GenerateQueenMoves(moves, _bitboards[WHITE_QUEEN], _bitboards[OCCUPANCY], _bitboards[ALL_WHITE]);
    } else if(side == Black) {
        generateKnightMoves(moves, _bitboards[BLACK_KNIGHTS], ~_bitboards[ALL_BLACK].getData());
        generateKingMoves(moves, _bitboards[BLACK_KING], ~_bitboards[ALL_BLACK].getData());
        generatePawnMoves(moves, _bitboards[BLACK_PAWNS], _bitboards[ALL_BLACK], _bitboards[ALL_WHITE], Black);
        GenerateBishopMoves(moves, _bitboards[BLACK_BISHOPS], _bitboards[OCCUPANCY], _bitboards[ALL_BLACK]);
        GenerateRookMoves(moves, _bitboards[BLACK_ROOKS], _bitboards[OCCUPANCY], _bitboards[ALL_BLACK]); 
        GenerateQueenMoves(moves, _bitboards[BLACK_QUEEN], _bitboards[OCCUPANCY], _bitboards[ALL_BLACK]);
    }
        
    //generate pawn is different wanting occupancy rather than free spaces

   


    return moves; 
}

//way it does it: array of bitboards and enum for their indexes. int _bitboardLookup[128]
//changes let you make bitboards w/o branching
//still need MagicBitboards.h
//_bitboardKoop[''] = ENUM
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
            //before I emplace I need to check if the proposed capture is on my side
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

//generates all the valid moves for sliding pieces based on pre-computation magic
void Chess::GenerateBishopMoves(std::vector<BitMove>&moves, BitboardElement bishops, BitboardElement occupied, BitboardElement allies) {
    bishops.forEachBit([&](int fromSquare) {
        BitboardElement attacks = getBishopAttacks(fromSquare, occupied.getData()); //pretty sure getBishopAttacks returns an uint64 so easy enough to turn into a Bitboard
        //remove all attacks that are places where allies are. getAttacks calcuates for blocks, not if the piece itself is a valid capture
        BitboardElement valid_attacks = attacks.getData() & ~allies.getData();
        valid_attacks.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Bishop);
       });
    });
}

void Chess::GenerateRookMoves(std::vector<BitMove>&moves, BitboardElement rooks, BitboardElement occupied, BitboardElement allies) {
    rooks.forEachBit([&](int fromSquare) {
        BitboardElement attacks = getRookAttacks(fromSquare, occupied.getData()); //pretty sure getBishopAttacks returns an uint64 so easy enough to turn into a Bitboard
        //remove all attacks that are places where allies are. getAttacks calcuates for blocks, not if the piece itself is a valid capture
        BitboardElement valid_attacks = attacks.getData() & ~allies.getData();
        valid_attacks.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Rook);
       });
    });
}

void Chess::GenerateQueenMoves(std::vector<BitMove>&moves, BitboardElement queen, BitboardElement occupied, BitboardElement allies) {
    queen.forEachBit([&](int fromSquare) {
        BitboardElement attacks = getQueenAttacks(fromSquare, occupied.getData()); //pretty sure getBishopAttacks returns an uint64 so easy enough to turn into a Bitboard
        //remove all attacks that are places where allies are. getAttacks calcuates for blocks, not if the piece itself is a valid capture
        BitboardElement valid_attacks = attacks.getData() & ~allies.getData();
        valid_attacks.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Queen);
       });
    });
}

//AI FUNCTIONS------
//assume moves are legal and not handle check
//copied from Connect4 AI
void Chess::updateAI() {
    _movesChecked = 0; //reset movesChecked for fresh AI call
    int bestVal = negInfinite;
    BitMove bestMove;
    std::string state = stateString();
    Logger *L = Logger::getInstance();
    L->LogInfo("F10 is thinking...");
    //evaluate all moves
    for(auto move : _moves) { //use _moves, as when it became AI's turn, should have updated the variable to be of its side.
        int srcSquare = move.from;
        int dstSquare = move.to;
        //save old data of the move
        char oldPiece = state[dstSquare];
        char movingPiece = state[srcSquare];
        //preform the move
        state[dstSquare] = movingPiece;
        state[srcSquare] = '0';
        int moveVal = -negamax(state, 0, -(_AI_COLOR), negInfinite, posInfinite); //run negamax starting with the other player's color
        //that's why you want the - answer
        //undo the move
        state[dstSquare] = oldPiece;
        state[srcSquare] = movingPiece;
        //if evaluated move is the best, save it and update value
        if (moveVal > bestVal) {
                L->LogInfo("new best move " + to_string(move.from) + " to " + to_string(move.to) + ". With score " + to_string(moveVal));
                bestMove = move;
                bestVal = moveVal;
        }
        _movesChecked++;
    }

    //preform the move
    if(bestVal != negInfinite) {
        L->LogGameEvent(to_string(_movesChecked) + " moves evaluated. Making move...");
        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
        BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);
        Bit* bit = src.bit();
        //actual moving
        dst.dropBitAtPoint(bit, ImVec2(0, 0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    } else {
        L->LogError("Didn't get a move better than negInfinite somehow.");
    }
}


//
// player is the current player's number (AI or human)
//
int Chess::negamax(std::string& state, int depth, int playerColor, int alpha, int beta) 
{
    //Logger *L = Logger::getInstance();
    //L->LogInfo("negamax called at depth " + to_string(depth) + " with color " + to_string(playerColor));
    //there is no draw state
    if(state.npos)
    //only evaluate at bottom depth
    if(depth >= _gameOptions.AIMAXDepth) {
        int score = evaluateBoard(state) * playerColor; //playerColor will be 1 (white) or -1 (black)
        return score; //passing score up the recursion will ensure it has the right sign once it reaches updateAI
    }
    //if one side is down a king, also eval and return
    if((state.find('K') == string::npos) || (state.find('k') == string::npos)) {
        int score = evaluateBoard(state) * playerColor;
        return score;
    }
    
    int bestVal = negInfinite;
    //only need to evaluate per free column
    //L->LogInfo("calling generate all moves as negamax.");
    vector<BitMove> moves = generateAllMoves(state, (ChessSide) playerColor);
    //evaluate all moves
    //evaluate all moves
    for(auto move : moves) {
        int srcSquare = move.from;
        int dstSquare = move.to;
        //save old data of the move
        char oldPiece = state[dstSquare];
        char movingPiece = state[srcSquare];
        //preform the move
        state[dstSquare] = movingPiece;
        state[srcSquare] = '0';
        int score = -negamax(state, depth + 1, -playerColor, -beta, -alpha);
        //undo the move
        state[dstSquare] = oldPiece;
        state[srcSquare] = movingPiece;
        _movesChecked++;
        //alpha beta pruning
        if(score > bestVal) {
            bestVal = score;
        }
        if(bestVal > alpha) {
            alpha = bestVal;
        }
        if(alpha >= beta){
            break;
        }
    }
    return bestVal;
}



//simple eval function
//very similar to bitboard lookup
//if black wants score from their prespective, multiypling it by -1
int Chess::evaluateBoard(std::string state) {
    int values[128];
    values['P'] = 100;
    values['N'] = 300;
    values['B'] = 400;
    values['R'] = 500;
    values['Q'] = 900;
    values['K'] = 2000;
    values['p'] = -100;
    values['n'] = -300;
    values['b'] = -400;
    values['r'] = -500;
    values['q'] = -900;
    values['k'] = -2000;
    values['0'] = 0;

    int score = 0;
    for(char ch: state) {
        score += values[ch];
    }

    return score;
}

//release build is faster
