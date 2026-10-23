#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

int main(int argc, char* argv[]) {
    ifstream f(argc > 1 ? argv[1] : "input.in");
    ofstream g(argc > 2 ? argv[2] : "output.out");
    if (!f || !g) {
        cerr << "Could not open input or output file.\n";
        return 1;
    }

    int n;
    if (!(f >> n)) return 1;

    // Directions 1..6: right, down-right, down-left, left, up-left, up-right.
    const int dr[] = {0, 0, 1, 1, 0, -1, -1};
    const int dc[] = {0, 2, 1, -1, -2, -1, 1};

    for (int i = 0; i < n; i++) {
        int radius, rows, cols, ticks;
        if (!(f >> radius >> rows >> cols) || radius <= 0
            || rows != 2 * radius + 1 || cols != 4 * radius + 1) return 1;

        vector<string> pattern(rows);
        vector<pair<int, int>> required;
        for (int row = 0; row < rows; row++) {
            if (!(f >> pattern[row]) || static_cast<int>(pattern[row].size()) != cols) return 1;
            for (int col = 0; col < cols; col++) {
                if (pattern[row][col] == 'M') required.push_back({row, col});
            }
        }
        if (!(f >> ticks) || ticks < 0 || required.empty()) return 1;

        vector<string> board = pattern;
        vector<vector<bool>> forbidden(rows, vector<bool>(cols, false));
        for (const auto& cell : required) {
            board[cell.first][cell.second] = 'O';
            for (int direction = 1; direction <= 6; direction++) {
                int row = cell.first + dr[direction];
                int col = cell.second + dc[direction];
                if (row >= 0 && row < rows && col >= 0 && col < cols
                    && pattern[row][col] == 'O') forbidden[row][col] = true;
            }
        }
        vector<pair<int, int>> adjacent;
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                if (forbidden[row][col]) adjacent.push_back({row, col});
            }
        }

        // Clockwise border order, starting at emitter 1's left corner.
        const int borderDr[] = {-1, 0, 1, 1, 0, -1};
        const int borderDc[] = {1, 2, 1, -1, -2, -1};
        vector<pair<int, int>> border;
        int borderRow = radius, borderCol = 0;
        for (int side = 0; side < 6; side++) {
            for (int step = 0; step < radius; step++) {
                border.push_back({borderRow, borderCol});
                borderRow += borderDr[side];
                borderCol += borderDc[side];
            }
        }
        int perimeter = 6 * radius;
        int position[6];
        bool active[6] = {};
        for (int emitter = 0; emitter < 6; emitter++) position[emitter] = emitter * radius;
        int submitted = 0;

        for (int tick = 0; tick < ticks; tick++) {
            for (int emitter = 0; emitter < 6; emitter++) {
                char instruction;
                if (!(f >> instruction)) return 1;
                switch (instruction) {
                    case 'F': position[emitter] = (position[emitter] + 1) % perimeter; break;
                    case 'B': position[emitter] = (position[emitter] + perimeter - 1) % perimeter; break;
                    case 'T': active[emitter] = !active[emitter]; break;
                    case '-': break;
                    default: return 1;
                }
            }

            vector<vector<int>> hits(rows, vector<int>(cols, 0));
            for (int emitter = 0; emitter < 6; emitter++) {
                if (!active[emitter]) continue;
                int direction = emitter + 1;
                int row = border[position[emitter]].first + dr[direction];
                int col = border[position[emitter]].second + dc[direction];
                while (row >= 0 && row < rows && col >= 0 && col < cols) {
                    if (board[row][col] == 'O' || board[row][col] == 'M') {
                        hits[row][col]++;
                    }
                    row += dr[direction];
                    col += dc[direction];
                }
            }

            // Update only after every active emitter has fired.
            for (int row = 0; row < rows; row++) {
                for (int col = 0; col < cols; col++) {
                    if (hits[row][col] == 2) board[row][col] = 'M';
                    else if (hits[row][col] >= 3) board[row][col] = 'O';
                }
            }

            // A match is the exact target component, with no extra matter touching it.
            bool matches = true;
            for (const auto& cell : required) {
                if (board[cell.first][cell.second] != 'M') {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                for (const auto& cell : adjacent) {
                    if (board[cell.first][cell.second] == 'M') {
                        matches = false;
                        break;
                    }
                }
            }
            if (matches) {
                submitted++;
                for (const auto& cell : required) board[cell.first][cell.second] = 'O';
            }
        }

        g << submitted << '\n';
    }
    return 0;
}
