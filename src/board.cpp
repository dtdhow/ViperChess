// board.cpp
#include "board.hpp"
#include "magic_bits.hpp" // Contains ROOK_MAGIC_NUMBERS, BISHOP_MAGIC_NUMBERS
#include <sstream>
#include <cctype>
#include <iostream>
#include <bitset>
#include <cmath>

namespace ViperChess {

uint64_t Board::pawn_attack_table[NUM_COLORS][64] = {};
uint64_t Board::knight_attack_table[64] = {};
uint64_t Board::king_attack_table[64] = {};
    
constexpr Square operator+(Square s, int i) { return Square(int(s) + i); }
constexpr Square operator-(Square s, int i) { return Square(int(s) - i); }
Square& operator++(Square& s) { return s = Square(int(s) + 1); }
Square& operator--(Square& s) { return s = Square(int(s) - 1); }
Square operator++(Square& s, int) { Square tmp = s; ++s; return tmp; }
Square operator--(Square& s, int) { Square tmp = s; --s; return tmp; }

// Enum operators for PieceType
constexpr PieceType operator+(PieceType pt, int i) { return PieceType(int(pt) + i); }
constexpr PieceType operator-(PieceType pt, int i) { return PieceType(int(pt) - i); }
PieceType& operator++(PieceType& pt) { return pt = PieceType(int(pt) + 1); }
PieceType& operator--(PieceType& pt) { return pt = PieceType(int(pt) - 1); }
PieceType operator++(PieceType& pt, int) { PieceType tmp = pt; ++pt; return tmp; }
PieceType operator--(PieceType& pt, int) { PieceType tmp = pt; --pt; return tmp; }

// Enum operators for Color
constexpr Color operator+(Color c, int i) { return Color(int(c) + i); }
constexpr Color operator-(Color c, int i) { return Color(int(c) - i); }
Color& operator++(Color& c) { return c = Color(int(c) + 1); }
Color& operator--(Color& c) { return c = Color(int(c) - 1); }
Color operator++(Color& c, int) { Color tmp = c; ++c; return tmp; }
Color operator--(Color& c, int) { Color tmp = c; --c; return tmp; }

// ===== Piece Constants =====
const Piece Piece::NONE     = {NONE_PIECE, ViperChess::Color::NONE};
const Piece Piece::W_PAWN   = {PAWN, WHITE};
const Piece Piece::B_PAWN   = {PAWN, BLACK};
const Piece Piece::W_KNIGHT = {KNIGHT, WHITE};
const Piece Piece::B_KNIGHT = {KNIGHT, BLACK};
const Piece Piece::W_BISHOP = {BISHOP, WHITE};
const Piece Piece::B_BISHOP = {BISHOP, BLACK};
const Piece Piece::W_ROOK   = {ROOK, WHITE};
const Piece Piece::B_ROOK   = {ROOK, BLACK};
const Piece Piece::W_QUEEN  = {QUEEN, WHITE};
const Piece Piece::B_QUEEN  = {QUEEN, BLACK};
const Piece Piece::W_KING   = {KING, WHITE};
const Piece Piece::B_KING   = {KING, BLACK};

// ===== Magic Bitboard Tables =====
Magic rook_magics[64];
Magic bishop_magics[64];
uint64_t rook_attacks[64][4096];
uint64_t bishop_attacks[64][512];

// ===== Board Initialization =====
Board::Board() : m_side_to_move(WHITE) {
    Zobrist::init();
    static bool initialized = false;
    if (!initialized) {
        init_magics();
        init_attack_tables();  // Add this line
        initialized = true;
    }
    // Initialize empty board
    m_squares.fill(Piece::NONE);
    for (auto& color : m_pieces) {
        for (auto& piece : color) {
            piece = 0;
        }
    }
    set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

Square Board::pop_lsb(uint64_t& bb) 
{
        Square sq = Square(__builtin_ctzll(bb));
        bb &= bb - 1; // Clear the least significant bit
        return sq;
}

void Board::set_fen(const std::string& fen) {
    std::istringstream iss(fen);
    std::string token;
    Square sq = A8;

    // Clear the board first
    m_squares.fill(Piece::NONE);
    for (auto& color : m_pieces) {
        for (auto& piece : color) {
            piece = 0;
        }
    }
    m_castling_rights = 0;  // Reset all castling rights
    m_en_passant = NUM_SQUARES; // Indicates no en passant
    m_halfmove_clock = 0;
    m_fullmove_number = 1;

    // 1. Piece placement
    iss >> token;
    for (char c : token) {
        if (c == '/') {
            sq = sq - 16; // Move to the beginning of the next rank
        } else if (isdigit(c)) {
            sq = sq + (c - '0'); // Skip empty squares
        } else {
            Color color = isupper(c) ? WHITE : BLACK;
            PieceType type = char_to_piece(toupper(c));
            m_squares[sq] = {type, color};
            m_pieces[color][type] |= (1ULL << sq);
            ++sq;
        }
    }

    // 2. Active color
    iss >> token;
    m_side_to_move = (token == "w") ? WHITE : BLACK;

    // 3. Castling availability
    iss >> token;
    if (token != "-") {
        for (char c : token) {
            switch (c) {
                case 'K': m_castling_rights |= (1 << WHITE_OO); break;
                case 'Q': m_castling_rights |= (1 << WHITE_OOO); break;
                case 'k': m_castling_rights |= (1 << BLACK_OO); break;
                case 'q': m_castling_rights |= (1 << BLACK_OOO); break;
            }
        }
    }

    // 4. En passant target square
    iss >> token;
    if (token != "-") {
        int file = token[0] - 'a';
        int rank = token[1] - '1';
        m_en_passant = Square(rank * 8 + file);
    }

    // 5. Halfmove clock (for 50-move rule)
    iss >> token;
    m_halfmove_clock = std::stoi(token);

    // 6. Fullmove number (increments after black's move)
    if (iss >> token) {
        m_fullmove_number = std::stoi(token);
    }
}

// ===== Magic Bitboard Initialization =====
void Board::init() {}

uint64_t Board::compute_rook_attacks(Square sq, uint64_t occupancy) {
    uint64_t attacks = 0;
    int r = sq / 8, f = sq % 8;
    
    // North
    for (int i = r + 1; i < 8; ++i) {
        attacks |= (1ULL << (8 * i + f));
        if (occupancy & (1ULL << (8 * i + f))) break;
    }
    // South
    for (int i = r - 1; i >= 0; --i) {
        attacks |= (1ULL << (8 * i + f));
        if (occupancy & (1ULL << (8 * i + f))) break;
    }
    // East
    for (int j = f + 1; j < 8; ++j) {
        attacks |= (1ULL << (8 * r + j));
        if (occupancy & (1ULL << (8 * r + j))) break;
    }
    // West
    for (int j = f - 1; j >= 0; --j) {
        attacks |= (1ULL << (8 * r + j));
        if (occupancy & (1ULL << (8 * r + j))) break;
    }
    
    return attacks;
}

uint64_t Board::compute_bishop_attacks(Square sq, uint64_t occupancy) {
    uint64_t attacks = 0;
    int r = sq / 8, f = sq % 8;
    
    // Northeast
    for (int i = r + 1, j = f + 1; i < 8 && j < 8; ++i, ++j) {
        attacks |= (1ULL << (8 * i + j));
        if (occupancy & (1ULL << (8 * i + j))) break;
    }
    // Southeast
    for (int i = r - 1, j = f + 1; i >= 0 && j < 8; --i, ++j) {
        attacks |= (1ULL << (8 * i + j));
        if (occupancy & (1ULL << (8 * i + j))) break;
    }
    // Southwest
    for (int i = r - 1, j = f - 1; i >= 0 && j >= 0; --i, --j) {
        attacks |= (1ULL << (8 * i + j));
        if (occupancy & (1ULL << (8 * i + j))) break;
    }
    // Northwest
    for (int i = r + 1, j = f - 1; i < 8 && j >= 0; ++i, --j) {
        attacks |= (1ULL << (8 * i + j));
        if (occupancy & (1ULL << (8 * i + j))) break;
    }
    
    return attacks;
}

void Board::init_magics() {
    for (Square sq = A1; sq < NUM_SQUARES; ++sq) {
        // Initialize rook magics
        rook_magics[sq] = {
            rook_mask(sq),
            magic_bits::ROOK_MAGIC_NUMBERS[sq],
            rook_attacks[sq],
            64 - magic_bits::ROOK_INDEX_BITS[sq]
        };

        // Initialize bishop magics
        bishop_magics[sq] = {
            bishop_mask(sq),
            magic_bits::BISHOP_MAGIC_NUMBERS[sq],
            bishop_attacks[sq],
            64 - magic_bits::BISHOP_INDEX_BITS[sq]
        };

        // Precompute attack tables
        init_sliding_attacks(sq, true);  // Rooks
        init_sliding_attacks(sq, false); // Bishops
    }
}

void Board::reset_to_startpos() {
    m_zobrist_key = 0;
    
    // Hash pieces
    for (Square sq = A1; sq < NUM_SQUARES; ++sq) {
        if (m_squares[sq].type != NONE_PIECE) {
            m_zobrist_key ^= Zobrist::piece_keys[m_squares[sq].type][m_squares[sq].color][sq];
        }
    }
    
    // Hash side to move (if black)
    if (m_side_to_move == BLACK) {
        m_zobrist_key ^= Zobrist::side_key;
    }
    
    // Hash castling rights
    m_zobrist_key ^= Zobrist::castling_keys[m_castling_rights];
    
    // EP square (none in starting position)
    m_en_passant = NUM_SQUARES;
}

namespace Zobrist {
    uint64_t piece_keys[NUM_PIECE_TYPES][NUM_COLORS][NUM_SQUARES];
    uint64_t side_key;
    uint64_t castling_keys[16];
    uint64_t ep_keys[NUM_SQUARES];

    void init() {
        std::mt19937_64 rng(123456); // Fixed seed for reproducibility
        
        for (PieceType pt = PAWN; pt <= KING; ++pt)
            for (Color c = WHITE; c <= BLACK; ++c)
                for (Square sq = A1; sq < NUM_SQUARES; ++sq)
                    piece_keys[pt][c][sq] = rng();
        
        side_key = rng();
        for (int i = 0; i < 16; ++i) castling_keys[i] = rng();
        for (Square sq = A1; sq < NUM_SQUARES; ++sq) ep_keys[sq] = rng();
    }
}


// Zobrist key implementation
void Board::update_zobrist_key(Piece piece, Square sq) {
    m_zobrist_key ^= Zobrist::piece_keys[piece.type][piece.color][sq];
}

// Combined bitboards
uint64_t Board::get_white_pieces() const {
    return m_pieces[WHITE][PAWN] | m_pieces[WHITE][KNIGHT] | 
           m_pieces[WHITE][BISHOP] | m_pieces[WHITE][ROOK] | 
           m_pieces[WHITE][QUEEN] | m_pieces[WHITE][KING];
}

uint64_t Board::get_black_pieces() const {
    return m_pieces[BLACK][PAWN] | m_pieces[BLACK][KNIGHT] | 
           m_pieces[BLACK][BISHOP] | m_pieces[BLACK][ROOK] | 
           m_pieces[BLACK][QUEEN] | m_pieces[BLACK][KING];
}

// Individual piece types
uint64_t Board::get_kings() const   { return m_pieces[WHITE][KING] | m_pieces[BLACK][KING]; }
uint64_t Board::get_queens() const  { return m_pieces[WHITE][QUEEN] | m_pieces[BLACK][QUEEN]; }
uint64_t Board::get_rooks() const   { return m_pieces[WHITE][ROOK] | m_pieces[BLACK][ROOK]; }
uint64_t Board::get_bishops() const { return m_pieces[WHITE][BISHOP] | m_pieces[BLACK][BISHOP]; }
uint64_t Board::get_knights() const { return m_pieces[WHITE][KNIGHT] | m_pieces[BLACK][KNIGHT]; }
uint64_t Board::get_pawns() const   { return m_pieces[WHITE][PAWN] | m_pieces[BLACK][PAWN]; }

// Castling rights for Syzygy
int Board::get_castling_rights() const {
    int rights = 0;
    if (m_castling_rights & (1 << WHITE_OO)) rights |= 1;
    if (m_castling_rights & (1 << WHITE_OOO)) rights |= 2;
    if (m_castling_rights & (1 << BLACK_OO)) rights |= 4;
    if (m_castling_rights & (1 << BLACK_OOO)) rights |= 8;
    return rights;
}

void Board::init_sliding_attacks(Square sq, bool is_rook) {
    Magic* m = is_rook ? &rook_magics[sq] : &bishop_magics[sq];
    uint64_t mask = m->mask;
    int bits = count_bits(mask);

    for (int i = 0; i < (1 << bits); ++i) {
        uint64_t occ = index_to_occupancy(i, bits, mask);
        uint64_t attacks = is_rook ? compute_rook_attacks(sq, occ) 
                                  : compute_bishop_attacks(sq, occ);
        uint64_t idx = (occ * m->magic) >> m->shift;
        
        if (is_rook) {
            rook_attacks[sq][idx] = attacks;
        } else {
            bishop_attacks[sq][idx] = attacks;
        }
    }
}

// ===== Attack Generation ====at
uint64_t Board::get_rook_attacks(Square sq, uint64_t occupancy) const {
    const Magic& m = rook_magics[sq];
    occupancy &= m.mask;
    occupancy *= m.magic;
    occupancy >>= m.shift;
    return m.attacks[occupancy];
}

uint64_t Board::get_bishop_attacks(Square sq, uint64_t occupancy) const {
    const Magic& m = bishop_magics[sq];
    occupancy &= m.mask;
    occupancy *= m.magic;
    occupancy >>= m.shift;
    return m.attacks[occupancy];
}

uint64_t Board::get_queen_attacks(Square sq, uint64_t occupancy) const {
    return get_rook_attacks(sq, occupancy) | 
           get_bishop_attacks(sq, occupancy);
}

// ===== Helper Functions =====
uint64_t rook_mask(Square sq) {
    uint64_t mask = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r+1; i <= 6; ++i) mask |= (1ULL << (8*i + f));
    for (int i = r-1; i >= 1; --i) mask |= (1ULL << (8*i + f));
    for (int j = f+1; j <= 6; ++j) mask |= (1ULL << (8*r + j));
    for (int j = f-1; j >= 1; --j) mask |= (1ULL << (8*r + j));
    return mask;
}

uint64_t bishop_mask(Square sq) {
    uint64_t mask = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r+1, j = f+1; i <= 6 && j <= 6; ++i, ++j) mask |= (1ULL << (8*i + j));
    for (int i = r+1, j = f-1; i <= 6 && j >= 1; ++i, --j) mask |= (1ULL << (8*i + j));
    for (int i = r-1, j = f+1; i >= 1 && j <= 6; --i, ++j) mask |= (1ULL << (8*i + j));
    for (int i = r-1, j = f-1; i >= 1 && j >= 1; --i, --j) mask |= (1ULL << (8*i + j));
    return mask;
}

