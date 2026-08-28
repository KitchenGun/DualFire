// Copyright DualFire. All Rights Reserved.
// DefaultEngine.ini [/Script/Engine.CollisionProfile]과 채널 순서를 맞춰야 한다.

#pragma once

#include "Engine/EngineTypes.h"

namespace DualFireChannel
{
	constexpr ECollisionChannel PlayerHitbox = ECC_GameTraceChannel1;
	constexpr ECollisionChannel EnemyBody    = ECC_GameTraceChannel4;
}

namespace DualFireProfile
{
    static constexpr const TCHAR* PlayerPawn        = TEXT("PlayerPawn");
    static constexpr const TCHAR* EnemyBullet       = TEXT("EnemyBulletProfile");
    static constexpr const TCHAR* PlayerBullet      = TEXT("PlayerBulletProfile");
    static constexpr const TCHAR* EnemyBody         = TEXT("EnemyBodyProfile");
}
