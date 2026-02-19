#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// ============================================================================
// Piece Enum + Helpers
// ============================================================================

enum Piece {
    EMPTY    = 0,
    W_PAWN   = 1,
    W_KNIGHT = 2,
    W_BISHOP = 3,
    W_ROOK   = 4,
    W_KING   = 5,
    W_QUEEN  = 6,
    B_PAWN   = 7,
    B_KNIGHT = 8,
    B_BISHOP = 9,
    B_ROOK   = 10,
    B_KING   = 11,
    B_QUEEN  = 12
};

enum Color { WHITE, BLACK, NONE };

Color pieceColor(Piece p) {
    if (p >= W_PAWN && p <= W_QUEEN) return WHITE;
    if (p >= B_PAWN && p <= B_QUEEN) return BLACK;
    return NONE;
}

bool isWhite(Piece p) { return pieceColor(p) == WHITE; }
bool isBlack(Piece p) { return pieceColor(p) == BLACK; }

struct Move {
    int fromRow, fromCol;
    int toRow, toCol;
};

// ============================================================================
// Board Class — Game state and rules
// ============================================================================

class Board {
public:
    std::array<std::array<Piece, 8>, 8> squares;
    Color sideToMove;
    bool castleWK, castleWQ, castleBK, castleBQ;
    int enPassantCol; // -1 if none
    bool gameOver;
    std::string resultText;

    Board() { reset(); }

    void reset() {
        for (auto& row : squares)
            row.fill(EMPTY);

        squares[0][0] = W_ROOK;   squares[0][1] = W_KNIGHT;
        squares[0][2] = W_BISHOP; squares[0][3] = W_QUEEN;
        squares[0][4] = W_KING;   squares[0][5] = W_BISHOP;
        squares[0][6] = W_KNIGHT; squares[0][7] = W_ROOK;
        for (int c = 0; c < 8; c++) squares[1][c] = W_PAWN;

        squares[7][0] = B_ROOK;   squares[7][1] = B_KNIGHT;
        squares[7][2] = B_BISHOP; squares[7][3] = B_QUEEN;
        squares[7][4] = B_KING;   squares[7][5] = B_BISHOP;
        squares[7][6] = B_KNIGHT; squares[7][7] = B_ROOK;
        for (int c = 0; c < 8; c++) squares[6][c] = B_PAWN;

        sideToMove = WHITE;
        castleWK = castleWQ = castleBK = castleBQ = true;
        enPassantCol = -1;
        gameOver = false;
        resultText = "";
    }

    bool inBounds(int r, int c) const {
        return r >= 0 && r < 8 && c >= 0 && c < 8;
    }

    std::pair<int,int> findKing(Color col) const {
        Piece king = (col == WHITE) ? W_KING : B_KING;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (squares[r][c] == king) return {r, c};
        return {-1, -1};
    }

    bool isSquareAttackedBy(int r, int c, Color attacker) const {
        // Knight attacks
        int knightDr[] = {-2,-2,-1,-1,1,1,2,2};
        int knightDc[] = {-1,1,-2,2,-2,2,-1,1};
        Piece enemyKnight = (attacker == WHITE) ? W_KNIGHT : B_KNIGHT;
        for (int i = 0; i < 8; i++) {
            int nr = r + knightDr[i], nc = c + knightDc[i];
            if (inBounds(nr, nc) && squares[nr][nc] == enemyKnight)
                return true;
        }

        // Pawn attacks
        Piece enemyPawn = (attacker == WHITE) ? W_PAWN : B_PAWN;
        int pawnDir = (attacker == WHITE) ? -1 : 1;
        for (int dc : {-1, 1}) {
            int nr = r + pawnDir, nc = c + dc;
            if (inBounds(nr, nc) && squares[nr][nc] == enemyPawn)
                return true;
        }

        // King attacks
        Piece enemyKing = (attacker == WHITE) ? W_KING : B_KING;
        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                int nr = r + dr, nc = c + dc;
                if (inBounds(nr, nc) && squares[nr][nc] == enemyKing)
                    return true;
            }

