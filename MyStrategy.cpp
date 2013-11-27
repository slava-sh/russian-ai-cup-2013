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
    }

    void add(const Cells& cells, set< pair< int, Point > >& q,
            const Point& cur, int d, const Point& p) {
        // TODO: cells locked by teammates
        if (p.isCorrect()
                && cells[p.x][p.y] == FREE
                && !reached[p.x][p.y]
                && d < dist[p.x][p.y]) {
            dist[p.x][p.y] = d;
            prev[p.x][p.y] = cur;
            q.insert(make_pair(d, p));
        }
    }

    Point find_reachable(const Cells& cells, Point p) {
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

Point long_target;

void MyStrategy::move(const Trooper& self,
        const World& world, const Game& game, Move& action) {

    move_index += 1;

    if (move_index == 1) {
        sizeX = world.getWidth();
        sizeY = world.getHeight();
    }

    Point pos(self);
    logId("move number " << move_index);
    logId("at " << pos);

    auto& bonuses = world.getBonuses();
    auto& cells = world.getCells();
    auto& troopers = world.getTroopers();

    vector< Trooper > enemies;
    for (auto& trooper : troopers) {
        if (!trooper.isTeammate()) {
            enemies.push_back(trooper);
        }
    }
    logId("we see " << enemies.size() << " enemies");

    for (auto& enemy : enemies) {
        if (world.isVisible(self.getShootingRange(),
                    self.getX(), self.getY(), self.getStance(),
                    enemy.getX(), enemy.getY(), enemy.getStance())) {
            logId("see an enemy");
            if (self.isHoldingFieldRation() &&
                    self.getActionPoints() >= game.getFieldRationEatCost()) {
                action.setAction(EAT_FIELD_RATION);
                logId("eat field ration");
                return;
            }
            if (self.isHoldingMedikit() &&
                    self.getActionPoints() >= game.getMedikitUseCost()) {
                action.setAction(USE_MEDIKIT);
                action.setDirection(CURRENT_POINT);
                logId("use medkit");
                return;
            }
            // TODO: grenade
            if (self.getActionPoints() >= self.getShootCost()) {
                action.setAction(SHOOT);
                action.setX(enemy.getX());
                action.setY(enemy.getY());
                logId("shoot at " << Point(action.getX(), action.getY()));
                return;
            }
        }
    }

    if (self.getActionPoints() < game.getStandingMoveCost()) {
        logId("accumulate points");
        return;
    }

    Dijkstra dijkstra(pos, cells);

    if (move_index == 1) {
        long_target = dijkstra.find_reachable(cells, Point(sizeX / 2, sizeY / 2));
    }

    Point target = long_target;
    while (target == pos) {
        long_target = dijkstra.find_reachable(cells, Point(random(sizeX), random(sizeY)));
        target = long_target;
    }
    logId("target = " << target);

    for (auto& enemy : enemies) {
        Point e(enemy);
        if (dijkstra.dist[e.x][e.y] < dijkstra.dist[target.x][target.y]) {
            target = e;
        }
    }
    logId("target = " << target);

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

    if (target == pos) {
        logId("no move");
        return;
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