int count_bits(uint64_t b) {
    return __builtin_popcountll(b);
}

uint64_t index_to_occupancy(int index, int bits, uint64_t mask) {
    uint64_t result = 0;
    for (int i = 0; i < bits; ++i) {
        int sq = __builtin_ctzll(mask);
        mask &= mask - 1;
        if (index & (1 << i)) result |= (1ULL << sq);
    }
    return result;
}

void Board::update_zobrist_piece(Piece piece, Square sq) {
    m_zobrist_key ^= Zobrist::piece_keys[piece.type][piece.color][sq];
}

void Board::update_zobrist_side() {
    m_zobrist_key ^= Zobrist::side_key;
}

void Board::update_zobrist_castling() {
    m_zobrist_key ^= Zobrist::castling_keys[m_castling_rights];
}

void Board::update_zobrist_ep(Square ep_sq) {
    if (m_en_passant != NUM_SQUARES)
        m_zobrist_key ^= Zobrist::ep_keys[m_en_passant];
    if (ep_sq != NUM_SQUARES)
        m_zobrist_key ^= Zobrist::ep_keys[ep_sq];
}

void Board::make_move(const Move& move) {
    // Save game state for undo
    // ...

    m_zobrist_key ^= Zobrist::side_key;
    if (m_en_passant != NUM_SQUARES) 
        m_zobrist_key ^= Zobrist::ep_keys[m_en_passant];
    m_zobrist_key ^= Zobrist::castling_keys[m_castling_rights];
    
    // Move the piece
    Piece piece = piece_at(move.from);
    m_squares[move.from] = Piece::NONE;
    m_squares[move.to] = piece;
    
    // Handle captures
    // Handle castling
    // Handle en passant
    // Handle promotion
    
    // Update side to move
    m_side_to_move = opposite_color(m_side_to_move);

    if (m_en_passant != NUM_SQUARES)  // Changed from new_ep to m_en_passant
        m_zobrist_key ^= Zobrist::ep_keys[m_en_passant];
    m_zobrist_key ^= Zobrist::castling_keys[m_castling_rights];
}