        // Rook/Queen along ranks and files
        Piece enemyRook  = (attacker == WHITE) ? W_ROOK  : B_ROOK;
        Piece enemyQueen = (attacker == WHITE) ? W_QUEEN : B_QUEEN;
        int straightDr[] = {-1,1,0,0};
        int straightDc[] = {0,0,-1,1};
        for (int d = 0; d < 4; d++) {
            for (int step = 1; step < 8; step++) {
                int nr = r + straightDr[d]*step;
                int nc = c + straightDc[d]*step;
                if (!inBounds(nr, nc)) break;
                Piece p = squares[nr][nc];
                if (p != EMPTY) {
                    if (p == enemyRook || p == enemyQueen) return true;
                    break;
                }
            }
        }

        // Bishop/Queen along diagonals
        Piece enemyBishop = (attacker == WHITE) ? W_BISHOP : B_BISHOP;
        int diagDr[] = {-1,-1,1,1};
        int diagDc[] = {-1,1,-1,1};
        for (int d = 0; d < 4; d++) {
            for (int step = 1; step < 8; step++) {
                int nr = r + diagDr[d]*step;
                int nc = c + diagDc[d]*step;
                if (!inBounds(nr, nc)) break;
                Piece p = squares[nr][nc];
                if (p != EMPTY) {
                    if (p == enemyBishop || p == enemyQueen) return true;
                    break;
                }
            }
        }

