﻿#include "CollisionManager.h"
#include "../Engine/Components/PhysicsComponent/Collider.h"


void CollisionManager::CollisionUpdate(const Dosuha::Vector<ICollision*>& collisions)
{
    for (const auto& collisionA : collisions)
    {
        for (const auto& collisionB : collisions)
        {
            if (collisionA == collisionB) continue;
            if (CollisionCheck(collisionA->GetCollider(), collisionB->GetCollider()))
            {
                collisionA->GetCollider()->Collide(collisionB);
                collisionB->GetCollider()->Collide(collisionA);
            }
        }
    }
}

bool CollisionManager::CollisionCheck(Collider* a, Collider* b)
{
    auto [aMin, aMax] = a->GetBounds();
    auto [bMin, bMax] = b->GetBounds();
    if (aMin.x > bMax.x) return false;
    if (aMin.y > bMax.y) return false;
    if (aMax.x < bMin.x) return false;
    if (aMax.y < bMin.y) return false;
    return true;
}
