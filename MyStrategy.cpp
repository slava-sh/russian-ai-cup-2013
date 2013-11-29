#include "MyStrategy.h"

#include <iostream>
#include <vector>
#include <set>
#include <cmath>
#include <cstdlib>

using namespace model;
using namespace std;

#ifdef SLAVA_DEBUG
#include <ctime>
#define log_(x) { cerr << x << endl; }
#define log(x) log_("[" << (float) clock() / CLOCKS_PER_SEC << " " \
        << move_index << " " << self.getId() << "] " << x)
#else
#define log_(x)
#define log(x)
#endif

typedef vector< vector< CellType > > Cells;
typedef Move Action;

const int inf = 1e9;
int sizeX;
int sizeY;

ostream& operator<<(ostream& out, const ActionType& action) {
    switch (action) {
        case END_TURN:                  return out << "END_TURN";
        case MOVE:                      return out << "MOVE";
        case SHOOT:                     return out << "SHOOT";
        case RAISE_STANCE:              return out << "RAISE_STANCE";
        case LOWER_STANCE:              return out << "LOWER_STANCE";
        case THROW_GRENADE:             return out << "THROW_GRENADE";
        case USE_MEDIKIT:               return out << "USE_MEDIKIT";
        case EAT_FIELD_RATION:          return out << "EAT_FIELD_RATION";
        case HEAL:                      return out << "HEAL";
        case REQUEST_ENEMY_DISPOSITION: return out << "REQUEST_ENEMY_DISPOSITION";
        default:                        return out << "UNKNOWN_ACTION";
    };
}

ostream& operator<<(ostream& out, const TrooperType& type) {
    switch (type) {
        case COMMANDER:   return out << "COMMANDER";
        case FIELD_MEDIC: return out << "FIELD_MEDIC";
        case SOLDIER:     return out << "SOLDIER";
        case SNIPER:      return out << "SNIPER";
        case SCOUT:       return out << "SCOUT";
        default:          return out << "UNKNOWN_TROOPER";
    };
}

