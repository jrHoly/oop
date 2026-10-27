#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>
using namespace std;

// =====================================================================
// Rect — осевой прямоугольник (геометрия)
// Используется для областей ограничений и для габаритов блоков
// overlaps()   — есть ли пересечение по обеим осям (касание не считается)
// contains(o)  — целиком ли o лежит внутри this
// =====================================================================
struct Rect {
    double x = 0, y = 0, w = 0, h = 0;
    bool overlaps(const Rect& o) const {
        return !(x + w <= o.x || o.x + o.w <= x ||
                 y + h <= o.y || o.y + o.h <= y);
    }
    bool contains(const Rect& o) const {
        return o.x >= x && o.y >= y && o.x + o.w <= x + w && o.y + o.h <= y + h;
    }
};

// =====================================================================
// Block — базовый блок предметной области (single inheritance)
// Хранит общие для всех блоков поля: id, размеры, флаг фиксации
// (fixed — блок не двигаем, например, IO), список допустимых поворотов
// (0/90/180/270). kind() — для отладки/логов
// =====================================================================
class Block {
public:
    int    id   = 0;
    double width = 1, height = 1;
    bool   fixed = false;
    vector<int> allowed_orientations = {0, 90, 180, 270};

    virtual ~Block() = default;
    virtual string kind() const { return "Block"; }
};

// =====================================================================
// Macro — макроблок (SRAM, IP, ядро). Наследник Block (single inheritance),
// добавляет pin_count — число пинов (пока для модели, в расчетах не участвует)
// =====================================================================
class Macro : public Block {                      
public:
    int pin_count = 0;                            
    string kind() const override { return "Macro"; }
};

// =====================================================================
// Constraint — абстрактное ограничение размещения (single inheritance)
// violatedBy(r) — нарушает ли прямоугольник r это ограничение
// describe()    — короткое имя для логов/вывода
// Все конкретные ограничения живут в vector<shared_ptr<Constraint>> в Design,
// поэтому Placement::isLegal не знает, какие именно ограничения есть, а просто
// прогоняет их через violatedBy
// =====================================================================
class Constraint {
public:
    virtual ~Constraint() = default;
    virtual bool violatedBy(const Rect& r) const = 0;
    virtual string describe() const = 0;
};

// =====================================================================
// Blockage — запрещенная зона. Нарушение, если блок пересекает область
// =====================================================================
class Blockage : public Constraint {              
    Rect area_;
public:
    explicit Blockage(Rect a) : area_(a) {}
    bool violatedBy(const Rect& r) const override { return r.overlaps(area_); }
    string describe() const override { return "Blockage"; }
};

// =====================================================================
// Fence — разрешенная зона. Нарушение, если блок не лежит целиком внутри
// =====================================================================
class Fence : public Constraint {                
    Rect area_;
public:
    explicit Fence(Rect a) : area_(a) {}
    bool violatedBy(const Rect& r) const override { return !area_.contains(r); }
    string describe() const override { return "Fence"; }
};

// =====================================================================
// Halo — отступ вокруг блока. Нарушение, если расширенный на width_
// прямоугольник блока пересекается с ним самим (т.е. width_ > 0)
// На практике halo должен проверяться между блоками; здесь это пока как
// самостоятельное ограничение для демонстрации иерархии Constraint
// =====================================================================
class Halo : public Constraint {              
    double width_;
public:
    explicit Halo(double w) : width_(w) {}
    bool violatedBy(const Rect& r) const override {
        Rect rr{r.x - width_, r.y - width_, r.w + 2 * width_, r.h + 2 * width_};
        return rr.overlaps(r);
    }
    string describe() const override { return "Halo"; }
};

// =====================================================================
// Net — гиперребро (сеть соединений). Хранит любое число пинов (id
// макроблоков) и weight — коэффициент важности этой сети в целевой
// функции (множитель ее HPWL в CostFunction; при weight=1 это обычный HPWL)
// =====================================================================
struct Net {
    vector<int> pins;
    double weight = 1.0;
};

