#include "MyStrategy.h"

#include <iostream>
#include <vector>
#include <set>
#include <cmath>
#include <cstdlib>

using namespace model;
using namespace std;

#ifdef SLAVA_DEBUG
#define log(x) { cerr << x << endl; }
#else
#define log(x)
#endif

#define logId(x) log(move_index << " " << self.getId() << ": " << x)

typedef vector< vector< CellType > > Cells;
typedef Move Action;

const int inf = 1e9;
int sizeX;
int sizeY;

struct Point {
    int x, y;

    Point() {}
    Point(int x, int y): x(x), y(y) {}
    Point(const Unit& u): x(u.getX()), y(u.getY()) {}

    bool isCorrect() const {
        return 0 <= x && x < sizeX && 0 <= y && y < sizeY;
    }

    bool has_neigh(const Point &p) const {
        return p.isCorrect() && abs(x - p.x) + abs(y - p.y) == 1;
    }

    vector< Point > neighs() const {
        vector< Point > result;
        if (x - 1 >= 0)    { result.push_back(Point(x - 1, y)); }
        if (y - 1 >= 0)    { result.push_back(Point(x, y - 1)); }
        if (x + 1 < sizeX) { result.push_back(Point(x + 1, y)); }
        if (y + 1 < sizeY) { result.push_back(Point(x, y + 1)); }
        return result;
    }

    double distance_to(const Point& p) const {
        int xRange = p.x - x;
        int yRange = p.y - y;
        return sqrt((double) (xRange * xRange + yRange * yRange));
    }

    friend bool operator==(const Point& a, const Point& b) {
        return a.x == b.x && a.y == b.y;
    }

    friend bool operator!=(const Point& a, const Point& b) {
        return !(a == b);
    }

    friend bool operator<(const Point& a, const Point& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    }

    friend ostream& operator<<(ostream& out, const Point &p) {
        return out << p.x << " " << p.y;
    }
};

int random(int bound) { // [0, bound)
    return rand() % bound;
}

int random(int a, int b) { // [a, b)
    return a + random(b - a);
}

template< class T >
const T& random_choice(const vector< T >& ts) {
    return ts[random(ts.size())];
}

template< class T >
T& random_choice(vector< T >& ts) {
    return ts[random(ts.size())];
}

bool one_in(int x) {
    return random(x - 1) == 0;
}

Action make_action(ActionType action) {
    Action result;
    result.setAction(action);
    return result;
}

Action make_action(ActionType action, int x, int y) {
    Action result;
    result.setAction(action);
    result.setX(x);
    result.setY(y);
    return result;
}

Action make_action(ActionType action, const Point& p) {
    return make_action(action, p.x, p.y);
}

class Dijkstra {

    vector< vector< int   > > dist;
    vector< vector< Point > > prev;
    vector< vector< char  > > reached;

public:

    Dijkstra() {}

    Dijkstra(const Point& start, const Cells& cells):
            dist   (sizeX, vector< int   >(sizeY, inf)),
            prev   (sizeX, vector< Point >(sizeY)),
            reached(sizeX, vector< char  >(sizeY, false)) {
        dist[start.x][start.y] = 0;
        set< pair< int, Point > > q;
        q.insert(make_pair(0, start));
        while (!q.empty()) {
            auto it = q.begin();
            int d   = it->first;
            Point p = it->second;
            q.erase(it);
            if (reached[p.x][p.y]) {
                continue;
            }
            reached[p.x][p.y] = true;
            d += 1;
            for (auto& n : p.neighs()) {
                if (cells[n.x][n.y] == FREE
                        && !reached[n.x][n.y]
                        && d < dist[n.x][n.y]) {
                    dist[n.x][n.y] = d;
                    prev[n.x][n.y] = p;
                    q.insert(make_pair(d, n));
                }
            }
        }
        dist[start.x][start.y] = inf;
        reached[start.x][start.y] = false;
    }

    Point find_reachable(Point p) const {
        while (!reached[p.x][p.y]) {
            p.x += 1;
            if (p.x == sizeX) {
                p.x = 0;
                p.y += 1;
                if (p.y == sizeY) {
                    p.y = 0;
                }
            }
        }
        return p;
    }

    Point next(const Point& pos, Point target) const {
        while (prev[target.x][target.y] != pos) {
            target = prev[target.x][target.y];
        }
        return target;
    }

    bool has_reached(const Point& p) const {
        return reached[p.x][p.y];
    }

    Point get_prev(const Point& p) const {
        return prev[p.x][p.y];
    }

    int get_dist(const Point& p) const {
        return dist[p.x][p.y];
    }
};

Point target;
int move_index;

struct SlavaStrategy {

    const Trooper& self;
    const World& world;
    const Game& game;

    Cells cells;
    vector< Trooper > teammates;
    vector< Trooper > enemies;
    Dijkstra dijkstra;

    SlavaStrategy(const Trooper& self, const World& world,
            const Game& game): self(self), world(world), game(game) {

        move_index += 1;
        if (move_index == 1) {
            sizeX = world.getWidth();
            sizeY = world.getHeight();

            target = Point(sizeX / 2, sizeY / 2);
        }

        cells = world.getCells();
        for (auto& trooper : world.getTroopers()) {
            cells[trooper.getX()][trooper.getY()] = LOW_COVER;
            if (trooper.isTeammate()) {
                teammates.push_back(trooper);
            }
            else {
                enemies.push_back(trooper);
            }
        }

        dijkstra = Dijkstra(self, cells);
    }