void Board::init_attack_tables() {
    for (Square sq = A1; sq < NUM_SQUARES; ++sq) {
        // Pawn attacks
        pawn_attack_table[WHITE][sq] = 0;
        pawn_attack_table[BLACK][sq] = 0;
        
        int file = file_of(sq);
        int rank = rank_of(sq);
        
        // White pawn attacks
        if (rank < 7) {
            if (file > 0) pawn_attack_table[WHITE][sq] |= (1ULL << (sq + 7));
            if (file < 7) pawn_attack_table[WHITE][sq] |= (1ULL << (sq + 9));
        }
        
        // Black pawn attacks
        if (rank > 0) {
            if (file > 0) pawn_attack_table[BLACK][sq] |= (1ULL << (sq - 9));
            if (file < 7) pawn_attack_table[BLACK][sq] |= (1ULL << (sq - 7));
        }
        
        // Knight attacks
        uint64_t knight_bb = 0;
        for (int dx : {-2, -1, 1, 2}) {
            for (int dy : {-2, -1, 1, 2}) {
                if (abs(dx) + abs(dy) == 3) {
                    int file = file_of(sq) + dx;
                    int rank = rank_of(sq) + dy;
                    if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
                        knight_bb |= 1ULL << (rank * 8 + file);
                    }
                }
            }
        }
        knight_attack_table[sq] = knight_bb;
        
        // King attacks
        uint64_t king_bb = 0;
        for (int dx : {-1, 0, 1}) {
            for (int dy : {-1, 0, 1}) {
                if (dx == 0 && dy == 0) continue;
                int file = file_of(sq) + dx;
                int rank = rank_of(sq) + dy;
                if (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
                    king_bb |= 1ULL << (rank * 8 + file);
                }
            }
        }
        king_attack_table[sq] = king_bb;
    }
}

