#pragma once
#include "board.hpp"
#include <string>
#include <vector>

namespace ViperChess {

class Syzygy {
public:
    bool init(const std::string& path);
    int probe_wdl(Board& board, int* success);
    Move probe_dtz(Board& board, int* success);
    
private:
    bool m_initialized = false;
};

} // namespace ViperChess