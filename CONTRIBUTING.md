# Contributing to ViperChess-Mega (Alpha)

Thank you for helping build Viper! Since we're in alpha, contributions are especially valuable.

## How to Help

### 🐛 Reporting Bugs
1. Check if the issue already exists in [GitHub Issues]
2. Provide:
   - The **FEN** or moves leading to the bug
   - Expected vs. actual behavior
   - Your system specs (CPU, OS, compiler)

### 💻 Code Contributions
1. **Small fixes**: Direct PRs welcome (typos, simple bugs)
2. **Major features**: Open an issue first to discuss
3. **Style**:
   - Follow existing C++20 conventions
   - 4-space indents, `snake_case` for variables
   - Document non-obvious eval terms

### 📈 Testing
- Benchmark with `go depth 12` on startpos
- Verify no speed regressions via `nodes/second`

## First-Time Setup
```sh
git clone https://github.com/yourname/ViperChess-Mega
cd ViperChess-Mega
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
