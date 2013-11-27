#include "MyStrategy.h"

#include <cmath>
#include <cstdlib>

using namespace model;
using namespace std;

MyStrategy::MyStrategy() {
}

void MyStrategy::move(const Trooper& self,
        const World& world, const Game& game, Move& action) {
    if (self.getActionPoints() < game.getStandingMoveCost()) {
        return;
    }

    auto& cells = world.getCells();
    auto& troopers = world.getTroopers();

    for (auto& enemy : troopers) {
        if (enemy.isTeammate()) {
            continue;
        }
        bool can_shoot =
            self.getActionPoints() >= self.getShootCost()
            && world.isVisible(self.getShootingRange(),
                    self.getX(), self.getY(), self.getStance(),
                    enemy.getX(), enemy.getY(), enemy.getStance());
        if (can_shoot) {
            action.setAction(SHOOT);
            action.setX(enemy.getX());
            action.setY(enemy.getY());
            return;
        }
    }

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
        action.setAction(MOVE);

        if (canMoveX && canMoveY) {
            if (rand() % 2 == 0) {
                action.setX(self.getX() + offsetX);
                action.setY(self.getY());
            } else {
                action.setX(self.getX());
                action.setY(self.getY() + offsetY);
            }
        } else if (canMoveX) {
            action.setX(self.getX() + offsetX);
            action.setY(self.getY());
        } else {
            action.setX(self.getX());
            action.setY(self.getY() + offsetY);
        }

        return;
    }
}
