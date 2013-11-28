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

Move make_move(ActionType action) {
    Move result;
    result.setAction(action);
    return result;
}

Move make_move(ActionType action, int x, int y) {
    Move result;
    result.setAction(action);
    result.setX(x);
    result.setY(y);
    return result;
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

    const Point pos;
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
        }

        cells = world.getCells();
        for (auto& trooper : world.getTroopers()) {
            if (trooper.isTeammate()) {
                cells[trooper.getX()][trooper.getY()] = HIGH_COVER;
                teammates.push_back(trooper);
            }
            else {
                enemies.push_back(trooper);
            }
        }

        dijkstra = Dijkstra(pos, cells);
    }

    Move run() {
        int action_points = self.getActionPoints();

        logId("move number " << move_index);
        logId(self.getType() << " (" << action_points << ") at " << pos);
        logId("we see " << enemies.size() << " enemies");

        return go(action_points);
    }

    Move go(int action_points) {
        if (action_points >= game.getStandingMoveCost()) {
            for (auto& n : pos.neighs()) {
                return make_move(MOVE, n.x, n.y);
            }
        }
        return make_move(END_TURN);
    }
};

MyStrategy::MyStrategy() {}

void MyStrategy::move(const Trooper& self,
        const World& world, const Game& game, Move& action) {
    SlavaStrategy strategy(self, world, game);
    action = strategy.run();
}
