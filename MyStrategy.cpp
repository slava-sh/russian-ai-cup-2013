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

Point target;
int move_index = -1;
vector< vector< vector< vector< int > > > > floyd_dist;

int min_distance(const Point& a, const Point& b) {
    return floyd_dist[a.x][a.y][b.x][b.y];
}

struct SlavaStrategy {

    const Trooper& self;
    const World& world;
    const Game& game;

    Cells cells;
    vector< Trooper > teammates;
    vector< Trooper > enemies;

    SlavaStrategy(const Trooper& self, const World& world,
            const Game& game): self(self), world(world), game(game) {

        move_index += 1;
        if (move_index == 0) {
            sizeX = world.getWidth();
            sizeY = world.getHeight();
            floyd();
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
        bool used_field_ration; // TODO: better method
        bool has_grenade;
    };

    Action run() {
        if (move_index == 0 || min_distance(self, target) <= 5) {
            if (!enemies.empty()) {
                target = enemies.front();
            }
            else {
                target = Point(random(sizeX), random(sizeY));
            }
            log("new target: " << target);
        }

        int action_points = self.getActionPoints();

        logId(self.getType() << " (" << action_points << ") at " << Point(self));
        logId("we see " << enemies.size() << " enemies");

        best_score = -inf;
        cur_action = make_action(END_TURN);

        State state;
        state.mate_damage       = 0;
        state.damage            = 0;
        state.pos               = self;
        state.has_medkit        = self.isHoldingMedikit();
        state.has_field_ration  = self.isHoldingFieldRation();
        state.used_field_ration = false;
        state.has_grenade       = self.isHoldingGrenade();

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
                    if (!state.used_field_ration) {
                        state.has_field_ration = true;
                    }
                }
                else if (bonus.getType() == GRENADE) {
                    state.has_grenade = true;
                }
            }
        }

        {
            int commander_dist = 0;
            int mates_dist = 0;
            for (auto& mate : teammates) {
                int dist = min_distance(state.pos, mate);
                mates_dist += dist;
                if (mate.getType() == COMMANDER) {
                    commander_dist = dist;
                }
            }

            int target_dist = min_distance(state.pos, target);

            int score = 5 * (-target_dist)
                      + (-mates_dist)
                      + 2 * (-commander_dist)
                      + 40 * (-state.mate_damage)
                      + 30 * state.damage
                      + 5 * state.has_medkit
                      + 5 * state.has_field_ration
                      + 5 * state.has_grenade;

            if (score > best_score) {
                best_action = cur_action;
                best_score = score;
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

        if (state.has_field_ration) {
            int points = action_points - game.getFieldRationEatCost();
            if (points >= 0) {
                State new_state = state;
                points += game.getFieldRationBonusActionPoints();
                new_state.has_field_ration = false;
                new_state.used_field_ration = true;
                if (action_number == 1) {
                    cur_action = make_action(EAT_FIELD_RATION);
                }
                maximize_score(action_number, points, new_state);
            }
        }
    }

    void floyd() {
        vector< Point > free_points;
        for (int x = 0; x < sizeX; ++x) {
            for (int y = 0; y < sizeY; ++y) {
                if (world.getCells()[x][y] == FREE) {
                    free_points.push_back(Point(x, y));
                }
            }
        }

        floyd_dist.resize(sizeX, vector< vector< vector< int > > >(sizeY,
                    vector< vector< int > >(sizeX, vector< int >(sizeY, inf))));
#define at2(t, p, q) (t[p.x][p.y][q.x][q.y])
        for (auto& p : free_points) {
            for (auto& n : p.neighs()) {
                if (world.getCells()[n.x][n.y] != FREE) {
                    continue;
                }
                at2(floyd_dist, p, n) = 1;
            }
        }
        for (auto& k : free_points) {
            for (auto& i : free_points) {
                if (at2(floyd_dist, i, k) == inf) {
                    continue;
                }
                for (auto& j : free_points) {
                    if (at2(floyd_dist, k, j) == inf) {
                        continue;
                    }
                    at2(floyd_dist, i, j) = min(at2(floyd_dist, i, j), at2(floyd_dist, i, k) + at2(floyd_dist, k, j));
                }
            }
        }
#undef at2
    }
};

MyStrategy::MyStrategy() {}

void MyStrategy::move(const Trooper& self,
        const World& world, const Game& game, Action& action) {
    SlavaStrategy strategy(self, world, game);
    action = strategy.run();
    logId("action = " << action.getAction() << " " << action.getX() << " " << action.getY());
}
