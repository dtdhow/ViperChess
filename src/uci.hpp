#pragma once
#include "board.hpp"
#include "search.hpp"
#include "book.hpp"
#include <string>

namespace ViperChess {

class UCI {
public:
    UCI(Board& board, Evaluator& evaluator);  // Constructor declaration
    
    void loop();
    void handle_uci();
    void handle_isready();
    void handle_position(const std::string& args);
    void handle_go(const std::string& args);
    void handle_stop();
    void handle_setoption(std::istringstream& iss);
    void print_best_move(const Move& move);
private:
    Board& m_board;
    Evaluator& m_evaluator;  // Change to reference
    Searcher m_searcher;
    OpeningBook m_book;

    bool m_use_book = true;
    bool m_running = false;
};

}