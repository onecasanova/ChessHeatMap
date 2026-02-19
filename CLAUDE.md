# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

C++ two-player chess game with SFML graphical interface. Drag-and-drop piece movement, full legal move generation, and complete chess rules (castling, en passant, promotion, check/checkmate/stalemate detection).

## Build Commands

```bash
make        # compile
make clean  # remove binary
```

Requires SFML installed (`brew install sfml` on macOS).

## Running

```bash
./chess     # run from project root (so assets/ path resolves)
```

## Project Structure

```
ChessEngine/
  assets/
    white_pieces/  (6 PNGs: white-pawn.png, white-knight.png, etc.)
    black_pieces/  (6 PNGs: black-pawn.png, black-knight.png, etc.)
  src/
    chess.cpp      (entire game — single file, 4 classes)
  Makefile
  CLAUDE.md
```

## Architecture — Single File (`src/chess.cpp`)

1. **Piece enum + helpers** — `EMPTY=0`, `W_PAWN=1`..`W_QUEEN=6`, `B_PAWN=7`..`B_QUEEN=12`. `Color` enum, `pieceColor()`, `isWhite()`, `isBlack()`.
2. **Board class** — Game state and rules. `std::array<std::array<Piece,8>,8> squares` where `squares[0][*]` = rank 1 (white back rank), col 0 = a-file. Tracks castling rights, en passant, side to move. Has per-piece move generation, legal move filtering (copy-make + check test), `isSquareAttackedBy()`, `makeMove()`, checkmate/stalemate detection.
3. **Renderer class** — SFML drawing. Loads 12 textures indexed by Piece enum. Renders board with row 0 at screen bottom. Visual feedback: yellow highlight on selected square, green dots on legal targets, red overlay on king in check. Status bar shows turn/check/result.
4. **Game class** — Owns window, board, renderer. Drag-and-drop: mouse press picks up piece, mouse move drags it, mouse release drops on legal square or snaps back. Turn enforcement, game-over disabling.
5. **`main()`** — Creates Game and calls `run()`.

## Key Design Details

- **Coordinate system**: `squares[row][col]` — row 0 = rank 1 (white), row 7 = rank 8 (black). Col 0 = a-file.
- **Legal move filtering**: Copy board, apply move, check if own king still safe. Simple and correct for human play speed.
- **Auto-promotion**: Pawns promote to queen automatically.
- **Dependencies**: SFML for graphics, standard C++ library for everything else.
