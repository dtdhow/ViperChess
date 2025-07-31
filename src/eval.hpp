#pragma once
#include "board.hpp"

namespace ViperChess {

// Piece values and evaluation weights
struct EvalWeights {
    int pawn = 100;
    int knight = 320;
    int bishop = 330;
    int rook = 500;
    int queen = 900;
    int king_safety = 50;
    int pawn_structure = 30;
    int material = 100;
    int mobility = 1;
    int center_control = 1;
};

class Evaluator {
public:
    explicit Evaluator(const EvalWeights& weights = {}) : m_weights(weights) {}

    int evaluate(const Board& board) const;
    int evaluate_mobility(const Board& board, Color color) const;
    int evaluate_material(const Board& board, Color color) const;
    int evaluate_pawn_structure(const Board& board, Color color) const;
    int evaluate_king_safety(const Board& board, Color color) const;
    float game_phase(const Board& board) const;

private:
    EvalWeights m_weights;
    static const int PAWN_TABLE[64];
};

// ===== 3. Piece-Square Tables =====
extern const int PSQT_PAWN[64];
extern const int PSQT_KNIGHT[64];
extern const int PSQT_BISHOP[64];
extern const int PSQT_ROOK[64];
extern const int PSQT_QUEEN[64];
extern const int PSQT_KING_MID[64];
extern const int PSQT_KING_END[64];

} // namespace ViperChess