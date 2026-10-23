#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

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

    for (int i = 0; i < n; i++) {
        int r;
        if (!(f >> r) || r < 0) return 1;

        for (int row = 0; row <= 2 * r; row++) {
            int padding = abs(r - row);
            int cells = 2 * r + 1 - padding;

            g << string(padding, '-');
            for (int col = 0; col < cells; col++) {
                if (col > 0) g << '-';
                bool border = row == 0 || row == 2 * r
                           || col == 0 || col == cells - 1;
                g << (border ? '*' : 'O');
            }
            g << string(padding, '-') << '\n';
        }
        g << '\n';
    }
    return 0;
}
