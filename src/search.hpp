#pragma once
#include "board.hpp"
#include "eval.hpp"
#include "book.hpp"  // Add this include instead of forward declaration
#include <limits> // For INT_MAX
#include <chrono>
#include <stdio.h>
#include <atomic>
#include <thread>

namespace ViperChess {

constexpr int INF = std::numeric_limits<int>::max();



enum TTFlag {
    EXACT,
    LOWER_BOUND,
    UPPER_BOUND
};


struct TTEntry {
    uint64_t key;
    int depth;
    int score;
    Move best_move;
    uint8_t flag; // EXACT, LOWER_BOUND, UPPER_BOUND
};

class TranspositionTable {
    std::vector<TTEntry> table;
    size_t size;
    
public:
    TranspositionTable(size_t mb_size) {
        size = (mb_size * 1024 * 1024) / sizeof(TTEntry);
        table.resize(size);
    }
    
    // Copy constructor
    TranspositionTable(const TranspositionTable& other) {
        size = other.size;
        table = other.table;
    }
    
    // Copy assignment
    TranspositionTable& operator=(const TranspositionTable& other) {
        if (this != &other) {
            size = other.size;
            table = other.table;
        }
        return *this;
    }
    
    void store(uint64_t key, int depth, int score, Move move, uint8_t flag) {
        size_t index = key % size;
        if (depth >= table[index].depth) {
            table[index] = {key, depth, score, move, flag};
        }
    }
    
    bool probe(uint64_t key, int depth, int alpha, int beta, int& score, Move& move) {
        size_t index = key % size;
        if (table[index].key == key && table[index].depth >= depth) {
            move = table[index].best_move;
            score = table[index].score;
            
            if (table[index].flag == EXACT) return true;
            if (table[index].flag == LOWER_BOUND && score >= beta) return true;
            if (table[index].flag == UPPER_BOUND && score <= alpha) return true;
        }
        return false;
    }
    
};

struct SearchParams {
    int depth = 6;
    int time_ms = 5000;
    bool use_time = true;
    bool infinite = false;
};

struct SearchResult {
    Move best_move = {A1, A1};
    int score = 0;
    uint64_t nodes = 0;
    int depth = 0;
    std::vector<Move> pv;
    // Remove the TT pointer - we'll handle it differently
};

class Searcher {
public:
    explicit Searcher(Evaluator& evaluator, OpeningBook* book = nullptr);
    Move probe_book(const Board& board) const;
    SearchResult search(const Board& board, const SearchParams& params);
    int pvs(Board& board, int depth, int alpha, int beta, bool null_move);
    void parallel_search(const Board& board, SearchParams params);
    int get_num_threads() { return num_threads(); }

private:
    bool m_running = false;  // Add this line
    OpeningBook* m_book;  // Non-owning pointer
    void adjust_time(int move_number, int time_left, int increment);
    int alpha_beta(Board& board, int depth, int alpha, int beta, bool null_move);
    int quiescence(Board& board, int alpha, int beta);
    void order_moves(Board& board, std::vector<Move>& moves, Move tt_move);
    bool time_elapsed() const;
    std::atomic<bool>* m_stop = nullptr;
    static int num_threads() { 
        static int num = std::thread::hardware_concurrency(); 
        return num > 0 ? num : 4; // Fallback to 4 if hardware_concurrency fails
    }
    TranspositionTable m_tt{16}; // 16MB transposition table
    Evaluator& m_evaluator;
    SearchParams m_params;
    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    uint64_t m_nodes = 0;
    Move m_killer_moves[64][2]; // [ply][slot]
    int m_history[2][64][64];   // [color][from][to]
    int m_ply = 0; // Track current ply
};

// Piece values for move ordering
constexpr int piece_value(PieceType pt) {
    switch(pt) {
        case PAWN: return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK: return 500;
        case QUEEN: return 900;
        case KING: return 20000;
        default: return 0;
    }
}

} // namespace ViperChess