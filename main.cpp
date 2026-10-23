#include <iostream>
#include <fstream>

using namespace std;

ifstream f("input.in");
ofstream g("output.out");

int main() {
    g << "Hello, World!" << endl;
    return 0;
}