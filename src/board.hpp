// board.hpp
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <array>
#include <random>

namespace ViperChess {

using Bitboard = uint64_t;

// Forward declarations
enum Color : uint8_t { WHITE, BLACK, NUM_COLORS, NONE };
enum PieceType : uint8_t { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NUM_PIECE_TYPES, NONE_PIECE };
enum Square : uint8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NUM_SQUARES
};
enum castlingRights : uint8_t {
    WHITE_OO = 1,   // 1 << 0
    WHITE_OOO = 2,  // 1 << 1
    BLACK_OO = 4,   // 1 << 2
    BLACK_OOO = 8   // 1 << 3
};

// Enum operators


struct Move {
    Square from;
    Square to;
    PieceType promotion = NONE_PIECE; // Default to no promotion
    
    int weight = 1;

    static Move none() { return Move(NUM_SQUARES, NUM_SQUARES); }
    
    // Utility methods
    bool is_valid() const { return from != NUM_SQUARES && to != NUM_SQUARES; }

    Move() = default;  // Default constructor
        Move(Square f, Square t, PieceType p = NONE_PIECE) : from(f), to(t), promotion(p) {}
        
        // Comparison operator
        bool operator==(const Move& other) const {
            return from == other.from && to == other.to && promotion == other.promotion;
        }
};

struct Piece {
    PieceType type;
    Color color;
    
    static const Piece NONE;
    static const Piece W_PAWN;
    static const Piece B_PAWN;
    static const Piece W_KNIGHT;
    static const Piece B_KNIGHT;
    static const Piece W_BISHOP;
    static const Piece B_BISHOP;
    static const Piece W_ROOK;
    static const Piece B_ROOK;
    static const Piece W_QUEEN;
    static const Piece B_QUEEN;
    static const Piece W_KING;
    static const Piece B_KING;
};

struct Magic {
    uint64_t mask;
    uint64_t magic;
    uint64_t* attacks;
    int shift;
};

// Global magic tables
extern Magic rook_magics[64];
extern Magic bishop_magics[64];
extern uint64_t rook_attacks[64][4096];
extern uint64_t bishop_attacks[64][512];

class Board {
private:
    uint64_t m_zobrist_key = 0;
    std::array<Piece, 64> m_squares;
    std::array<std::array<Bitboard, NUM_PIECE_TYPES>, NUM_COLORS> m_pieces;
    Color m_side_to_move;
    Square m_en_passant;
    int m_halfmove_clock;
    int m_fullmove_number;
    uint8_t m_castling_rights;  // Bitmask for castling rights
    
    // Move generation helpers
    std::vector<Move> generate_pawn_moves(Square sq) const;
    std::vector<Move> generate_knight_moves(Square sq) const;
    std::vector<Move> generate_bishop_moves(Square sq) const;
    std::vector<Move> generate_rook_moves(Square sq) const;
    std::vector<Move> generate_king_moves(Square sq) const;
    std::vector<Move> generate_queen_moves(Square sq) const;

    // Magic bitboard helpers
    void init_magics();
    static void init_sliding_attacks(Square sq, bool is_rook);
    static uint64_t compute_rook_attacks(Square sq, uint64_t occupancy);
    static uint64_t compute_bishop_attacks(Square sq, uint64_t occupancy);

public:
    uint64_t get_zobrist_key() const { return m_zobrist_key; }
    void update_zobrist_key(Piece piece, Square sq);

    void update_zobrist_piece(Piece piece, Square sq);
    void update_zobrist_side();
    void update_zobrist_castling();
    void update_zobrist_ep(Square ep_sq);
    void reset_to_startpos();
    
    // Bitboard accessors
    uint64_t get_pieces(Color c, PieceType pt) const { return m_pieces[c][pt]; }
    uint64_t get_white_pieces() const;
    uint64_t get_black_pieces() const;
    uint64_t get_kings() const;
    uint64_t get_queens() const;
    uint64_t get_rooks() const;
    uint64_t get_bishops() const;
    uint64_t get_knights() const;
    uint64_t get_pawns() const;
    
