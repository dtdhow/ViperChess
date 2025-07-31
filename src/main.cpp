#include "uci.hpp"
#include "eval.hpp"

namespace ViperChess {

int main() {
    Board board;
    Evaluator evaluator;
    UCI uci(board, evaluator);  // This is where the constructor is called
    
    uci.loop();
    return 0;
}

} // namespace ViperChess

// Entry point
int main(int argc, char* argv[]) {
    return ViperChess::main();
}