// =====================================================================
// Design — модель задачи размещения.
// Содержит сетку (grid_cols × grid_rows), макроблоки (полиморфно,
// vector<shared_ptr<Macro>>), соединения (Net) и ограничения
// (Blockage/Fence/Halo — через общий базовый Constraint)
// Design ничего не знает про алгоритмы размещения
// =====================================================================
class Design {
public:
    int grid_cols = 0, grid_rows = 0;
    vector<shared_ptr<Macro>>      macros;      
    vector<Net>                    nets;
    vector<shared_ptr<Constraint>> constraints; 

    int macroCount() const { return (int)macros.size(); }
};

// =====================================================================
// Placement — результат размещения
// Хранит только координаты x, y и поворот каждого блока
// rectOf(d,i) — прямоугольник блока i с учётом поворота (90/270 меняют
// местами width/height). isLegal(d) — проверка границ сетки, всех
// Constraint через violatedBy и попарных наложений блоков
// hpwl(d) — суммарная длина соединений (взвешенная по net.weight),
// по центрам прямоугольников блоков
// =====================================================================
class Placement {
public:
    vector<double> x, y;
    vector<int>    orientation;

    void init(const Design& d) {
        x.assign(d.macroCount(), 0.0);
        y.assign(d.macroCount(), 0.0);
        orientation.assign(d.macroCount(), 0);
    }

    Rect rectOf(const Design& d, int i) const {
        const auto& m = *d.macros[i];
        bool swapWH = (orientation[i] % 180) != 0;
        double w = swapWH ? m.height : m.width;
        double h = swapWH ? m.width  : m.height;
        return {x[i], y[i], w, h};
    }

    bool isLegal(const Design& d) const {
        for (int i = 0; i < d.macroCount(); ++i) {
            Rect ri = rectOf(d, i);
            if (ri.x < 0 || ri.y < 0 ||
                ri.x + ri.w > d.grid_cols || ri.y + ri.h > d.grid_rows)
                return false;
            for (const auto& c : d.constraints)
                if (c->violatedBy(ri)) return false;
            for (int j = i + 1; j < d.macroCount(); ++j)
                if (ri.overlaps(rectOf(d, j))) return false;
        }
        return true;
    }

    double hpwl(const Design& d) const {
        double total = 0.0;
        for (const auto& net : d.nets) {
            if (net.pins.empty()) continue;
            double xmin = 1e18, ymin = 1e18, xmax = -1e18, ymax = -1e18;
            for (int p : net.pins) {
                Rect r = rectOf(d, p);
                double cx = r.x + r.w / 2.0, cy = r.y + r.h / 2.0;
                xmin = min(xmin, cx); xmax = max(xmax, cx);
                ymin = min(ymin, cy); ymax = max(ymax, cy);
            }
            total += net.weight * ((xmax - xmin) + (ymax - ymin));
        }
        return total;
    }
};

// =====================================================================
// CostFunction — абстрактная целевая функция (single inheritance)
// evaluate(d,p) — численная оценка размещения p для дизайна d (чем меньше,
// тем лучше). name() — имя для логов. Все плейсеры работают через этот
// интерфейс и не зависят от конкретной метрики
// =====================================================================
class CostFunction {
public:
    virtual ~CostFunction() = default;
    virtual double evaluate(const Design& d, const Placement& p) const = 0;
    virtual string name() const = 0;
};

// =====================================================================
// HPWLCost — простая целевая функция: суммарная HPWL (уже взвешенная
// по net.weight). Делегирует расчет в Placement::hpwl
// =====================================================================
class HPWLCost : public CostFunction {
public:
    double evaluate(const Design& d, const Placement& p) const override {
        return p.hpwl(d);
    }
    string name() const override { return "HPWL"; }
};

// =====================================================================
// CompositeCost — взвешенная сумма произвольных CostFunction
// Принимает vector<pair<CostFunction*, weight>> и считает Σ w_i·f_i(d,p).
// Сама ничего не знает про плотность/перегрузку: любую новую метрику
// достаточно реализовать как отдельный CostFunction и добавить ее в terms_
// =====================================================================
class CompositeCost : public CostFunction {
    vector<pair<shared_ptr<CostFunction>, double>> terms_;
public:
    explicit CompositeCost(vector<pair<shared_ptr<CostFunction>, double>> t)
        : terms_(std::move(t)) {}
    double evaluate(const Design& d, const Placement& p) const override {
        double s = 0;
        for (auto& [f, w] : terms_) s += w * f->evaluate(d, p);
        return s;
    }
    string name() const override { return "Composite"; }
};