        return false;
    }

    bool isInCheck(Color col) const {
        auto [kr, kc] = findKing(col);
        Color enemy = (col == WHITE) ? BLACK : WHITE;
        return isSquareAttackedBy(kr, kc, enemy);
    }

    void generatePieceMoves(int r, int c, std::vector<Move>& moves) const {
        Piece p = squares[r][c];
        Color col = pieceColor(p);
        if (col == NONE) return;

        auto addIfValid = [&](int tr, int tc) {
            if (!inBounds(tr, tc)) return;
            Piece target = squares[tr][tc];
            if (target != EMPTY && pieceColor(target) == col) return;
            moves.push_back({r, c, tr, tc});
        };

        auto addSliding = [&](int dr, int dc) {
            for (int step = 1; step < 8; step++) {
                int nr = r + dr*step, nc = c + dc*step;
                if (!inBounds(nr, nc)) break;
                Piece target = squares[nr][nc];
                if (target != EMPTY) {
                    if (pieceColor(target) != col)
                        moves.push_back({r, c, nr, nc});
                    break;
                }
                moves.push_back({r, c, nr, nc});
            }
        };

        switch (p) {
        case W_PAWN: {
            if (inBounds(r+1, c) && squares[r+1][c] == EMPTY) {
                moves.push_back({r, c, r+1, c});
                if (r == 1 && squares[r+2][c] == EMPTY)
                    moves.push_back({r, c, r+2, c});
            }
            for (int dc : {-1, 1}) {
                int nc = c + dc;
                if (!inBounds(r+1, nc)) continue;
                if (squares[r+1][nc] != EMPTY && isBlack(squares[r+1][nc]))
                    moves.push_back({r, c, r+1, nc});
                if (r == 4 && nc == enPassantCol && squares[r+1][nc] == EMPTY)
                    moves.push_back({r, c, r+1, nc});
            }
            break;
        }
        case B_PAWN: {
            if (inBounds(r-1, c) && squares[r-1][c] == EMPTY) {
                moves.push_back({r, c, r-1, c});
                if (r == 6 && squares[r-2][c] == EMPTY)
                    moves.push_back({r, c, r-2, c});
            }
            for (int dc : {-1, 1}) {
                int nc = c + dc;
                if (!inBounds(r-1, nc)) continue;
                if (squares[r-1][nc] != EMPTY && isWhite(squares[r-1][nc]))
                    moves.push_back({r, c, r-1, nc});
                if (r == 3 && nc == enPassantCol && squares[r-1][nc] == EMPTY)
                    moves.push_back({r, c, r-1, nc});
            }
            break;
        }
        case W_KNIGHT: case B_KNIGHT: {
            int dr[] = {-2,-2,-1,-1,1,1,2,2};
            int dc[] = {-1,1,-2,2,-2,2,-1,1};
            for (int i = 0; i < 8; i++) addIfValid(r+dr[i], c+dc[i]);
            break;
        }
        case W_BISHOP: case B_BISHOP: {
            for (auto [dr,dc] : std::vector<std::pair<int,int>>{{-1,-1},{-1,1},{1,-1},{1,1}})
                addSliding(dr, dc);
            break;
        }
        case W_ROOK: case B_ROOK: {
            for (auto [dr,dc] : std::vector<std::pair<int,int>>{{-1,0},{1,0},{0,-1},{0,1}})
                addSliding(dr, dc);
            break;
        }
        case W_QUEEN: case B_QUEEN: {
            for (auto [dr,dc] : std::vector<std::pair<int,int>>{{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}})
                addSliding(dr, dc);
            break;
        }
        case W_KING: case B_KING: {
            for (int dr = -1; dr <= 1; dr++)
                for (int dc = -1; dc <= 1; dc++) {
                    if (dr == 0 && dc == 0) continue;
                    addIfValid(r+dr, c+dc);
                }
            // Castling
            if (col == WHITE && r == 0 && c == 4 && !isInCheck(WHITE)) {
                if (castleWK && squares[0][5] == EMPTY && squares[0][6] == EMPTY
                    && squares[0][7] == W_ROOK
                    && !isSquareAttackedBy(0, 5, BLACK)
                    && !isSquareAttackedBy(0, 6, BLACK))
                    moves.push_back({0, 4, 0, 6});
                if (castleWQ && squares[0][3] == EMPTY && squares[0][2] == EMPTY
                    && squares[0][1] == EMPTY && squares[0][0] == W_ROOK
                    && !isSquareAttackedBy(0, 3, BLACK)
                    && !isSquareAttackedBy(0, 2, BLACK))
                    moves.push_back({0, 4, 0, 2});
            }
            if (col == BLACK && r == 7 && c == 4 && !isInCheck(BLACK)) {
                if (castleBK && squares[7][5] == EMPTY && squares[7][6] == EMPTY
                    && squares[7][7] == B_ROOK
                    && !isSquareAttackedBy(7, 5, WHITE)
                    && !isSquareAttackedBy(7, 6, WHITE))
                    moves.push_back({7, 4, 7, 6});
                if (castleBQ && squares[7][3] == EMPTY && squares[7][2] == EMPTY
                    && squares[7][1] == EMPTY && squares[7][0] == B_ROOK
                    && !isSquareAttackedBy(7, 3, WHITE)
                    && !isSquareAttackedBy(7, 2, WHITE))
                    moves.push_back({7, 4, 7, 2});
            }
            break;
        }
        default: break;
        }
    }

    std::vector<Move> getLegalMoves(int r, int c) const {
        std::vector<Move> pseudo;
        generatePieceMoves(r, c, pseudo);

        std::vector<Move> legal;
        Color col = pieceColor(squares[r][c]);
        for (auto& m : pseudo) {
            Board copy = *this;
            copy.applyMoveRaw(m);
            if (!copy.isInCheck(col))
                legal.push_back(m);
        }
        return legal;
    }

    std::vector<Move> getAllLegalMoves() const {
        std::vector<Move> all;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (pieceColor(squares[r][c]) == sideToMove) {
                    auto moves = getLegalMoves(r, c);
                    all.insert(all.end(), moves.begin(), moves.end());
                }
        return all;
    }

    void applyMoveRaw(const Move& m) {
        Piece p = squares[m.fromRow][m.fromCol];

        // En passant capture
        if ((p == W_PAWN || p == B_PAWN) && m.fromCol != m.toCol
            && squares[m.toRow][m.toCol] == EMPTY) {
            squares[m.fromRow][m.toCol] = EMPTY;
        }

        // Castling — move the rook
        if ((p == W_KING || p == B_KING) && std::abs(m.toCol - m.fromCol) == 2) {
            int row = m.fromRow;
            if (m.toCol == 6) {
                squares[row][5] = squares[row][7];
                squares[row][7] = EMPTY;
            } else {
                squares[row][3] = squares[row][0];
                squares[row][0] = EMPTY;
            }
        }

        squares[m.toRow][m.toCol] = p;
        squares[m.fromRow][m.fromCol] = EMPTY;

        // Promotion (auto-queen)
        if (p == W_PAWN && m.toRow == 7)
            squares[m.toRow][m.toCol] = W_QUEEN;
        if (p == B_PAWN && m.toRow == 0)
            squares[m.toRow][m.toCol] = B_QUEEN;
    }

    void makeMove(const Move& m) {
        Piece p = squares[m.fromRow][m.fromCol];
        applyMoveRaw(m);

        // Update en passant
        enPassantCol = -1;
        if (p == W_PAWN && m.toRow - m.fromRow == 2)
            enPassantCol = m.fromCol;
        if (p == B_PAWN && m.fromRow - m.toRow == 2)
            enPassantCol = m.fromCol;

        // Update castling rights
        if (p == W_KING)   { castleWK = false; castleWQ = false; }
        if (p == B_KING)   { castleBK = false; castleBQ = false; }
        if (p == W_ROOK && m.fromRow == 0 && m.fromCol == 0) castleWQ = false;
        if (p == W_ROOK && m.fromRow == 0 && m.fromCol == 7) castleWK = false;
        if (p == B_ROOK && m.fromRow == 7 && m.fromCol == 0) castleBQ = false;
        if (p == B_ROOK && m.fromRow == 7 && m.fromCol == 7) castleBK = false;

        // If a rook is captured on its starting square
        if (m.toRow == 0 && m.toCol == 0) castleWQ = false;
        if (m.toRow == 0 && m.toCol == 7) castleWK = false;
        if (m.toRow == 7 && m.toCol == 0) castleBQ = false;
        if (m.toRow == 7 && m.toCol == 7) castleBK = false;

        sideToMove = (sideToMove == WHITE) ? BLACK : WHITE;

        if (getAllLegalMoves().empty()) {
            gameOver = true;
            if (isInCheck(sideToMove)) {
                resultText = (sideToMove == WHITE) ? "Black wins by checkmate!"
                                                   : "White wins by checkmate!";
            } else {
                resultText = "Stalemate — draw!";
            }
        }
    }
};

