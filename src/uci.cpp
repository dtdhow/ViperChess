#include "uci.hpp"
#include <sstream>
#include <thread>
#include <iostream>

namespace ViperChess {

UCI::UCI(Board& board, Evaluator& evaluator) 
    : m_board(board),
      m_evaluator(evaluator),  // Store evaluator reference
      m_searcher(m_evaluator, &m_book)  // Initialize searcher
{
    // Load default book
    m_book.load("books/book.bin");
}

void UCI::loop() {
    // Load default book
    
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "uci") {
            handle_uci();
        } else if (command == "isready") {
            handle_isready();
        } else if (command == "position") {
            std::string args;
            std::getline(iss, args);
            handle_position(args);
        } else if (command == "go") {
            std::string args;
            std::getline(iss, args);
            handle_go(args);
        } else if (command == "stop") {
            handle_stop();
        } else if (command == "setoption") {
            handle_setoption(iss);
        } else if (command == "quit") {
            break;
        }
    }
}

void UCI::handle_setoption(std::istringstream& iss) {
    std::string token;
    iss >> token; // Read "name"
    iss >> token; // Read option name
    
    if (token == "OwnBook") {
        iss >> token; // skip "value"
        iss >> m_use_book;
        std::cout << "info string Book usage " << (m_use_book ? "enabled" : "disabled") << "\n";
    } else if (token == "BookFile") {
        iss >> token; // skip "value"
        std::string book_file;
        iss >> book_file;
        if (m_book.load(book_file)) {
            std::cout << "info string Loaded book: " << book_file << "\n";
        } else {
            std::cout << "info string Failed to load book: " << book_file << "\n";
        }
    }
}

void UCI::handle_uci() {
    std::cout << "id name ViperChessMegaEdition\n";
    std::cout << "id author dtdhow (AUTHORS FILE)\n";
    std::cout << "option name OwnBook type check default true\n";
    std::cout << "option name BookFile type string default book.bin\n";
    std::cout << "uciok\n";
}

void UCI::handle_isready() {
    std::cout << "readyok\n";
}

void UCI::handle_position(const std::string& args) {
    std::istringstream iss(args);
    std::string token;
    
    // Parse position
    iss >> token;
    if (token == "startpos") {
        m_board.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        iss >> token; // Consume "moves" if present
    } else if (token == "fen") {
        std::string fen;
        while (iss >> token && token != "moves") {
            fen += token + " ";
        }
        m_board.set_fen(fen);
    }

    // Parse moves
    if (token == "moves") {
        while (iss >> token) {
            Move move = {
                static_cast<Square>(token[0] - 'a' + (token[1] - '1') * 8),
                static_cast<Square>(token[2] - 'a' + (token[3] - '1') * 8),
                token.length() > 4 ? char_to_piece(token[4]) : NONE_PIECE
            };
            m_board.make_move(move);
        }
    }
    
    if (m_use_book && m_book.is_loaded() && m_board.get_fullmove_number() < 10) {
        Move book_move = m_book.probe(m_board);
        if (book_move.is_valid()) {
            m_board.make_move(book_move);
        }
    }
}

void UCI::handle_go(const std::string& args) {
    SearchParams params;
    std::istringstream iss(args);
    std::string token;
    
    while (iss >> token) {
        if (token == "depth") {
            iss >> params.depth;
        } else if (token == "movetime") {
            iss >> params.time_ms;
        } else if (token == "infinite") {
            params.infinite = true;
        }
    }

    m_running = true;
    std::thread search_thread([this, params]() {
        SearchResult result = m_searcher.search(m_board, params);
        if (m_running) {
            print_best_move(result.best_move);
        }
    });
    search_thread.detach();
}

void UCI::handle_stop() {
    m_running = false;
    // Note: Actual stopping requires search changes (not shown here for brevity)
}

void UCI::print_best_move(const Move& move) {
    std::cout << "bestmove ";
    std::cout << char('a' + m_board.file_of(move.from));
    std::cout << char('1' + m_board.rank_of(move.from));
    std::cout << char('a' + m_board.file_of(move.to));
    std::cout << char('1' + m_board.rank_of(move.to));
    if (move.promotion != NONE_PIECE) {
        std::cout << char(tolower(" PNBRQK"[move.promotion]));
    }
    std::cout << "\n";
}

} // namespace ViperChess