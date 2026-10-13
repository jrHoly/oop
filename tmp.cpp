#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <vector>
using namespace std;

const int T = 4, CELLS = 5;           // 4 макроса, 5 клеток
mt19937 rng(0);

using State = vector<int>;            // позиции уже стоящих макросов

struct Node { State state; int parent; int N = 0; double Q = -1e9; };
vector<Node> tree;                    // узлы дерева (в нулевом корень)
map<State, int> index_of;             // состояние -> индекс узла

int hpwl(const State& s) {            // сумма расстояний между соседями
    int sum = 0;
    for (size_t i = 0; i + 1 < s.size(); ++i) sum += abs(s[i] - s[i + 1]);
    return sum;
}

State rollout(State s) {              // псевдо-политика случайный выбор клетки
    while ((int)s.size() < T) s.push_back(rng() % CELLS);
    return s;
}

int pick_start() {                    // корень или лучший неполный узел по Q
    int best = 0;
    for (size_t i = 1; i < tree.size(); ++i)
        if ((int)tree[i].state.size() < T && tree[i].Q > tree[best].Q) best = i;
    return rng() % 2 ? 0 : best;
}

void one_iteration() {
    int node = pick_start();                       // дерево -> rollout: откуда стартовать
    State full = rollout(tree[node].state);        // rollout до конца
    double value = -hpwl(full);                    // чем меньше HPWL (это целевая метрика), тем лучше

    for (size_t k = tree[node].state.size(); k < full.size(); ++k) {   // rollout -> дерево: новые узлы
        State child(full.begin(), full.begin() + k + 1);
        if (!index_of.count(child)) {
            index_of[child] = tree.size();
            tree.push_back({child, node});
        }
        node = index_of[child];
    }
    for (int n = node; n != -1; n = tree[n].parent) {
        tree[n].N++;
        tree[n].Q = max(tree[n].Q, value);
    }
    cout << "HPWL=" << -value << "  Q корня=" << tree[0].Q << "  узлов=" << tree.size() << "\n";
}

int main() {
    tree.push_back({State{}, -1});
    index_of[State{}] = 0;
    for (int i = 0; i < 10; ++i) one_iteration();
}