// Returns bitboard of all attackers to a square
uint64_t Board::attackers_to(Square sq, Color by_color) const {
    uint64_t attackers = 0;
    uint64_t occupied = occupancy();
    
    attackers |= pawn_attack_table[opposite_color(by_color)][sq] & m_pieces[by_color][PAWN];
    attackers |= knight_attack_table[sq] & m_pieces[by_color][KNIGHT];
    attackers |= get_bishop_attacks(sq, occupied) & (m_pieces[by_color][BISHOP] | m_pieces[by_color][QUEEN]);
    attackers |= get_rook_attacks(sq, occupied) & (m_pieces[by_color][ROOK] | m_pieces[by_color][QUEEN]);
    attackers |= king_attack_table[sq] & m_pieces[by_color][KING];
    
    return attackers;
}

// Returns bitboard of squares between two squares (for blocking moves)
uint64_t Board::squares_between(Square a, Square b) const {
    const int a_file = file_of(a);
    const int a_rank = rank_of(a);
    const int b_file = file_of(b);
    const int b_rank = rank_of(b);
    
    uint64_t result = 0;
    
    // Same rank (horizontal)
    if (a_rank == b_rank) {
        int start = std::min(a_file, b_file) + 1;
        int end = std::max(a_file, b_file);
        for (int f = start; f < end; f++) {
            result |= 1ULL << (a_rank * 8 + f);
        }
    }
    // Same file (vertical)
    else if (a_file == b_file) {
        int start = std::min(a_rank, b_rank) + 1;
        int end = std::max(a_rank, b_rank);
        for (int r = start; r < end; r++) {
            result |= 1ULL << (r * 8 + a_file);
        }
    }
    // Diagonal (same slope)
    else if (abs(a_file - b_file) == abs(a_rank - b_rank)) {
        int file_step = (b_file > a_file) ? 1 : -1;
        int rank_step = (b_rank > a_rank) ? 1 : -1;
        int f = a_file + file_step;
        int r = a_rank + rank_step;
        
        while (f != b_file && r != b_rank) {
            result |= 1ULL << (r * 8 + f);
            f += file_step;
            r += rank_step;
        }
    }
    
    return result;
}