// ============================================================================
// Renderer Class — All SFML drawing (SFML 3.x API)
// ============================================================================

class Renderer {
public:
    static constexpr float TILE_SIZE = 80.f;
    static constexpr float BOARD_PX = TILE_SIZE * 8;
    static constexpr float STATUS_HEIGHT = 40.f;

    sf::Texture textures[13]; // indexed by Piece enum
    std::optional<sf::Font> font;

    bool loadAssets() {
        struct TexInfo { Piece piece; std::string path; };
        TexInfo infos[] = {
            {W_PAWN,   "assets/white_pieces/white-pawn.png"},
            {W_KNIGHT, "assets/white_pieces/white-knight.png"},
            {W_BISHOP, "assets/white_pieces/white-bishop.png"},
            {W_ROOK,   "assets/white_pieces/white-rook.png"},
            {W_KING,   "assets/white_pieces/white-king.png"},
            {W_QUEEN,  "assets/white_pieces/white-queen.png"},
            {B_PAWN,   "assets/black_pieces/black-pawn.png"},
            {B_KNIGHT, "assets/black_pieces/black-knight.png"},
            {B_BISHOP, "assets/black_pieces/black-bishop.png"},
            {B_ROOK,   "assets/black_pieces/black-rook.png"},
            {B_KING,   "assets/black_pieces/black-king.png"},
            {B_QUEEN,  "assets/black_pieces/black-queen.png"},
        };
        for (auto& info : infos) {
            if (!textures[info.piece].loadFromFile(info.path)) {
                std::cerr << "Failed to load " << info.path << std::endl;
                return false;
            }
        }
        // Try loading a system font for the status bar
        const char* fontPaths[] = {
            "/System/Library/Fonts/Helvetica.ttc",
            "/System/Library/Fonts/SFNSMono.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "C:/Windows/Fonts/arial.ttf"
        };
        for (auto* fp : fontPaths) {
            sf::Font tryFont;
            if (tryFont.openFromFile(fp)) {
                font = std::move(tryFont);
                break;
            }
        }
        return true;
    }

