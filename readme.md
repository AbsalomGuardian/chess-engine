Absalom Ranelletti Fall 2025 CSE123

Chess Engine program

Current fuctionality:

Create board from FEN string set in source code.

Complete chess engine including basic movement rules and promotion (always to Queen). No check or stalemates.

Has AI that works up to depth 4. AI will currently always play as Black, but code is modularized in a way that changing it will be trival.

GenerateAllMoves only generates the moves for the upcoming side (or side calling it in negamax). For safety purposes, it is still checked if you own a piece in the can move functions. Honestly getting that working, and especially going from player numbers 0 and 1 to 1 and -1 was the hardest part. Other than that it was just cobbling code that already existed together. I was even able to come up with the code using the magicbitboard functions myself instead of copying from Zoom.

Currently uses the simple evaluation function demonstrated in class.

Keeps track of moves checked and prints it to the log before the move the AI has decided upon is made.

Note, by default the FEN string in the source code generates an in-progress game instead of the starting positions.