    Action best_action;
    int best_score;
    Action cur_action;

    struct State {
        int mate_damage;
        int damage;
        Point pos;
        bool has_medkit;
        bool has_field_ration;
        bool has_grenade;
    };

    Action run() {
        int action_points = self.getActionPoints();

        logId(self.getType() << " (" << action_points << ") at " << Point(self));
        logId("we see " << enemies.size() << " enemies");

        best_score = -inf;
        cur_action = make_action(END_TURN);

        State state;
        state.mate_damage      = 0;
        state.damage           = 0;
        state.pos              = self;
        state.has_medkit       = self.isHoldingMedikit();
        state.has_field_ration = self.isHoldingFieldRation();
        state.has_grenade      = self.isHoldingGrenade();

        maximize_score(0, action_points, state);
        logId("best_score = " << best_score);
        return best_action;
    }

    void maximize_score(int action_number, const int action_points, State state) {
        action_number += 1;

        for (auto& bonus : world.getBonuses()) {
            if (state.pos == bonus) {
                if (bonus.getType() == MEDIKIT) {
                    state.has_medkit = true;
                }
                else if (bonus.getType() == FIELD_RATION) {
                    state.has_field_ration = true;
                }
                else if (bonus.getType() == GRENADE) {
                    state.has_grenade = true;
                }
            }
        }

        {
            int mates_dist = 0;
            for (auto& mate : teammates) {
                mates_dist += ceil(state.pos.distance_to(mate));
            }

            int target_dist = ceil(state.pos.distance_to(target));

            int score = 5 * (-target_dist)
                      + (-mates_dist)
                      + 30 * (-state.mate_damage)
                      + 20 * state.damage
                      + 2 * state.has_medkit
                      + 2 * state.has_field_ration
                      + 2 * state.has_grenade;

            if (score > best_score) {
                best_action = cur_action;
                best_score = score;
            }
        }

        {
            int points = action_points - game.getStandingMoveCost();
            if (points >= 0) {
                for (auto& n : state.pos.neighs()) {
                    if (cells[n.x][n.y] == FREE) {
                        State new_state = state;
                        new_state.pos = n;
                        if (action_number == 1) {
                            cur_action = make_action(MOVE, n);
                        }
                        maximize_score(action_number, points, new_state);
                    }
                }
            }
        }

        {
            int points = action_points - self.getShootCost();
            if (points >= 0) {
                for (auto& enemy : enemies) {
                    if (world.isVisible(self.getShootingRange(),
                                state.pos.x, state.pos.y, self.getStance(), // TODO: state.stance
                                enemy.getX(), enemy.getY(), enemy.getStance())) {
                        State new_state = state;
                        new_state.damage += self.getStandingDamage();
                        if (action_number == 1) {
                            cur_action = make_action(SHOOT, enemy);
                        }
                        maximize_score(action_number, points, new_state);
                    }
                }
            }
        }

        if (state.has_medkit) {
            int points = action_points - game.getMedikitUseCost();
            if (points >= 0) {
                for (auto& mate : teammates) {
                    if (!state.pos.has_neigh(mate)) {
                        continue;
                    }
                    int heal = min(
                            game.getMedikitBonusHitpoints(),
                            mate.getMaximalHitpoints() - mate.getHitpoints());
                    if (heal > 0) {
                        State new_state = state;
                        new_state.mate_damage -= heal;
                        new_state.has_medkit = false;
                        if (action_number == 1) {
                            cur_action = make_action(USE_MEDIKIT, mate);
                        }
                        maximize_score(action_number, points, new_state);
                    }
                }

                {
                    int heal = min(
                            game.getMedikitHealSelfBonusHitpoints(),
                            self.getMaximalHitpoints() - self.getHitpoints());
                    if (heal > 0) {
                        State new_state = state;
                        new_state.mate_damage -= heal;
                        new_state.has_medkit = false;
                        if (action_number == 1) {
                            cur_action = make_action(USE_MEDIKIT, self);
                        }
                        maximize_score(action_number, points, new_state);
                    }
                }
            }
        }

        if (self.getType() == FIELD_MEDIC) {
            int points = action_points - game.getFieldMedicHealCost();
            if (points >= 0) {
                for (auto& mate : teammates) {
                    if (!state.pos.has_neigh(mate)) {
                        continue;
                    }
                    int heal = min(
                            game.getFieldMedicHealBonusHitpoints(),
                            mate.getMaximalHitpoints() - mate.getHitpoints());
                    if (heal > 0) {
                        State new_state = state;
                        new_state.mate_damage -= heal;
                        if (action_number == 1) {
                            cur_action = make_action(HEAL, mate);
                        }
                        maximize_score(action_number, points, new_state);
                    }
                }

                {
                    int heal = min(
                            game.getFieldMedicHealSelfBonusHitpoints(),
                            self.getMaximalHitpoints() - self.getHitpoints());
                    if (heal > 0) {
                        State new_state = state;
                        new_state.mate_damage -= heal;
                        if (action_number == 1) {
                            cur_action = make_action(HEAL, self);
                        }
                        maximize_score(action_number, points, new_state);
                    }
                }
            }
        }
    }
};

MyStrategy::MyStrategy() {}

void MyStrategy::move(const Trooper& self,
        const World& world, const Game& game, Action& action) {
    SlavaStrategy strategy(self, world, game);
    action = strategy.run();
    logId("action = " << action.getAction() << " " << action.getX() << " " << action.getY());
}
