#include <iostream>
#include <fstream>
#include <string>
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
        int radius, rows, cols;
        if (!(f >> radius >> rows >> cols) || rows <= 0 || cols <= 0) return 1;

        vector<string> board(rows);
        for (int row = 0; row < rows; row++) {
            if (!(f >> board[row]) || static_cast<int>(board[row].size()) != cols) return 1;
        }

        vector<vector<int>> hits(rows, vector<int>(cols, 0));
        for (int emitterRow = 0; emitterRow < rows; emitterRow++) {
            for (int emitterCol = 0; emitterCol < cols; emitterCol++) {
                char emitter = board[emitterRow][emitterCol];
                if (emitter < '1' || emitter > '6') continue;
                int direction = emitter - '0';
                int row = emitterRow + dr[direction];
                int col = emitterCol + dc[direction];
                while (row >= 0 && row < rows && col >= 0 && col < cols) {
                    if (board[row][col] == 'O' || board[row][col] == 'M') {
                        hits[row][col]++;
                    }
                    row += dr[direction];
                    col += dc[direction];
                }
            }
        }

        // Apply all beams simultaneously; zero or one hit preserves the cell.
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                if (hits[row][col] == 2) board[row][col] = 'M';
                else if (hits[row][col] >= 3) board[row][col] = 'O';
            }
        }

        for (const string& line : board) g << line << '\n';
        g << '\n';
    }
    return 0;
}