bool Board::is_square_attacked(Square sq, Color by_color) const {
    Bitboard attackers = 0;
    
    // Pawns
    Bitboard pawn_attacks = pawn_attack_table[by_color][sq];
    if (pawn_attacks & m_pieces[by_color][PAWN]) return true;
    
    // Knights
    Bitboard knight_attacks = knight_attack_table[sq];
    if (knight_attacks & m_pieces[by_color][KNIGHT]) return true;
    
    // Bishops/Queens
    Bitboard bishop_attacks = get_bishop_attacks(sq, occupancy());
    if (bishop_attacks & (m_pieces[by_color][BISHOP] | m_pieces[by_color][QUEEN])) 
        return true;
    
    // Rooks/Queens
    Bitboard rook_attacks = get_rook_attacks(sq, occupancy());
    if (rook_attacks & (m_pieces[by_color][ROOK] | m_pieces[by_color][QUEEN])) 
        return true;
    
    // King
    Bitboard king_attacks = king_attack_table[sq];
    if (king_attacks & m_pieces[by_color][KING]) return true;
    
    return false;
}

bool Board::is_in_check(Color color) const {
    Square king_sq = find_king(color);
    Color opponent = opposite_color(color);
    return is_square_attacked(king_sq, opponent);
}

Color Board::opposite_color(Color color) const {
    return color == WHITE ? BLACK : WHITE;
}

