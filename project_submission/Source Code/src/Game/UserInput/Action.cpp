#include <iostream>
#include "Game/UserInput/Action.h"

using namespace std;

void Action::PerformAction(const ActionType& action) {
    switch (action) {
        case ActionType::MoveLeft:
            MoveLeft();
            break;
        case ActionType::MoveRight:
            MoveRight();
            break;
        case ActionType::MoveUp:
            MoveUp();
            break;
        case ActionType::MoveDown:
            MoveDown();
            break;
        case ActionType::Accelerate:
            Accelerate();
            break;
        case ActionType::Decelerate:
            Decelerate();
            break;
        case ActionType::Attack:
            Shoot();
            break;
        default:
            cout << "Unknown action type!" << endl;
            break;
    }
}
