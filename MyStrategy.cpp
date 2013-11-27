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

#define logId(x) log(self.getId() << ": " << x)

typedef vector< vector< CellType > > Cells;

const int inf = 1e9;
int sizeX;
int sizeY;
int move_index = 0;

struct Point {
    int x, y;

    Point() {}
    Point(int x, int y): x(x), y(y) {}
    Point(const Unit& u): x(u.getX()), y(u.getY()) {}

    bool isCorrect() const {
        return 0 <= x && x < sizeX && 0 <= y && y < sizeY;
    }

    bool has_neigh(const Point &p) {
        return p.isCorrect() && abs(x - p.x) + abs(y - p.y) == 1;
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

struct Dijkstra {
    vector< vector< int   > > dist;
    vector< vector< Point > > prev;
    vector< vector< char  > > reached;

    Dijkstra(const Point& start, const Cells& cells):
            dist(sizeX, vector< int   >(sizeY, inf)),
            prev(sizeX, vector< Point >(sizeY)),
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
            add(cells, q, p, d + 1, Point(p.x + 1, p.y));
            add(cells, q, p, d + 1, Point(p.x - 1, p.y));
            add(cells, q, p, d + 1, Point(p.x, p.y + 1));
            add(cells, q, p, d + 1, Point(p.x, p.y - 1));
        }
        reached[start.x][start.y] = false;
    }

    void add(const Cells& cells, set< pair< int, Point > >& q,
            const Point& cur, int d, const Point& p) {
        if (p.isCorrect()
                && cells[p.x][p.y] == FREE
                && !reached[p.x][p.y]
                && d < dist[p.x][p.y]) {
            dist[p.x][p.y] = d;
            prev[p.x][p.y] = cur;
            q.insert(make_pair(d, p));
        }
    }

    Point find_reachable(Point p) {
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
};

MyStrategy::MyStrategy() {
}

Point target;

void MyStrategy::move(const Trooper& self,
        const World& world, const Game& game, Move& action) {

    move_index += 1;

    if (move_index == 1) {
        sizeX = world.getWidth();
        sizeY = world.getHeight();
    }

    Point pos(self);
    int action_points = self.getActionPoints();
    logId("move number " << move_index);
    logId(self.getType() << " (" << action_points << ") at " << pos);

    auto& bonuses = world.getBonuses();
    auto& troopers = world.getTroopers();
    auto cells = world.getCells();
    vector< Trooper > teammates;
    vector< Trooper > enemies;
    for (auto& trooper : troopers) {
        if (trooper.isTeammate()) {
            cells[trooper.getX()][trooper.getY()] = HIGH_COVER;
            teammates.push_back(trooper);
        }
        else {
            enemies.push_back(trooper);
        }
    }
    logId("we see " << enemies.size() << " enemies");

    if (self.getType() == FIELD_MEDIC &&
            action_points >= game.getFieldMedicHealCost()) {
        Trooper to_heal = self;
        for (auto& t : teammates) {
            if (pos.has_neigh(Point(t)) &&
                    t.getHitpoints() < to_heal.getHitpoints()) {
                to_heal = t;
            }
        }
        if (to_heal.getHitpoints() < self.getMaximalHitpoints()) {
            action.setAction(HEAL);
            action.setX(to_heal.getX());
            action.setY(to_heal.getY());
            logId("heal " << to_heal.getType() << " (" << to_heal.getHitpoints() << ") at " << Point(to_heal));
            return;
        }
    }

    for (auto& enemy : enemies) {
        if (world.isVisible(self.getShootingRange(),
                    self.getX(), self.getY(), self.getStance(),
                    enemy.getX(), enemy.getY(), enemy.getStance())) {
            logId("seeing an enemy");
            target = Point(enemy);
            if (self.isHoldingFieldRation() &&
                    action_points >= game.getFieldRationEatCost()) {
                action.setAction(EAT_FIELD_RATION);
                logId("eat field ration");
                return;
            }
            if (self.isHoldingMedikit() &&
                    action_points >= game.getMedikitUseCost()) {
                // TODO: heal a teammate
                action.setAction(USE_MEDIKIT);
                action.setDirection(CURRENT_POINT);
                logId("use medkit");
                return;
            }
            // TODO: grenade
            if (action_points >= self.getShootCost()) {
                action.setAction(SHOOT);
                action.setX(enemy.getX());
                action.setY(enemy.getY());
                logId("shoot at " << Point(action.getX(), action.getY()));
                return;
            }
        }
    }

    if (action_points < game.getStandingMoveCost()) {
        action.setAction(END_TURN);
        logId("accumulate points");
        return;
    }

    Dijkstra dijkstra(pos, cells);

    logId("target = " << target);
    if (move_index == 1 || target == pos) {
        target = dijkstra.find_reachable(Point(random(sizeX), random(sizeY)));
        for (auto& bonus : bonuses) {
            Point b(bonus);
            if (
                    (bonus.getType() == GRENADE && self.isHoldingGrenade()) ||
                    (bonus.getType() == MEDIKIT && self.isHoldingMedikit()) ||
                    (bonus.getType() == FIELD_RATION && self.isHoldingFieldRation())
               ) {
                continue;
            }
            if (dijkstra.dist[b.x][b.y] < dijkstra.dist[target.x][target.y]) {
                target = b;
            }
        }
        logId("target = " << target);
    }

    Point next = target;
    while (dijkstra.prev[next.x][next.y] != pos) {
        next = dijkstra.prev[next.x][next.y];
    }
    action.setAction(MOVE);
    action.setX(next.x);
    action.setY(next.y);
    logId("move from " << pos << " to " << next);
}