Square Board::find_king(Color color) const {
    Bitboard king_bb = m_pieces[color][KING];
    assert(king_bb != 0 && "No king found for color");
    return pop_lsb(king_bb);
}

bool Board::is_checkmate() const {
    if (!is_in_check(m_side_to_move)) return false;
    return generate_legal_moves().empty();
}

bool Board::is_stalemate() const {
    if (is_in_check(m_side_to_move)) return false;
    return generate_legal_moves().empty();
}

bool Board::is_legal(const Move& move) const {
    // 1. Basic validation
    if (!is_on_board(move.from) || !is_on_board(move.to)) return false;
    
    // 2. Verify piece exists and belongs to current player
    const Piece& piece = piece_at(move.from);
    if (piece.color != m_side_to_move) return false;

    // 3. Special cases (en passant, castling, promotion)
    if (piece.type == PAWN) {
        // Validate en passant
        if (move.to == m_en_passant) {
            //return is_valid_en_passant(move);
        }
        // Validate promotion
        if (move.promotion != NONE_PIECE && 
            !(rank_of(move.to) == 7 || rank_of(move.to) == 0)) {
            return false;
        }
    }
    
    // 4. Simulate the move and check for king safety
    Board temp = *this;
    temp.make_move(move);
    return !temp.is_in_check(m_side_to_move);
}

std::vector<Move> Board::generate_king_moves(Square sq) const {
    std::vector<Move> moves;
    const Piece& king = piece_at(sq);
    uint64_t attacks = king_attack_table[sq];
    
    while (attacks) {
        Square to = pop_lsb(attacks);
        const Piece& target = piece_at(to);
        
        // Can move to empty squares or capture opponent pieces
        if (target.type == NONE_PIECE || target.color != king.color) {
            // Verify destination isn't attacked
            if (!is_square_attacked(to, opposite_color(king.color))) {
                moves.push_back({sq, to});
            }
        }
    }
    
    // Castling - only if not in check
    if (!is_in_check(king.color)) {
        // Kingside castling
        if ((king.color == WHITE && (m_castling_rights & (1 << WHITE_OO))) ||
            (king.color == BLACK && (m_castling_rights & (1 << BLACK_OO)))) {
            Square rook_sq = king.color == WHITE ? H1 : H8;
            Square f = king.color == WHITE ? F1 : F8;
            Square g = king.color == WHITE ? G1 : G8;
            
            if (is_empty(f) && is_empty(g) &&
                !is_square_attacked(f, opposite_color(king.color)) &&
                !is_square_attacked(g, opposite_color(king.color))) {
                moves.push_back({sq, g});
            }
        }
        
        // Queenside castling
        if ((king.color == WHITE && (m_castling_rights & (1 << WHITE_OOO))) ||
            (king.color == BLACK && (m_castling_rights & (1 << BLACK_OOO)))) {
            Square rook_sq = king.color == WHITE ? A1 : A8;
            Square d = king.color == WHITE ? D1 : D8;
            Square c = king.color == WHITE ? C1 : C8;
            Square b = king.color == WHITE ? B1 : B8;
            
            if (is_empty(d) && is_empty(c) && is_empty(b) &&
                !is_square_attacked(d, opposite_color(king.color)) &&
                !is_square_attacked(c, opposite_color(king.color))) {
                moves.push_back({sq, c});
            }
        }
    }
    
    return moves;
}