    float rowToY(int row) const { return (7 - row) * TILE_SIZE; }
    float colToX(int col) const { return col * TILE_SIZE; }
    int yToRow(float y) const { return 7 - static_cast<int>(y / TILE_SIZE); }
    int xToCol(float x) const { return static_cast<int>(x / TILE_SIZE); }

    void drawBoard(sf::RenderWindow& window) {
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++) {
                sf::RectangleShape sq({TILE_SIZE, TILE_SIZE});
                sq.setPosition({colToX(col), rowToY(row)});
                bool light = (row + col) % 2 == 0;
                sq.setFillColor(light ? sf::Color(240, 217, 181)
                                      : sf::Color(181, 136, 99));
                window.draw(sq);
            }
    }

    void drawHighlight(sf::RenderWindow& window, int row, int col,
                       sf::Color color) {
        sf::RectangleShape sq({TILE_SIZE, TILE_SIZE});
        sq.setPosition({colToX(col), rowToY(row)});
        sq.setFillColor(color);
        window.draw(sq);
    }

    void drawLegalDot(sf::RenderWindow& window, int row, int col,
                      const Board& board) {
        float cx = colToX(col) + TILE_SIZE / 2;
        float cy = rowToY(row) + TILE_SIZE / 2;
        if (board.squares[row][col] != EMPTY) {
            sf::CircleShape ring(TILE_SIZE / 2 - 4);
            ring.setPosition({colToX(col) + 4, rowToY(row) + 4});
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(4);
            ring.setOutlineColor(sf::Color(0, 0, 0, 80));
            window.draw(ring);
        } else {
            sf::CircleShape dot(10);
            dot.setPosition({cx - 10, cy - 10});
            dot.setFillColor(sf::Color(0, 0, 0, 80));
            window.draw(dot);
        }
    }

    void drawPiece(sf::RenderWindow& window, Piece p, float x, float y) {
        if (p == EMPTY) return;
        sf::Sprite sprite(textures[p]);
        auto sz = textures[p].getSize();
        sprite.setScale({TILE_SIZE / sz.x, TILE_SIZE / sz.y});
        sprite.setPosition({x, y});
        window.draw(sprite);
    }

    void drawPieces(sf::RenderWindow& window, const Board& board,
                    int skipRow = -1, int skipCol = -1) {
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++) {
                if (row == skipRow && col == skipCol) continue;
                drawPiece(window, board.squares[row][col],
                          colToX(col), rowToY(row));
            }
    }

    void drawStatusBar(sf::RenderWindow& window, const Board& board) {
        sf::RectangleShape bar({BOARD_PX, STATUS_HEIGHT});
        bar.setPosition({0, BOARD_PX});
        bar.setFillColor(sf::Color(50, 50, 50));
        window.draw(bar);

        if (!font) return;

        std::string text;
        if (board.gameOver) {
            text = board.resultText;
        } else {
            text = (board.sideToMove == WHITE) ? "White to move" : "Black to move";
            if (board.isInCheck(board.sideToMove))
                text += " -- CHECK!";
        }
        sf::Text label(*font, text, 20);
        label.setPosition({10.f, BOARD_PX + 8.f});
        label.setFillColor(sf::Color::White);
        window.draw(label);
    }
};