// =====================================================================
// MoveGenerator — абстрактный «ход» для локального поиска / SA (single
// inheritance). apply() как-то модифицирует Placement p (сдвиг, своп,
// поворот). алгоритм не знает, какие именно ходы ему подсунули
// =====================================================================
class MoveGenerator {
public:
    virtual ~MoveGenerator() = default;
    virtual void apply(Placement& p, const Design& d, mt19937& rng) = 0;
};

// =====================================================================
// SwapMove — обмен позициями и поворотами двух случайных незафиксированных
// блоков
// =====================================================================
class SwapMove : public MoveGenerator {
public:
    void apply(Placement& p, const Design& d, mt19937& rng) override {
        if (d.macroCount() < 2) return;
        uniform_int_distribution<int> di(0, d.macroCount() - 1);
        int i = di(rng), j = di(rng);
        if (i == j || d.macros[i]->fixed || d.macros[j]->fixed) return;
        swap(p.x[i], p.x[j]); swap(p.y[i], p.y[j]);
        swap(p.orientation[i], p.orientation[j]);
    }
};

// =====================================================================
// ShiftMove — сдвиг одного случайного незафиксированного блока на
// ±1 по каждой оси (координаты зажимаются в границы сетки)
// =====================================================================
class ShiftMove : public MoveGenerator {
public:
    void apply(Placement& p, const Design& d, mt19937& rng) override {
        if (d.macroCount() < 1) return;
        uniform_int_distribution<int> di(0, d.macroCount() - 1);
        int i = di(rng);
        if (d.macros[i]->fixed) return;
        uniform_int_distribution<int> step(-1, 1);
        p.x[i] = clamp(p.x[i] + step(rng), 0.0, double(d.grid_cols));
        p.y[i] = clamp(p.y[i] + step(rng), 0.0, double(d.grid_rows));
    }
};

// =====================================================================
// RotateMove — выбор случайного незафиксированного блока и поворот в
// одну из его allowed_orientations
// =====================================================================
class RotateMove : public MoveGenerator {
public:
    void apply(Placement& p, const Design& d, mt19937& rng) override {
        if (d.macroCount() < 1) return;
        uniform_int_distribution<int> di(0, d.macroCount() - 1);
        int i = di(rng);
        if (d.macros[i]->fixed) return;
        const auto& ao = d.macros[i]->allowed_orientations;
        if (ao.size() < 2) return;
        uniform_int_distribution<int> oi(0, (int)ao.size() - 1);
        p.orientation[i] = ao[oi(rng)];
    }
};

// =====================================================================
// IPlacer — общий интерфейс алгоритма размещения (single inheritance)
// place(d,p) заполняет Placement p для дизайна d; name() — имя для логов
// и сравнения плейсеров в одном прогоне
// =====================================================================
class IPlacer {
public:
    virtual ~IPlacer() = default;
    virtual void place(const Design& d, Placement& p) = 0;
    virtual string name() const = 0;
};

// =====================================================================
// RandomPlacer — базовый случайный плейсер
// Инициализирует Placement случайными координатами и ориентациями из
// allowed_orientations. Использует std::mt19937 с фиксированным seed чтоб
// результат воспроизводим. Служит baseline и стартовой точкой для
// остальных плейсеров
// ====================================================================
class RandomPlacer : public IPlacer {
    mt19937 rng_;
public:
    explicit RandomPlacer(unsigned seed = 42) : rng_(seed) {}
    void place(const Design& d, Placement& p) override {
        p.init(d);
        uniform_int_distribution<int> rx(0, max(0, d.grid_cols - 1));
        uniform_int_distribution<int> ry(0, max(0, d.grid_rows - 1));
        for (int i = 0; i < d.macroCount(); ++i) {
            if (d.macros[i]->fixed) continue;
            p.x[i] = rx(rng_);
            p.y[i] = ry(rng_);
            const auto& ao = d.macros[i]->allowed_orientations;
            uniform_int_distribution<int> oi(0, (int)ao.size() - 1);
            p.orientation[i] = ao[oi(rng_)];
        }
    }
    string name() const override { return "RandomPlacer"; }
};