    // Board state accessors
    Square get_ep_square() const { return m_en_passant; }
    int get_castling_rights() const;
    int get_fullmove_number() const { return m_fullmove_number; }
    Board();
    void set_fen(const std::string& fen);
    static void init();

    uint64_t occupancy() const {
        uint64_t occ = 0;
        for (Color c : {WHITE, BLACK}) {
            for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
                occ |= m_pieces[c][pt];
            }
        }
        return occ;
    }

    const std::array<Bitboard, NUM_PIECE_TYPES>& get_pieces(Color color) const { 
        return m_pieces[color]; 
    }
    
    //Square find_king(Color color) const {
     //   return static_cast<Square>(__builtin_ctzll(m_pieces[color][KING]));
    //}

    // Constants - Now properly initialized as std::array
    static constexpr std::array<Bitboard, 8> FILE_MASKS = {{
        0x0101010101010101ULL, // File A
        0x0202020202020202ULL, // File B
        0x0404040404040404ULL, // File C
        0x0808080808080808ULL, // File D
        0x1010101010101010ULL, // File E
        0x2020202020202020ULL, // File F
        0x4040404040404040ULL, // File G
        0x8080808080808080ULL  // File H
    }};
    
    static constexpr std::array<std::array<Bitboard, 64>, 2> KING_SHIELD = {{
        // White king shields
        {{
            (1ULL << A2) | (1ULL << B2) | (1ULL << C2),
            // ... remaining 63 squares for white
        }},
        // Black king shields
        {{
            (1ULL << A7) | (1ULL << B7) | (1ULL << C7),
            // ... remaining 63 squares for black
        }}
    }};

    static Square pop_lsb(uint64_t& bb);

    void init_attack_tables();

    uint64_t zobrist_key() const; // Implement Zobrist hashing
    void make_null_move() { m_side_to_move = opposite_color(m_side_to_move); }

    static uint64_t pawn_attack_table[NUM_COLORS][64];
    static uint64_t knight_attack_table[64];
    static uint64_t king_attack_table[64];

    // Accessors
    const Piece& piece_at(Square sq) const { return m_squares[sq]; }
    bool is_empty(Square sq) const { return piece_at(sq).type == NONE_PIECE; }
    bool is_on_board(Square sq) const { return sq >= A1 && sq < NUM_SQUARES; }
    static int file_of(Square sq) { return sq % 8; }
    static int rank_of(Square sq) { return sq / 8; }

    // Move generation
    uint64_t get_rook_attacks(Square sq, uint64_t occupancy) const;
    uint64_t get_bishop_attacks(Square sq, uint64_t occupancy) const;
    uint64_t get_queen_attacks(Square sq, uint64_t occupancy) const;
    std::vector<Move> generate_pseudo_legal_moves() const;
    std::vector<Move> generate_legal_moves() const;
    
    bool is_legal(const Move& move) const;

    // Attack detection
    Color get_side_to_move() const { return m_side_to_move; }
    bool is_square_attacked(Square sq, Color by_color) const;
    Square find_king(Color color) const;
    bool is_in_check(Color color) const;
    Color opposite_color(Color color) const;
    void make_move(const Move& move);
    bool is_checkmate() const;
    bool is_stalemate() const;

    uint64_t attackers_to(Square sq, Color by_color) const;
    uint64_t squares_between(Square a, Square b) const;

    // Debug
    void print() const;
};

namespace Zobrist {
    extern uint64_t piece_keys[NUM_PIECE_TYPES][NUM_COLORS][NUM_SQUARES];
    extern uint64_t side_key;
    extern uint64_t castling_keys[16];
    extern uint64_t ep_keys[NUM_SQUARES];
    
    void init();
}

// Non-member helper functions
uint64_t rook_mask(Square sq);
uint64_t bishop_mask(Square sq);
int count_bits(uint64_t b);
uint64_t index_to_occupancy(int index, int bits, uint64_t mask);
PieceType char_to_piece(char c);


} // namespace ViperChess