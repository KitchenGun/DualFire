// Copyright DualFire. All Rights Reserved.
// DefaultEngine.ini [/Script/Engine.CollisionProfile] 섹션과 1:1 대응
// 채널 순서 변경 시 ini와 동기화 필요

#pragma once

#include "Engine/EngineTypes.h"

// ── Custom Object Channels ───────────────────────────────────────────────────
namespace DualFireChannel
{
    constexpr ECollisionChannel PlayerHitbox = ECC_GameTraceChannel1;
    constexpr ECollisionChannel EnemyBullet  = ECC_GameTraceChannel2;
    constexpr ECollisionChannel PlayerBullet = ECC_GameTraceChannel3;
    constexpr ECollisionChannel EnemyBody    = ECC_GameTraceChannel4;
}

// ── Collision Profile Names ──────────────────────────────────────────────────
namespace DualFireProfile
{
    // 플레이어 히트박스 컴포넌트 (ObjectType=PlayerHitbox)
    // Overlap: PlayerHitbox, EnemyBullet
    static constexpr const TCHAR* PlayerPawn        = TEXT("PlayerPawn");

    // 적 총알 (ObjectType=EnemyBullet)
    // Overlap: PlayerHitbox
    static constexpr const TCHAR* EnemyBullet       = TEXT("EnemyBulletProfile");

    // 플레이어 총알 (ObjectType=PlayerBullet)
    // Overlap: EnemyBody  ← 속성 비교는 OnComponentBeginOverlap 이벤트에서 처리
    static constexpr const TCHAR* PlayerBullet      = TEXT("PlayerBulletProfile");

    // 적 몸통 히트박스 (ObjectType=EnemyBody)
    // Overlap: PlayerBullet
    static constexpr const TCHAR* EnemyBody         = TEXT("EnemyBodyProfile");
}

/*
 * ── 사용 예시 ──────────────────────────────────────────────────────────────
 *
 * [플레이어 캐릭터 히트박스 컴포넌트]
 *   USphereComponent* HitboxComp = CreateDefaultSubobject<USphereComponent>(TEXT("Hitbox"));
 *   HitboxComp->SetCollisionProfileName(DualFireProfile::PlayerPawn);
 *
 * [적 총알 액터]
 *   USphereComponent* BulletComp = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
 *   BulletComp->SetCollisionProfileName(DualFireProfile::EnemyBullet);
 *   BulletComp->OnComponentBeginOverlap.AddDynamic(this, &AEnemyBullet::OnHit);
 *
 * [플레이어 총알 액터]
 *   USphereComponent* BulletComp = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
 *   BulletComp->SetCollisionProfileName(DualFireProfile::PlayerBullet);
 *   BulletComp->OnComponentBeginOverlap.AddDynamic(this, &APlayerBullet::OnHit);
 *   // OnHit 내부에서 OtherActor의 속성(등급, 팀 등) 비교 후 피해 처리
 *
 * [적 몸통 히트박스 컴포넌트]
 *   UCapsuleComponent* BodyComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("EnemyBody"));
 *   BodyComp->SetCollisionProfileName(DualFireProfile::EnemyBody);
 */
