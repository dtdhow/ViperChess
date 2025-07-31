#include "syzygy.hpp"
#include "fathom/src/tbprobe.h" // From Fathom library

namespace ViperChess {

bool Syzygy::init(const std::string& path) {
    m_initialized = tb_init(path.c_str()) == TB_NO_MISSING_WDL;
    return m_initialized;
}

int Syzygy::probe_wdl(Board& board, int* success) {
    if (!m_initialized) {
        *success = 0;
        return 0;
    }
    
    unsigned result;
    *success = tb_probe_wdl(
        board.get_white_pieces(),
        board.get_black_pieces(),
        board.get_kings(),
        board.get_queens(),
        board.get_rooks(),
        board.get_bishops(),
        board.get_knights(),
        board.get_pawns(),
        board.get_ep_square(),
        board.get_castling_rights(),
        board.get_side_to_move(),
        &result
    );
    
    return static_cast<int>(result);
}

Move Syzygy::probe_dtz(Board& board, int* success) {
    if (!m_initialized) {
        *success = 0;
        return Move::none();
    }
    
    unsigned move;
    int result = tb_probe_root(
        board.get_white_pieces(),
        board.get_black_pieces(),
        board.get_kings(),
        board.get_queens(),
        board.get_rooks(),
        board.get_bishops(),
        board.get_knights(),
        board.get_pawns(),
        board.get_ep_square(),
        board.get_castling_rights(),
        board.get_side_to_move(),
        nullptr, // Use TB_GET_MOVE
        &move
    );
    
    *success = result != TB_RESULT_FAILED;
    return Move::from_syzygy(move);
}

} // namespace ViperChess