// In board.cpp
std::vector<Move> Board::generate_queen_moves(Square sq) const {
    std::vector<Move> moves;
    // Combine rook and bishop attacks for queen
    uint64_t attacks = get_rook_attacks(sq, occupancy()) | 
                      get_bishop_attacks(sq, occupancy());
    
    while (attacks) {
        Square to = pop_lsb(attacks);
        const Piece& target = piece_at(to);
        if (target.type == NONE_PIECE || target.color != piece_at(sq).color) {
            moves.push_back({sq, to}); // No promotion for queen moves
        }
    }
    return moves;
}

// Knight moves
std::vector<Move> Board::generate_knight_moves(Square sq) const {
    std::vector<Move> moves;
    static const int knight_deltas[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
    const Piece& knight = piece_at(sq);
    
    for (int delta : knight_deltas) {
        Square to = sq + delta;
        if (is_on_board(to)) {
            // Check if move stays on board (no wrapping)
            int file_diff = abs(file_of(to) - file_of(sq));
            int rank_diff = abs(rank_of(to) - rank_of(sq));
            if (file_diff + rank_diff == 3 && file_diff > 0 && rank_diff > 0) {
                const Piece& target = piece_at(to);
                if (target.type == NONE_PIECE || target.color != knight.color) {
                    moves.push_back({sq, to}); // No promotion for knights
                }
            }
        }
    }
    return moves;
}

// Bishop moves
std::vector<Move> Board::generate_bishop_moves(Square sq) const {
    std::vector<Move> moves;
    const Piece& bishop = piece_at(sq);
    uint64_t attacks = get_bishop_attacks(sq, occupancy());
    
    while (attacks) {
        Square to = pop_lsb(attacks);
        const Piece& target = piece_at(to);
        if (target.type == NONE_PIECE || target.color != bishop.color) {
            moves.push_back({sq, to});
        }
    }
    return moves;
}

// Rook moves
std::vector<Move> Board::generate_rook_moves(Square sq) const {
    std::vector<Move> moves;
    const Piece& rook = piece_at(sq);
    uint64_t attacks = get_rook_attacks(sq, occupancy());
    
    while (attacks) {
        Square to = pop_lsb(attacks);
        const Piece& target = piece_at(to);
        if (target.type == NONE_PIECE || target.color != rook.color) {
            moves.push_back({sq, to});
        }
    }
    return moves;
}

// King moves

std::vector<Move> Board::generate_pawn_moves(Square sq) const {
    std::vector<Move> moves;
    const Piece& pawn = piece_at(sq);
    int direction = (pawn.color == WHITE) ? 1 : -1;
    Square forward = sq + (8 * direction);

    // Single push
    if (is_empty(forward)) {
        // In generate_pawn_moves()
        // Remove the "promote to P" from non-promotion moves
        // Only add promotion for 7th->8th rank moves
        
        if (rank_of(forward) == 7 || rank_of(forward) == 0) {
            // Only add promotion moves when actually promoting
            moves.push_back({sq, forward, QUEEN});
            moves.push_back({sq, forward, ROOK});
            moves.push_back({sq, forward, BISHOP});
            moves.push_back({sq, forward, KNIGHT});
        } else {
            moves.push_back({sq, forward}); // Normal move, no promotion
        }
        
        // Double push from starting rank
        if ((pawn.color == WHITE && Board::rank_of(sq) == 1) ||
            (pawn.color == BLACK && Board::rank_of(sq) == 6)) {
            Square double_forward = forward + (8 * direction);
            if (is_empty(double_forward)) {
                moves.push_back({sq, double_forward});
            }
        }
    }

    // Captures
    for (int capture_dir : {-1, 1}) {
        int new_file = Board::file_of(sq) + capture_dir;
        if (new_file >= 0 && new_file < 8) {
            Square capture_sq = forward + capture_dir;
            if (!is_empty(capture_sq) && piece_at(capture_sq).color != pawn.color) {
                moves.push_back({sq, capture_sq});
            }
            // En passant
            // In generate_pawn_moves()
            if (capture_sq == m_en_passant) {
                moves.push_back({sq, capture_sq});
            }
        }
    }

    return moves;
}

std::vector<Move> Board::generate_pseudo_legal_moves() const {
    std::vector<Move> moves;
    
    for (Square sq = A1; sq < NUM_SQUARES; ++sq) {
        const Piece& piece = piece_at(sq);
        if (piece.color != m_side_to_move) continue;
        
        std::vector<Move> piece_moves;
        
        switch (piece.type) {
            case PAWN:
                piece_moves = generate_pawn_moves(sq);
                break;
            case KNIGHT:
                piece_moves = generate_knight_moves(sq);
                break;
            case BISHOP:
                piece_moves = generate_bishop_moves(sq);
                break;
            case ROOK:
                piece_moves = generate_rook_moves(sq);
                break;
            case QUEEN:
                piece_moves = generate_queen_moves(sq);
                break;
            case KING:
                piece_moves = generate_king_moves(sq);
                break;
            default:
                break;
        }
        
        moves.insert(moves.end(), piece_moves.begin(), piece_moves.end());
    }
    
    return moves;
}


std::vector<Move> Board::generate_legal_moves() const {
    std::vector<Move> legal_moves;
    Color us = m_side_to_move;
    Square king_sq = find_king(us);
    bool in_check = is_in_check(us);
    
    // Get all checking pieces
    uint64_t checkers = in_check ? attackers_to(king_sq, opposite_color(us)) : 0;
    
    // Generate all pseudo-legal moves
    std::vector<Move> pseudo_legal = generate_pseudo_legal_moves();
    
    for (const Move& move : pseudo_legal) {
        // King moves are special - just check if destination is safe
        if (piece_at(move.from).type == KING) {
            if (!is_square_attacked(move.to, opposite_color(us))) {
                legal_moves.push_back(move);
            }
            continue;
        }
        
        // If in check, move must address the check
        if (in_check) {
            // Must capture checking piece or block the attack
            bool valid = false;
            
            // Capture the checking piece
            if (checkers & (1ULL << move.to)) {
                valid = true;
            }
            // Block a sliding attack
            else if (count_bits(checkers) == 1) {
                Square checker_sq = pop_lsb(checkers);
                PieceType checker_type = piece_at(checker_sq).type;
                if (checker_type == ROOK || checker_type == BISHOP || checker_type == QUEEN) {
                    uint64_t between = squares_between(king_sq, checker_sq);
                    if (between & (1ULL << move.to)) {
                        valid = true;
                    }
                }
            }
            
            if (!valid) continue;
        }
        
        // Verify move doesn't leave king in check
        Board temp = *this;
        temp.make_move(move);
        if (!temp.is_in_check(us)) {
            legal_moves.push_back(move);
        }
    }
    
    return legal_moves;
}

PieceType char_to_piece(char c) {
    switch (toupper(c)) {
        case 'P': return PAWN;
        case 'N': return KNIGHT;
        case 'B': return BISHOP;
        case 'R': return ROOK;
        case 'Q': return QUEEN;
        case 'K': return KING;
        default: return NONE_PIECE;
    }
}

// ===== Debug =====
void Board::print() const {
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            Square sq = Square(rank * 8 + file);
            const Piece& p = piece_at(sq);
            char c = '.';
            if (p.type != NONE_PIECE) {
                c = "PNBRQK"[p.type];
                if (p.color == BLACK) c = tolower(c);
            }
            std::cout << c << ' ';
        }
        std::cout << '\n';
    }
}

} // namespace ViperChess