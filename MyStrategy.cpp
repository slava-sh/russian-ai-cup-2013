#include "MyStrategy.h"

#include <cmath>
#include <cstdlib>

using namespace model;
using namespace std;

MyStrategy::MyStrategy() {
}

void MyStrategy::move(const Trooper& self, const World& world, const Game& game, Move& move) {
    if (self.getActionPoints() < game.getStandingMoveCost()) {
        return;
    }

    vector< vector< CellType > > cells = world.getCells();

    int targetX = world.getWidth() / 2;
    int targetY = world.getHeight() / 2;

    int offsetX = self.getX() > targetX ?
        -1 : (self.getX() < targetX ? 1 : 0);
    int offsetY = self.getY() > targetY ?
        -1 : (self.getY() < targetY ? 1 : 0);

    bool canMoveX =
        offsetX != 0 && cells[self.getX() + offsetX][self.getY()] == FREE;
    bool canMoveY =
        offsetY != 0 && cells[self.getX()][self.getY() + offsetY] == FREE;

    if (canMoveX || canMoveY) {
        move.setAction(MOVE);

        if (canMoveX && canMoveY) {
            if (rand() % 2 == 0) {
                move.setX(self.getX() + offsetX);
                move.setY(self.getY());
            } else {
                move.setX(self.getX());
                move.setY(self.getY() + offsetY);
            }
        } else if (canMoveX) {
            move.setX(self.getX() + offsetX);
            move.setY(self.getY());
        } else {
            move.setX(self.getX());
            move.setY(self.getY() + offsetY);
        }

        return;
    }
}
