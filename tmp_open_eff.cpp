#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>
using namespace std;

/*
class Chip {
public:
    int size = 5;
    int blocks = 6;
    vector<vector<int>> wires = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0}, {0, 3}};

    int length(const vector<int>& pos) const {      
        int total = 0;
        for (auto& w : wires) {
            int a = pos[w[0]], b = pos[w[1]];
            total += abs(a % size - b % size) + abs(a / size - b / size);
        }
        return total;
    }

    void show(const vector<int>& pos) const {      
        for (int y = size - 1; y >= 0; --y) {
            for (int x = 0; x < size; ++x) {
                auto it = find(pos.begin(), pos.end(), y * size + x);
                if (it == pos.end()) cout << " .";
                else cout << ' ' << (it - pos.begin());
            }
            cout << "\n";
        }
    }
};

class EfficientPlace {
public:
    vector<int> run(const Chip& chip, int blocks, int tries) {
        mt19937 rng(42);
        vector<int> cells(chip.size * chip.size);
        for (int i = 0; i < (int)cells.size(); ++i) cells[i] = i; 

        vector<int> best;
        int best_len = 1000000;
        for (int t = 0; t < tries; ++t) {
            shuffle(cells.begin(), cells.end(), rng);             
            vector<int> pos(cells.begin(), cells.begin() + blocks); 
            int len = chip.length(pos);
            if (len < best_len) { best_len = len; best = pos; }
        }
        return best;
    }
};

int main() {
    Chip chip;
    vector<int> naive = {0, 1, 2, 3, 4, 5};       
    cout << "Подряд: " << chip.length(naive) << "\n";
    chip.show(naive);

    vector<int> best = EfficientPlace().run(chip, 6, 10000);
    cout << "\nПосле поиска: " << chip.length(best) << "\n";
    chip.show(best);
}


*/



class Chip {
    int size = 5; int block = 3; vector<vector<int>> wire = {{1, 2}, {2, 3}};
    public:
    int length(int len){
        return 1;
    }
    };

const int side = 42;
class Random_Placer{
    int unnalible = 0;
    public:
    int choise(const Chip& chip, int size, int block,  vector<vector<int>> wire){
    
    vector<int> best;
    int best_len = 1000000;
    
    for(int i = 0; i < chip.size; i++);
        cout << i - chip.size;
    };
};

int main() {
    Chip chip;
    vector<int> naive = {0, 1, 2, 3, 4, 5};       
};

