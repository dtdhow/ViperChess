#pragma once
#include "board.hpp"
#include <vector>
#include <random>
#include <unordered_map>

namespace ViperChess {

class OpeningBook {
public:
    OpeningBook();
    ~OpeningBook();
    
    bool load(const std::string& filename);
    Move probe(const Board& board) const;
    bool is_loaded() const { return !m_entries.empty(); }

private:
    struct PolyglotEntry {
        uint64_t key;
        uint16_t move;
        uint16_t weight;
        uint32_t learn;
    };

    struct BookMove {
        Move move;
        int weight;
    };
    
    std::unordered_map<uint64_t, std::vector<BookMove>> m_entries;
    mutable std::mt19937 m_rng;
    
    Move decode_polyglot_move(uint16_t move, const Board& board) const;
};

} // namespace ViperChess