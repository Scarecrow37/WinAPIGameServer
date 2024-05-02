#include "Initialize.h"

#include "../../GameManager.h"
#include "../../Engine/World/World.h"
#include "../Objects/Player.h"
#include "../../Engine/Object/Object.h"
#include "../../InputSystems/IInputSystem.h"
#include "../../InputSystems/Controller/Controller.h"
#include "../../InputSystems/EnhancedInputSystem/InputAction/InputAction.h"
#include "../../InputSystems/EnhancedInputSystem/Modifiers/Modifier.h"
#include "../../InputSystems/EnhancedInputSystem/Modifiers/ModifyStrategies/DirectionModifyStrategies/InverseDirectionModifyStrategy.h"
#include "../Objects/Floor.h"

InputAction* CreateMovementAction()
{
    InputAction* action = InputAction::Builder::GetInstance()
                          .Initialize()
                          .OpenInvokerSet()
                          .AddInvoker(GameManager::GetKeyboard()->GetInputValueGetter(VK_LEFT), Modifier::InverseStrategy)
                          .AddInvoker(GameManager::GetKeyboard()->GetInputValueGetter(VK_RIGHT))
                          .CloseInvokerSet()
                          .OpenInvokerSet()
                          .AddInvoker( GameManager::GetController()->GetInputValueGetter(Controller::Component::LeftThumbX))
                          .CloseInvokerSet()
                          .Build();
    GameManager::AddInputAction(action);
    return action;
}

InputAction* CreateJumpAction()
{
    InputAction* jump = InputAction::Builder::GetInstance()
                        .Initialize()
                        .OpenInvokerSet()
                        .AddInvoker(GameManager::GetKeyboard()->GetInputValueGetter(VK_SPACE))
                        .CloseInvokerSet()
                        .OpenInvokerSet()
                        .AddInvoker(GameManager::GetController()->GetInputValueGetter(Controller::Component::A))
                        .CloseInvokerSet()
                        .Build();
    GameManager::AddInputAction(jump);
    return jump;
}

Player* Game::CreatePlayer()
{
    Player* player = new Player(L"GraveRobber");
    player->SetMovementAction(CreateMovementAction());
    player->SetJumpAction(CreateJumpAction());
    return player;
}

World* Game::LoadWorld()
{
    World* world = new World;
    world->AddObject(new Floor);

    world->AddObject(CreatePlayer());

    return world;
}
