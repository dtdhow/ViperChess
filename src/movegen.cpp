// movegen.cpp
#include "board.hpp"
#include <algorithm>

namespace ViperChess {

std::vector<Move> Board::generate_legal_moves() const {
    std::vector<Move> moves;
    
    // Generate for each piece
    for (Square sq = A1; sq <= H8; ++sq) {
        if (piece_at(sq).color != m_side_to_move) continue;
        
        switch (piece_at(sq).type) {
            case PAWN: {
                auto pawn_moves = generate_pawn_moves(sq);
                moves.insert(moves.end(), pawn_moves.begin(), pawn_moves.end());
                break;
            }
            case KNIGHT: {
                auto knight_moves = generate_knight_moves(sq);
                moves.insert(moves.end(), knight_moves.begin(), knight_moves.end());
                break;
            }
            // ... other pieces
        }
    }
    
    // Remove moves that leave king in check
    moves.erase(std::remove_if(moves.begin(), moves.end(),
        [this](const Move& m) {
            Board temp = *this;
            temp.make_move(m); // Assume you have make_move implemented
            return temp.is_king_in_check(m_side_to_move);
        }), moves.end());
    
    return moves;
}

// Pawn moves (most complex)
std::vector<Move> Board::generate_pawn_moves(Square sq) const {
    std::vector<Move> moves;
    const Color color = piece_at(sq).color;
    const int forward = (color == WHITE) ? 8 : -8;
    
    // Single push
    Square target = sq + forward;
    if (is_empty(target)) {
        add_promotion_moves(moves, sq, target);
        
        // Double push from starting rank
        if ((color == WHITE && rank_of(sq) == 1) || 
            (color == BLACK && rank_of(sq) == 6)) {
            Square double_target = target + forward;
            if (is_empty(double_target)) {
                moves.push_back({sq, double_target});
            }
        }
    }
    
    // Captures
    for (int capture_dir : {-1, 1}) { // Left and right
        Square capture_sq = sq + forward + capture_dir;
        if (is_on_board(capture_sq) && 
            (piece_at(capture_sq).color == !color || capture_sq == m_en_passant)) {
            add_promotion_moves(moves, sq, capture_sq);
        }
    }
    
    return moves;
}

// Knight moves (fixed pattern)
std::vector<Move> Board::generate_knight_moves(Square sq) const {
    static constexpr std::array<int, 8> offsets = {-17, -15, -10, -6, 6, 10, 15, 17};
    std::vector<Move> moves;
    
    for (int offset : offsets) {
        Square target = sq + offset;
        if (is_on_board(target) && 
            piece_at(target).color != m_side_to_move) {
            moves.push_back({sq, target});
        }
    }
    
    return moves;
}

// Sliding pieces (bishops/rooks/queens)
std::vector<Move> Board::generate_sliding_moves(Square sq) const {
    static constexpr std::array<std::array<int, 4>, 2> directions = {{
        {8, -8, 1, -1},  // Rooks
        {7, -7, 9, -9}    // Bishops
    }};
    
    std::vector<Move> moves;
    PieceType pt = piece_at(sq).type;
    int start_idx = (pt == BISHOP) ? 0 : (pt == ROOK) ? 2 : 0; // Queen uses both
    
    for (int i = start_idx; i < (pt == QUEEN ? 4 : 2); ++i) {
        for (Square target = sq + directions[pt == BISHOP ? 1 : 0][i]; 
             is_on_board(target); 
             target += directions[pt == BISHOP ? 1 : 0][i]) {
            
            if (is_empty(target)) {
                moves.push_back({sq, target});
            }
            else {
                if (piece_at(target).color != m_side_to_move) {
                    moves.push_back({sq, target});
                }
                break;
            }
        }
    }
    
    return moves;
}

} // namespace ViperChess