// ============================================================================
// Game Class — Input handling + main loop (SFML 3.x event API)
// ============================================================================

class Game {
public:
    sf::RenderWindow window;
    Board board;
    Renderer renderer;

    bool dragging;
    int selRow, selCol;
    float dragX, dragY;
    std::vector<Move> legalFromSelected;

    Game() : window(sf::VideoMode({static_cast<unsigned>(Renderer::BOARD_PX),
                                    static_cast<unsigned>(Renderer::BOARD_PX + Renderer::STATUS_HEIGHT)}),
                    "Chess"),
             dragging(false), selRow(-1), selCol(-1), dragX(0), dragY(0) {}

    bool init() { return renderer.loadAssets(); }

    void run() {
        while (window.isOpen()) {
            handleEvents();
            render();
        }
    }

private:
    void handleEvents() {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return;
            }
            if (board.gameOver) continue;

            if (const auto* mp = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mp->button == sf::Mouse::Button::Left)
                    onMousePress(mp->position.x, mp->position.y);
            }
            if (const auto* mm = event->getIf<sf::Event::MouseMoved>()) {
                if (dragging) {
                    dragX = mm->position.x - Renderer::TILE_SIZE / 2;
                    dragY = mm->position.y - Renderer::TILE_SIZE / 2;
                }
            }
            if (const auto* mr = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mr->button == sf::Mouse::Button::Left && dragging)
                    onMouseRelease(mr->position.x, mr->position.y);
            }
        }
    }

    void onMousePress(int mx, int my) {
        int col = renderer.xToCol(static_cast<float>(mx));
        int row = renderer.yToRow(static_cast<float>(my));
        if (!board.inBounds(row, col)) return;

        Piece p = board.squares[row][col];
        if (pieceColor(p) != board.sideToMove) return;

        selRow = row;
        selCol = col;
        dragging = true;
        dragX = mx - Renderer::TILE_SIZE / 2;
        dragY = my - Renderer::TILE_SIZE / 2;
        legalFromSelected = board.getLegalMoves(row, col);
    }

    void onMouseRelease(int mx, int my) {
        int col = renderer.xToCol(static_cast<float>(mx));
        int row = renderer.yToRow(static_cast<float>(my));

        for (auto& m : legalFromSelected) {
            if (m.toRow == row && m.toCol == col) {
                board.makeMove(m);
                break;
            }
        }

        dragging = false;
        selRow = selCol = -1;
        legalFromSelected.clear();
    }

    void render() {
        window.clear();

        renderer.drawBoard(window);

        if (selRow >= 0)
            renderer.drawHighlight(window, selRow, selCol,
                                   sf::Color(255, 255, 0, 100));

        if (!board.gameOver && board.isInCheck(board.sideToMove)) {
            auto [kr, kc] = board.findKing(board.sideToMove);
            renderer.drawHighlight(window, kr, kc,
                                   sf::Color(255, 0, 0, 120));
        }

        for (auto& m : legalFromSelected)
            renderer.drawLegalDot(window, m.toRow, m.toCol, board);

        renderer.drawPieces(window, board,
                            dragging ? selRow : -1,
                            dragging ? selCol : -1);

        if (dragging)
            renderer.drawPiece(window, board.squares[selRow][selCol],
                               dragX, dragY);

        renderer.drawStatusBar(window, board);

        window.display();
    }
};

// ============================================================================
// main
// ============================================================================

int main() {
    Game game;
    if (!game.init()) {
        std::cerr << "Failed to initialize. Run from project root." << std::endl;
        return 1;
    }
    game.run();
    return 0;
}
