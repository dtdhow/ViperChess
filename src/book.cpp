#include "book.hpp"
#include <fstream>
#include <algorithm>

namespace ViperChess {

OpeningBook::OpeningBook() : m_rng(std::random_device{}()) {}

OpeningBook::~OpeningBook() {
    m_entries.clear();
}

bool OpeningBook::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    
    m_entries.clear();
    
    PolyglotEntry entry;
    while (file.read(reinterpret_cast<char*>(&entry), sizeof(entry))) {
        BookMove book_move;
        book_move.weight = entry.weight;
        m_entries[entry.key].push_back(book_move);
    }
    
    return !m_entries.empty();
}

Move OpeningBook::probe(const Board& board) const {
    if (m_entries.empty()) return Move::none();
    
    uint64_t key = board.get_zobrist_key();
    auto it = m_entries.find(key);
    if (it == m_entries.end()) return Move::none();
    
    const auto& moves = it->second;
    if (moves.empty()) return Move::none();
    
    // Calculate total weight
    int total_weight = 0;
    for (const auto& book_move : moves) {
        total_weight += book_move.weight;
    }
    
    // Select random move weighted by frequency
    std::uniform_int_distribution<int> dist(0, total_weight-1);
    int r = dist(m_rng);
    int sum = 0;
    
    for (const auto& book_move : moves) {
        sum += book_move.weight;
        if (r < sum) {
            return book_move.move;
        }
    }
    
    return moves.back().move;
}

Move OpeningBook::decode_polyglot_move(uint16_t move, const Board& board) const {
    // Polyglot move format:
    // bits 0-5: from square (0-63)
    // bits 6-11: to square (0-63)
    // bits 12-14: promotion piece (0=None, 1=Knight, 2=Bishop, 3=Rook, 4=Queen)
    
    Square from = Square((move >> 6) & 0x3F);
    Square to = Square(move & 0x3F);
    uint8_t promotion = (move >> 12) & 0x7;
    
    PieceType promo_piece = NONE_PIECE;
    switch (promotion) {
        case 1: promo_piece = KNIGHT; break;
        case 2: promo_piece = BISHOP; break;
        case 3: promo_piece = ROOK; break;
        case 4: promo_piece = QUEEN; break;
    }
    
    // Verify the move is legal
    auto legal_moves = board.generate_legal_moves();
    for (const auto& legal_move : legal_moves) {
        if (legal_move.from == from && 
            legal_move.to == to && 
            legal_move.promotion == promo_piece) {
            return legal_move;
        }
    }
    
    return Move::none();
}

} // namespace ViperChess