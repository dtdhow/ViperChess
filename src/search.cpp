#include "search.hpp"
#include <algorithm>
#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>  // Add this for memset

namespace ViperChess {

Searcher::Searcher(Evaluator& evaluator, OpeningBook* book)
    : m_evaluator(evaluator), 
      m_book(book),
      m_tt(16),  // Initialize transposition table with 16MB
      m_running(false),
      m_nodes(0),
      m_ply(0)
{
    // Initialize killer moves and history heuristics
    std::memset(m_killer_moves, 0, sizeof(m_killer_moves));
    std::memset(m_history, 0, sizeof(m_history));
}

Move Searcher::probe_book(const Board& board) const {
    if (!m_book || board.get_fullmove_number() > 20) {
        return Move::none();
    }
    
    // This will now work since book.hpp is fully included
    Move book_move = m_book->probe(board);
    if (book_move.is_valid() && board.is_legal(book_move)) {
        return book_move;
    }
    return Move::none();
}

SearchResult Searcher::search(const Board& board, const SearchParams& params) {
    m_params = params;
    m_start_time = std::chrono::steady_clock::now();
    m_nodes = 0;
    m_ply = 0;
    
    Board temp_board = board;
    SearchResult result;
    int alpha = -INF;
    int beta = INF;

    Move book_move = probe_book(board);
    if (book_move.is_valid()) {
        SearchResult result;
        result.best_move = book_move;
        result.score = 0; // Book moves are considered equal
        result.depth = 0;
        result.nodes = 0;
        return result;
    }


    if (m_stop && m_stop->load()) {
        return SearchResult{}; // Early exit if stopped
    }

    // Iterative deepening
    for (int depth = 1; depth <= m_params.depth; ++depth) {
        std::vector<Move> moves = temp_board.generate_legal_moves();
        order_moves(temp_board, moves, Move(A1, A1)); // Default empty move

        for (const Move& move : moves) {
            Board new_board = temp_board;
            new_board.make_move(move);
            m_nodes++;
            m_ply++;

            int score = -alpha_beta(new_board, depth - 1, -beta, -alpha, true);
            m_ply--;

            if (score > alpha) {
                alpha = score;
                result.best_move = move;
                result.score = score;
                result.depth = depth;
            }

            if (time_elapsed() && m_params.use_time) {
                goto finish_search;
            }
        }
    }

    finish_search:
    result.nodes = m_nodes;

    if (m_nodes % 1000 == 0 && m_stop && m_stop->load()) {
        return SearchResult{};
    }
    return result;
}

int Searcher::alpha_beta(Board& board, int depth, int alpha, int beta, bool null_move) {
    if (depth <= 0) {
        return quiescence(board, alpha, beta);
    }

    // Null move pruning
    if (null_move && depth >= 3 && !board.is_in_check(board.get_side_to_move())) {
        Board temp = board;
        temp.make_null_move();
        m_ply++;
        int score = -alpha_beta(temp, depth - 1 - 2, -beta, -beta + 1, false);
        m_ply--;
        if (score >= beta) return beta;
    }

    std::vector<Move> moves = board.generate_legal_moves();
    order_moves(board, moves, Move(A1, A1));

    for (const Move& move : moves) {
        Board new_board = board;
        new_board.make_move(move);
        m_nodes++;
        m_ply++;

        int score = -alpha_beta(new_board, depth - 1, -beta, -alpha, true);
        m_ply--;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}



int Searcher::quiescence(Board& board, int alpha, int beta) {
    int stand_pat = m_evaluator.evaluate(board);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    std::vector<Move> captures;
    for (const Move& move : board.generate_legal_moves()) {
        if (board.piece_at(move.to).type != NONE_PIECE) {
            captures.push_back(move);
        }
    }

    for (const Move& move : captures) {
        Board new_board = board;
        new_board.make_move(move);
        m_nodes++;
        m_ply++;

        int score = -quiescence(new_board, -beta, -alpha);
        m_ply--;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

int Searcher::pvs(Board& board, int depth, int alpha, int beta, bool null_move) {
    if (depth <= 0) return quiescence(board, alpha, beta);
    
    std::vector<Move> moves = board.generate_legal_moves();
    order_moves(board, moves, Move(A1, A1)); // Default empty move
    
    bool first_move = true;
    for (const Move& move : moves) {
        Board new_board = board;
        new_board.make_move(move);
        m_nodes++;
        
        int score;
        if (first_move) {
            score = -pvs(new_board, depth-1, -beta, -alpha, true);
            first_move = false;
        } else {
            score = -pvs(new_board, depth-1, -alpha-1, -alpha, true);
            if (score > alpha)
                score = -pvs(new_board, depth-1, -beta, -alpha, true);
        }
        
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int calculate_lmr(int depth, int move_number) {
    if (depth >= 3 && move_number >= 4) {
        return 1 + log(depth) * log(move_number) / 2;
    }
    return 0;
}

void Searcher::order_moves(Board& board, std::vector<Move>& moves, Move tt_move) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        // 1. TT move first
        if (a == tt_move) return true;
        if (b == tt_move) return false;

        // 2. Captures (MVV-LVA)
        int a_cap = piece_value(board.piece_at(a.to).type);
        int b_cap = piece_value(board.piece_at(b.to).type);
        if (a_cap != b_cap) return a_cap > b_cap;

        // 3. Killer moves
        for (int i = 0; i < 2; ++i) {
            if (a == m_killer_moves[m_ply][i]) return true;
            if (b == m_killer_moves[m_ply][i]) return false;
        }

        // 4. History heuristic
        Color stm = board.get_side_to_move();

        //a.lmr_reduction = calculate_lmr(depth, move_number);

        return m_history[stm][a.from][a.to] > m_history[stm][b.from][b.to];
    });
}

void Searcher::adjust_time(int move_number, int time_left, int increment) {
    // Allocate more time for critical moments
    float factor = 1.0f;
    if (move_number < 10) factor = 0.8f; // Opening
    else if (move_number < 30) factor = 1.0f; // Middlegame
    else factor = 1.2f; // Endgame
    
    m_params.time_ms = std::min(time_left / 30, 
                               static_cast<int>((time_left * 0.05 + increment * 0.8) * factor));
}

bool Searcher::time_elapsed() const {
    if (m_params.infinite) return false;
    auto elapsed = std::chrono::steady_clock::now() - m_start_time;
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= m_params.time_ms;
}

void Searcher::parallel_search(const Board& board, SearchParams params) {
    // Create thread-safe stop flag
    std::atomic<bool> stop_flag(false);
    m_stop = &stop_flag;
    
    // Get number of threads
    const int num_threads = Searcher::get_num_threads();
    
    // Create thread pool and results
    std::vector<std::thread> threads;
    std::vector<SearchResult> results;
    results.reserve(num_threads);
    
    // Create a shared transposition table
    TranspositionTable shared_tt(16); // 16MB
    
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([this, &board, params, &results, i, &stop_flag, &shared_tt]() {
            Board local_board = board;
            Searcher local_searcher(m_evaluator);
            local_searcher.m_tt = shared_tt; // Share the TT by copy
            local_searcher.m_stop = &stop_flag;
            
            // Different search depths for each thread (Lazy SMP)
            SearchParams local_params = params;
            local_params.depth += i % 2;
            
            SearchResult result = local_searcher.search(local_board, local_params);
            results.push_back(result);
            
            // First thread to finish stops the others
            if (i == 0 && !stop_flag.load()) {
                stop_flag.store(true);
            }
        });
    }
    
    // Wait for all threads to finish
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // Find best result
    if (!results.empty()) {
        auto best_result = std::max_element(results.begin(), results.end(),
            [](const SearchResult& a, const SearchResult& b) {
                return a.score < b.score;
            });
        
        // Update main transposition table from shared one
        m_tt = shared_tt;
    }
}

} // namespace ViperChess