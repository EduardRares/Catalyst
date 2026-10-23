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
        int emitterRow = -1, emitterCol = -1, direction = 0;
        for (int row = 0; row < rows; row++) {
            if (!(f >> board[row]) || static_cast<int>(board[row].size()) != cols) return 1;
            for (int col = 0; col < cols; col++) {
                if (board[row][col] >= '1' && board[row][col] <= '6') {
                    emitterRow = row;
                    emitterCol = col;
                    direction = board[row][col] - '0';
                }
            }
        }
        if (direction == 0) return 1;

        int row = emitterRow + dr[direction];
        int col = emitterCol + dc[direction];
        while (row >= 0 && row < rows && col >= 0 && col < cols) {
            if (board[row][col] == 'O') board[row][col] = 'M';
            row += dr[direction];
            col += dc[direction];
        }

        for (const string& line : board) g << line << '\n';
        g << '\n';
    }
    return 0;
}