// =====================================================================
// SimulatedAnnealingPlacer — имитация отжига
// Композиция: CostFunction + набор MoveGenerator. Параметры: T0 —
// начальная температура, cooling — коэффициент остывания, iters_per_T —
// число ходов на шаг. Недопустимые (не isLegal) кандидаты отбрасываются.
// Хранится лучшее найденное размещение (best) и возвращается по завершении.
// Стартует из RandomPlacer для воспроизводимости
// =====================================================================
class SimulatedAnnealingPlacer : public IPlacer {
    shared_ptr<CostFunction> cost_;
    vector<shared_ptr<MoveGenerator>> moves_;
    double T0_, cooling_;
    int iters_per_T_;
    unsigned seed_;
public:
    SimulatedAnnealingPlacer(shared_ptr<CostFunction> cost,
                             vector<shared_ptr<MoveGenerator>> moves,
                             double T0 = 10.0, double cooling = 0.95,
                             int iters_per_T = 100, unsigned seed = 42)
        : cost_(std::move(cost)), moves_(std::move(moves)),
          T0_(T0), cooling_(cooling), iters_per_T_(iters_per_T), seed_(seed) {}

    void place(const Design& d, Placement& p) override {
        mt19937 rng(seed_);
        RandomPlacer init(seed_); init.place(d, p);
        double cur = cost_->evaluate(d, p);
        Placement best = p; double bestCost = cur;
        double T = T0_;
        uniform_real_distribution<double> real(0.0, 1.0);
        uniform_int_distribution<int> mi(0, (int)moves_.size() - 1);
        for (int round = 0; round < 100 && T > 1e-3; ++round, T *= cooling_) {
            for (int it = 0; it < iters_per_T_; ++it) {
                Placement cand = p;
                moves_[mi(rng)]->apply(cand, d, rng);
                if (!cand.isLegal(d)) continue;
                double c = cost_->evaluate(d, cand);
                if (c < cur || real(rng) < exp((cur - c) / T)) {
                    p = std::move(cand); cur = c;
                    if (c < bestCost) { bestCost = c; best = p; }
                }
            }
        }
        p = std::move(best);
    }
    string name() const override { return "SimulatedAnnealingPlacer"; }
};

// =====================================================================
// AnalyticalPlacer — упрощённый «аналитический» покоординатный спуск.
// Стартует из RandomPlacer, затем для каждого незафиксированного блока
// пробует шаги по ±x/±y (несколько масштабов) и оставляет те, что
// уменьшают cost_->evaluate(). Работает с любой CostFunction, без
// градиента и без отдельного штрафа за плотность — «аналитический»
// по духу (итеративное улучшение целевой функции), не по методу.
// =====================================================================
class AnalyticalPlacer : public IPlacer {
    shared_ptr<CostFunction> cost_; int max_iters_;
public:
    explicit AnalyticalPlacer(shared_ptr<CostFunction> cost, int max_iters = 50)
        : cost_(std::move(cost)), max_iters_(max_iters) {}

    void place(const Design& d, Placement& p) override {
        RandomPlacer init(7); init.place(d, p);
        double cur = cost_->evaluate(d, p);
        for (int it = 0; it < max_iters_; ++it) {
            bool improved = false;
            for (int i = 0; i < d.macroCount(); ++i) {
                if (d.macros[i]->fixed) continue;
                for (double step : {4.0, 1.0, 0.25})
                    for (auto [dx, dy] : vector<pair<double,double>>{
                             {step,0},{-step,0},{0,step},{0,-step}}) {
                        Placement cand = p;
                        cand.x[i] += dx; cand.y[i] += dy;
                        if (!cand.isLegal(d)) continue;
                        double c = cost_->evaluate(d, cand);
                        if (c < cur) { p = std::move(cand); cur = c; improved = true; }
                    }
            }
            if (!improved) break;
        }
    }
    string name() const override { return "AnalyticalPlacer"; }
};

