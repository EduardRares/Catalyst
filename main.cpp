#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using Instruction = array<char, 6>;
const int DR[] = {0, 1, 1, 0, -1, -1};
const int DC[] = {2, 1, -1, -2, -1, 1};

Instruction idle() { return {'-', '-', '-', '-', '-', '-'}; }

int railDistance(int a, int b, int perimeter) {
    int d = abs(a - b);
    return min(d, perimeter - d);
}

char moveToward(int& position, int destination, int perimeter) {
    if (position == destination) return '-';
    int forward = (destination - position + perimeter) % perimeter;
    if (forward <= perimeter - forward) {
        position = (position + 1) % perimeter;
        return 'F';
    }
    position = (position + perimeter - 1) % perimeter;
    return 'B';
}

// Two nonparallel beams paint only their intersection. A walk restricted to
// target cells therefore creates no unwanted matter. Count actual covers,
// because a revisiting walk can submit a pattern before reaching its endpoint.
int productionLength(const vector<int>& walk, int cells, int required) {
    vector<bool> painted(cells, false);
    int covered = 0, submitted = 0, ticks = 0;
    bool backwards = false;
    while (submitted < required) {
        for (int j = 0; j < static_cast<int>(walk.size()); j++) {
            int cell = walk[backwards ? walk.size() - 1 - j : j];
            ticks++;
            if (!painted[cell]) { painted[cell] = true; covered++; }
            if (covered == cells) {
                submitted++;
                fill(painted.begin(), painted.end(), false);
                covered = 0;
                if (submitted == required) return ticks;
            }
        }
        backwards = !backwards;
    }
    return ticks;
}

struct Plan {
    int first = -1, second = -1, score = numeric_limits<int>::max();
    vector<int> walk;
};

