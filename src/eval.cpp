#include "board.hpp"
#include "eval.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace ViperChess {

// ===== 1. Material Values =====
constexpr int PIECE_VALUES[6] = {
    100,   // Pawn
    320,   // Knight
    330,   // Bishop
    500,   // Rook
    900,   // Queen
    20000  // King (not actually used)
};

// ===== 2. Piece-Square Tables =====
constexpr int PAWN_TABLE[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    10,  10,  20,  30,  30,  20,  10,  10,
     5,   5,  10,  25,  25,  10,   5,   5,
     0,   0,   0,  20,  20,   0,   0,   0,
     5,  -5, -10,   0,   0, -10,  -5,   5,
     5,  10,  10, -20, -20,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0
};

constexpr int KNIGHT_TABLE[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};




// Mirror table for black pieces
int read_psqt(int sq, const int* table) {
    return (sq < 64) ? table[sq] : table[63 - (sq % 64)];
}

// ===== 4. Core Evaluation Function =====
int Evaluator::evaluate(const Board& board) const {
    int score = 0;
    
    // Material evaluation
    score += evaluate_material(board, WHITE);
    score -= evaluate_material(board, BLACK);

    // Pawn structure
    score += evaluate_pawn_structure(board, WHITE);
    score -= evaluate_pawn_structure(board, BLACK);

    // King safety
    score += evaluate_king_safety(board, WHITE);
    score -= evaluate_king_safety(board, BLACK);

    // Mobility
    score += evaluate_mobility(board, WHITE);
    score -= evaluate_mobility(board, BLACK);

    // Tapered evaluation
    float phase = game_phase(board);
    return score * phase;
}

int Evaluator::evaluate_material(const Board& board, Color color) const {
    int material = 0;
    const auto& pieces = board.get_pieces(color);
    
    material += count_bits(pieces[PAWN]) * m_weights.pawn;
    material += count_bits(pieces[KNIGHT]) * m_weights.knight;
    material += count_bits(pieces[BISHOP]) * m_weights.bishop;
    material += count_bits(pieces[ROOK]) * m_weights.rook;
    material += count_bits(pieces[QUEEN]) * m_weights.queen;
    
    return material;
}

int Evaluator::evaluate_pawn_structure(const Board& board, Color color) const {
    int score = 0;
    const auto pawns = board.get_pieces(color)[PAWN];

    // Doubled pawns
    for (int file = 0; file < 8; file++) {
        int count = count_bits(pawns & Board::FILE_MASKS[file]);
        if (count > 1) score -= (count - 1) * 20;
    }

    // Isolated pawns
    for (int file = 0; file < 8; file++) {
        bool isolated = true;
        if (file > 0 && (pawns & Board::FILE_MASKS[file-1])) isolated = false;
        if (file < 7 && (pawns & Board::FILE_MASKS[file+1])) isolated = false;
        if (isolated) score -= 15;
    }

    // Passed pawns
    Bitboard passed = 0;
    


    score += count_bits(passed) * 30;

    return score;
}

int Evaluator::evaluate_king_safety(const Board& board, Color color) const {
    Square king_sq = board.find_king(color);
    int safety = 0;
    
    // Pawn shield bonus
    Bitboard shield = Board::KING_SHIELD[color][king_sq] & board.get_pieces(color)[PAWN];
    safety += count_bits(shield) * 5;
    
    return safety * m_weights.king_safety;
}

int Evaluator::evaluate_mobility(const Board& board, Color color) const {
    int mobility = 0;
    uint64_t occupied = board.occupancy();
    uint64_t friendly = color == WHITE ? board.get_white_pieces() : board.get_black_pieces();
    
    // Knight mobility
    Bitboard knights = board.get_pieces(color)[KNIGHT];
    while (knights) {
        Square sq = Board::pop_lsb(knights);
        mobility += count_bits(board.knight_attack_table[sq] & ~friendly);
    }
    
    // Bishop mobility
    Bitboard bishops = board.get_pieces(color)[BISHOP];
    while (bishops) {
        Square sq = Board::pop_lsb(bishops);
        mobility += count_bits(board.get_bishop_attacks(sq, occupied) & ~friendly);
    }

    // Rook mobility
    Bitboard rooks = board.get_pieces(color)[ROOK];
    while (rooks) {
        Square sq = Board::pop_lsb(rooks);
        mobility += count_bits(board.get_rook_attacks(sq, occupied) & ~friendly);
    }

    // Queen mobility
    Bitboard queens = board.get_pieces(color)[QUEEN];
    while (queens) {
        Square sq = Board::pop_lsb(queens);
        mobility += count_bits(board.get_queen_attacks(sq, occupied) & ~friendly);
    }
    
    return mobility * m_weights.mobility;
}

float Evaluator::game_phase(const Board& board) const {
    // Returns 0 (endgame) to 1 (opening)
    int material = evaluate_material(board, WHITE) + evaluate_material(board, BLACK);
    return std::clamp(material / 4000.0f, 0.0f, 1.0f);
}


// ===== 6. Helper Constants =====
constexpr Bitboard FILE_MASKS[8] = {
    0x0101010101010101ULL, 0x0202020202020202ULL, // Files A-H
    // ... (omitted for brevity)
};
}