// =====================================================================
// HierarchicalPlacer — трёхфазный плейсер.
//   1) кластеризация: union-find по Net, блоки из одной связной
//      компоненты считаются «группой» (упрощение dataflow-группировки);
//   2) грубое размещение: SimulatedAnnealingPlacer по всем блокам;
//   3) детализация: локальный покоординатный спуск с малыми шагами.
// Кластеры явно не используются в SA (упрощение), но этап 1 задаёт
// структуру для будущего группового размещения.
// =====================================================================
class HierarchicalPlacer : public IPlacer {
    shared_ptr<CostFunction> cost_;
public:
    explicit HierarchicalPlacer(shared_ptr<CostFunction> cost) : cost_(std::move(cost)) {}
    void place(const Design& d, Placement& p) override {
        vector<int> parent(d.macroCount());
        iota(parent.begin(), parent.end(), 0);
        function<int(int)> find = [&](int v) {
            return parent[v] == v ? v : parent[v] = find(parent[v]);
        };
        for (const auto& n : d.nets)
            for (size_t k = 1; k < n.pins.size(); ++k) {
                int a = find(n.pins[0]), b = find(n.pins[k]);
                if (a != b) parent[a] = b;
            }
        vector<shared_ptr<MoveGenerator>> moves = {
            make_shared<SwapMove>(), make_shared<ShiftMove>(), make_shared<RotateMove>()
        };
        SimulatedAnnealingPlacer coarse(cost_, moves, 20.0, 0.9, 200, 123);
        coarse.place(d, p);
        double cur = cost_->evaluate(d, p);
        for (int it = 0; it < 30; ++it) {
            bool improved = false;
            for (int i = 0; i < d.macroCount(); ++i) {
                if (d.macros[i]->fixed) continue;
                for (double step : {1.0, 0.25})
                    for (auto [dx, dy] : vector<pair<double,double>>{
                             {step,0},{-step,0},{0,step},{0,-step}}) {
                        Placement cand = p;
                        cand.x[i] += dx; cand.y[i] += dy;
                        if (!cand.isLegal(d)) continue;
                        double c = cost_->evaluate(d, cand);
                        if (c < cur) { p = std::move(cand); cur = c; improved = true; }
                    }
            }
            if (!improved) break;
        }
    }
    string name() const override { return "HierarchicalPlacer"; }
};

int main() {
    Design d;
    d.grid_cols = 15; d.grid_rows = 15;

    for (int i = 0; i < 5; ++i) {
        auto m = make_shared<Macro>();
        m->id = i; m->width = 2; m->height = 2; m->pin_count = 2;
        d.macros.push_back(m);
    }
    d.nets = {
        {{0, 1, 2}, 1.0},
        {{2, 3, 4}, 1.0},
        {{0, 4},    1.5},
    };

    d.constraints.push_back(make_shared<Halo>(0.5));
    d.constraints.push_back(make_shared<Blockage>(Rect{0, 0, 2, 2}));
    d.constraints.push_back(make_shared<Fence>(Rect{0, 0, 15, 15}));

    cout << "Constraints:";
    for (auto& c : d.constraints) cout << ' ' << c->describe();
    cout << "\n";

    auto cost = make_shared<HPWLCost>();

    RandomPlacer rp(1);    Placement p1; rp.place(d, p1);
    cout << rp.name() << " HPWL=" << p1.hpwl(d) << " legal=" << p1.isLegal(d) << "\n";

    vector<shared_ptr<MoveGenerator>> moves = {
        make_shared<SwapMove>(), make_shared<ShiftMove>(), make_shared<RotateMove>()
    };
    SimulatedAnnealingPlacer sa(cost, moves, 5.0, 0.95, 200, 42);
    Placement p2; sa.place(d, p2);
    cout << sa.name() << " HPWL=" << p2.hpwl(d) << " legal=" << p2.isLegal(d) << "\n";

    AnalyticalPlacer an(cost); Placement p3; an.place(d, p3);
    cout << an.name() << " HPWL=" << p3.hpwl(d) << " legal=" << p3.isLegal(d) << "\n";

    HierarchicalPlacer hi(cost); Placement p4; hi.place(d, p4);
    cout << hi.name() << " HPWL=" << p4.hpwl(d) << " legal=" << p4.isLegal(d) << "\n";

    return 0;
}