ostream& operator<<(ostream& out, const TrooperStance& stance) {
    switch (stance) {
        case PRONE:                  return out << "PRONE";
        case KNEELING:               return out << "KNEELING";
        case STANDING:               return out << "STANDING";
        case _TROOPER_STANCE_COUNT_: return out << "_TROOPER_STANCE_COUNT_";
        default:                     return out << "UNKNOWN_STANCE";
    };
}

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
        int kills;
        Point pos;
        TrooperStance stance;   // TODO: use stance
        bool has_medkit;
        bool has_field_ration;
        bool used_field_ration; // TODO: better method
        bool has_grenade;       // TODO: use grenades
        bool used_grenade;      // TODO: better method
    };

    Action run() {
        if (move_index == 0) {
            target = self;
        }
        while (min_distance(self, target) <= 5 ||
                min_distance(self, target) == inf) {
            target = Point(random(sizeX), random(sizeY));
            log("new target: " << target);
        }

        int action_points = self.getActionPoints();
        log(self.getType() << " " << self.getStance() << " (" << action_points << ") at " << Point(self));

        best_score = -inf;
        cur_action = make_action(END_TURN);

        State state;
        state.mate_damage       = 0;
        state.damage            = 0;
        state.kills             = 0;
        state.pos               = self;
        state.stance            = self.getStance();
        state.has_medkit        = self.isHoldingMedikit();
        state.has_field_ration  = self.isHoldingFieldRation();
        state.used_field_ration = false;
        state.has_grenade       = self.isHoldingGrenade();
        state.used_grenade      = false;

        maximize_score(0, action_points, state);
        log("best_score = " << best_score);
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
                    if (!state.used_grenade) {
                        state.has_grenade = true;
                    }
                }
            }
        }

        {
            int mates_dist = 0;
            bool close_to_commander = false;
            for (auto& mate : teammates) {
                mates_dist += min_distance(state.pos, mate);
                if (mate.getType() == COMMANDER &&
                        state.pos.distance_to(mate) <= game.getCommanderAuraRange()) {
                    close_to_commander = true;
                }
            }

            int shooting_enemies = 0;
            for (auto& enemy : enemies) {
                if (world.isVisible(enemy.getShootingRange(),
                            enemy.getX(), enemy.getY(), STANDING,
                            state.pos.x, state.pos.y, state.stance)) {
                    shooting_enemies += 1;
                }
            }

            int target_dist = min_distance(state.pos, target);

            int score = 0;
            score -= 40   * state.mate_damage;
            score += 30   * state.damage;
            score += 2000 * state.kills;
            score -= 1000 * shooting_enemies;
            score -= 2    * mates_dist;
            score += 40   * state.has_medkit;
            score += 40   * state.has_field_ration;
            score += 40   * state.has_grenade;
            if (state.mate_damage >= 0 && state.damage == 0) {
                score -= 5 * target_dist;
            }
            if (self.getType() != SCOUT) {
                score += 50 * close_to_commander;
            }

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
            int points = action_points - game.getStanceChangeCost();
            if (points >= 0) {
                if (state.stance != STANDING) {
                    State new_state = state;
                    new_state.stance = state.stance == PRONE ? KNEELING : STANDING;
                    if (action_number == 1) {
                        cur_action = make_action(RAISE_STANCE);
                    }
                    maximize_score(action_number, points, new_state);
                }
                if (state.stance != PRONE) {
                    State new_state = state;
                    new_state.stance = state.stance == STANDING ? KNEELING : PRONE;
                    if (action_number == 1) {
                        cur_action = make_action(LOWER_STANCE);
                    }
                    maximize_score(action_number, points, new_state);
                }
            }
        }

        {
            int points = action_points - self.getShootCost();
            if (points >= 0) {
                for (auto& enemy : enemies) {
                    if (world.isVisible(self.getShootingRange(),
                                state.pos.x, state.pos.y, state.stance,
                                enemy.getX(), enemy.getY(), enemy.getStance())) {
                        State new_state = state;
                        int damage = self.getDamage(state.stance);
                        new_state.damage += damage;
                        if (damage >= enemy.getHitpoints()) {
                            new_state.kills += 1;
                        }
                        if (action_number == 1) {
                            cur_action = make_action(SHOOT, enemy);
                        }
                        maximize_score(action_number, points, new_state);
                    }
                }
            }
        }

        {
            int cost =
                state.stance == STANDING ? game.getStandingMoveCost() :
                (state.stance == KNEELING ? game.getKneelingMoveCost() :
                 game.getProneMoveCost());
            int points = action_points - cost;
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

        if (state.has_grenade) {
            int points = action_points - game.getGrenadeThrowCost();
            if (points >= 0) {
                for (auto& enemy : enemies) {
                    Point e(enemy);
                    if (state.pos.distance_to(e) <= game.getGrenadeThrowRange()) {
                        State new_state = state;
                        {
                            int damage = game.getGrenadeDirectDamage();
                            new_state.damage += game.getGrenadeDirectDamage();
                            if (damage >= enemy.getHitpoints()) {
                                new_state.kills += 1;
                            }
                        }
                        for (auto& n : enemies) {
                            if (e.has_neigh(n)) {
                                int damage = game.getGrenadeCollateralDamage();
                                new_state.damage += damage;
                                if (damage >= n.getHitpoints()) {
                                    new_state.kills += 1;
                                }
                            }
                        }
                        for (auto& n : teammates) {
                            if (e.has_neigh(n)) {
                                int damage = game.getGrenadeCollateralDamage();
                                new_state.mate_damage += damage;
                                if (damage >= n.getHitpoints()) {
                                    new_state.kills -= 2;
                                }
                            }
                        }
                        new_state.has_grenade = false;
                        new_state.used_grenade = true;
                        if (action_number == 1) {
                            cur_action = make_action(THROW_GRENADE, e);
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
        log("floyd start");

        vector< Point > free_points;
        for (int x = 0; x < sizeX; x += 1) {
            for (int y = 0; y < sizeY; y += 1) {
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

        log("floyd end");
    }
};

MyStrategy::MyStrategy() {
#ifdef SLAVA_DEBUG
    cerr.setf(ios_base::fixed);
    cerr.precision(3);
#endif
}

void MyStrategy::move(const Trooper& self,
        const World& world, const Game& game, Action& action) {
    SlavaStrategy strategy(self, world, game);
    action = strategy.run();
    log("action = " << action.getAction() << " " << action.getX() << " " << action.getY());
}