vector<Instruction> solve(int radius, int required, const vector<string>& pattern) {
    if (required == 0) return {idle()};
    int rows = static_cast<int>(pattern.size());
    int cols = static_cast<int>(pattern[0].size());
    int perimeter = 6 * radius;
    vector<pair<int, int>> cells;
    for (int row = 0; row < rows; row++)
        for (int col = 0; col < cols; col++)
            if (pattern[row][col] == 'M') cells.push_back({row, col});
    int count = static_cast<int>(cells.size());
    if (count == 0) throw runtime_error("The pattern must contain matter.");

    // Clockwise rail, with emitter i initially at corner i * radius.
    const int railDr[] = {-1, 0, 1, 1, 0, -1};
    const int railDc[] = {1, 2, 1, -1, -2, -1};
    vector<vector<int>> railIndex(rows, vector<int>(cols, -1));
    int row = radius, col = 0, index = 0;
    for (int side = 0; side < 6; side++) {
        for (int step = 0; step < radius; step++) {
            railIndex[row][col] = index++;
            row += railDr[side];
            col += railDc[side];
        }
    }

    vector<vector<int>> entry(6, vector<int>(count));
    for (int direction = 0; direction < 6; direction++) {
        for (int cell = 0; cell < count; cell++) {
            row = cells[cell].first;
            col = cells[cell].second;
            while (railIndex[row][col] < 0) {
                row -= DR[direction];
                col -= DC[direction];
            }
            entry[direction][cell] = railIndex[row][col];
        }
    }

    Plan best;
    mt19937 random(42);
    // Opposite emitters share an axis. Compare all three axis pairs and all
    // four choices of actual emitters, including travel from their corners.
    for (int axisA = 0; axisA < 3; axisA++) {
        for (int axisB = axisA + 1; axisB < 3; axisB++) {
            vector<vector<int>> graph(count);
            for (int a = 0; a < count; a++) {
                for (int b = a + 1; b < count; b++) {
                    if (railDistance(entry[axisA][a], entry[axisA][b], perimeter) <= 1
                        && railDistance(entry[axisB][a], entry[axisB][b], perimeter) <= 1) {
                        graph[a].push_back(b);
                        graph[b].push_back(a);
                    }
                }
            }

            vector<vector<int>> distance(count, vector<int>(count, count + 1));
            for (int start = 0; start < count; start++) {
                queue<int> pending;
                pending.push(start);
                distance[start][start] = 0;
                while (!pending.empty()) {
                    int current = pending.front();
                    pending.pop();
                    for (int next : graph[current]) {
                        if (distance[start][next] == count + 1) {
                            distance[start][next] = distance[start][current] + 1;
                            pending.push(next);
                        }
                    }
                }
                if (find(distance[start].begin(), distance[start].end(), count + 1)
                    != distance[start].end()) throw runtime_error("The target must be connected.");
            }

            // Heuristic optimization: multiple nearest-neighbor paths, then
            // 2-opt improvements using distances within the target graph.
            vector<int> starts(count);
            iota(starts.begin(), starts.end(), 0);
            shuffle(starts.begin(), starts.end(), random);
            int attempts = 2 * min(count, 32);
            for (int attempt = 0; attempt < attempts; attempt++) {
                vector<int> order{starts[(attempt / 2) % count]};
                vector<bool> visited(count, false);
                visited[order[0]] = true;
                while (static_cast<int>(order.size()) < count) {
                    int next = -1, bestDistance = count + 1, bestDegree = count + 1;
                    for (int candidate = 0; candidate < count; candidate++) {
                        if (visited[candidate]) continue;
                        int d = distance[order.back()][candidate], degree = 0;
                        for (int neighbor : graph[candidate]) degree += !visited[neighbor];
                        if (d < bestDistance || (d == bestDistance && degree < bestDegree)
                            || (d == bestDistance && degree == bestDegree && random() % 2)) {
                            next = candidate;
                            bestDistance = d;
                            bestDegree = degree;
                        }
                    }
                    order.push_back(next);
                    visited[next] = true;
                }
                for (int pass = 0; pass < 6; pass++) {
                    bool improved = false;
                    for (int a = 0; a + 1 < count; a++) {
                        for (int b = a + 1; b < count; b++) {
                            int delta = 0;
                            if (a > 0) delta += distance[order[a - 1]][order[b]]
                                              - distance[order[a - 1]][order[a]];
                            if (b + 1 < count) delta += distance[order[a]][order[b + 1]]
                                                      - distance[order[b]][order[b + 1]];
                            if (delta < 0) {
                                reverse(order.begin() + a, order.begin() + b + 1);
                                improved = true;
                            }
                        }
                    }
                    if (!improved) break;
                }

                vector<int> walk{order[0]};
                fill(visited.begin(), visited.end(), false);
                visited[order[0]] = true;
                for (int target : order) {
                    if (visited[target]) continue;
                    while (walk.back() != target) {
                        int current = walk.back(), next = -1;
                        for (int neighbor : graph[current]) {
                            if (distance[neighbor][target] + 1 == distance[current][target]
                                && (next < 0 || (visited[next] && !visited[neighbor]))) next = neighbor;
                        }
                        walk.push_back(next);
                        visited[next] = true;
                    }
                }
                for (int orientation = 0; orientation < 2; orientation++) {
                    int production = productionLength(walk, count, required);
                    for (int flipA = 0; flipA < 2; flipA++) {
                        for (int flipB = 0; flipB < 2; flipB++) {
                            int a = axisA + 3 * flipA, b = axisB + 3 * flipB;
                            int setup = max(railDistance(a * radius, entry[a][walk[0]], perimeter),
                                            railDistance(b * radius, entry[b][walk[0]], perimeter));
                            if (setup + production < best.score)
                                best = {a, b, setup + production, walk};
                        }
                    }
                    reverse(walk.begin(), walk.end());
                }
            }
        }
    }

    vector<Instruction> program;
    int a = best.first, b = best.second;
    int positionA = a * radius, positionB = b * radius;
    int start = best.walk[0];
    while (positionA != entry[a][start] || positionB != entry[b][start]) {
        Instruction line = idle();
        line[a] = moveToward(positionA, entry[a][start], perimeter);
        line[b] = moveToward(positionB, entry[b][start], perimeter);
        program.push_back(line);
    }
    vector<bool> painted(count, false);
    int covered = 0, submitted = 0;
    bool firstTick = true, backwards = false;
    while (submitted < required) {
        for (int j = 0; j < static_cast<int>(best.walk.size()); j++) {
            int cell = best.walk[backwards ? best.walk.size() - 1 - j : j];
            Instruction line = idle();
            if (firstTick) {
                line[a] = line[b] = 'T';
                firstTick = false;
            } else {
                if (railDistance(positionA, entry[a][cell], perimeter) > 1
                    || railDistance(positionB, entry[b][cell], perimeter) > 1)
                    throw runtime_error("Invalid target walk.");
                line[a] = moveToward(positionA, entry[a][cell], perimeter);
                line[b] = moveToward(positionB, entry[b][cell], perimeter);
            }
            program.push_back(line);
            if (!painted[cell]) { painted[cell] = true; covered++; }
            if (covered == count) {
                submitted++;
                fill(painted.begin(), painted.end(), false);
                covered = 0;
                if (submitted == required) break;
            }
        }
        backwards = !backwards;
    }
    if (static_cast<int>(program.size()) != best.score) throw runtime_error("Program length mismatch.");
    return program;
}

int main(int argc, char* argv[]) {
    ifstream input(argc > 1 ? argv[1] : "input.in");
    ofstream output(argc > 2 ? argv[2] : "output.out");
    if (!input || !output) {
        cerr << "Could not open input or output file.\n";
        return 1;
    }
    try {
        int patterns;
        if (!(input >> patterns)) return 1;
        for (int i = 0; i < patterns; i++) {
            int radius, rows, cols, required;
            if (!(input >> radius >> rows >> cols >> required) || radius <= 0 || required < 0
                || rows != 2 * radius + 1 || cols != 4 * radius + 1) return 1;
            vector<string> pattern(rows);
            for (string& line : pattern)
                if (!(input >> line) || static_cast<int>(line.size()) != cols) return 1;
            vector<Instruction> program = solve(radius, required, pattern);
            for (const Instruction& line : program) {
                for (int emitter = 0; emitter < 6; emitter++) {
                    if (emitter) output << ' ';
                    output << line[emitter];
                }
                output << '\n';
            }
            output << '\n';
            cerr << "Pattern " << i + 1 << '/' << patterns << ": " << program.size() << " ticks\n";
        }
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
