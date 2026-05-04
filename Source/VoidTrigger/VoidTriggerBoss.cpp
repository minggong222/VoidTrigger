// Fill out your copyright notice in the Description page of Project Settings.

#include "VoidTriggerBoss.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "VoidTriggerGameMode.h"

AVoidTriggerBoss::AVoidTriggerBoss()
{
    MaxHealth = 500.f;
    AttackDamage = 30.f;
}

void AVoidTriggerBoss::BeginPlay()
{
    Super::BeginPlay();

    if (bIsActive)
    {
        GetWorldTimerManager().SetTimer(SkillTimerHandle, this, &AVoidTriggerBoss::UseRandomSkill, SkillCooldown, true);
    }
}

void AVoidTriggerBoss::HandleMovement(float DeltaTime)
{
    // 스킬을 준비하는 중이라면 쫓아가지 않고 제자리에 대기
    if (bIsCasting)
    {
        return; 
    }

    Super::HandleMovement(DeltaTime);
}

void AVoidTriggerBoss::Activate(FVector NewLocation)
{
    Super::Activate(NewLocation);

    bIsCasting = false; // 창고에서 나올 때 상태 초기화
    GetWorldTimerManager().SetTimer(SkillTimerHandle, this, &AVoidTriggerBoss::UseRandomSkill, SkillCooldown, true);
}

void AVoidTriggerBoss::UseRandomSkill()
{
    if (!bIsActive || bIsCasting) return;

    int32 RandomSkill = FMath::RandRange(0, 1);

    if (RandomSkill == 0)
    {
        PrepareThrowRock();
    }
    else
    {
        PrepareDash(); 
    }
}

// 1-A. 돌 던지기 준비
void AVoidTriggerBoss::PrepareThrowRock()
{
    bIsCasting = true; 
    UE_LOG(LogTemp, Warning, TEXT("보스 스킬: 돌 던지기 준비 중... (%f초)"), ThrowRockCastTime);

    if (ThrowRockMontage)
    {
        PlayAnimMontage(ThrowRockMontage);
    }

    GetWorldTimerManager().SetTimer(ActionTimerHandle, this, &AVoidTriggerBoss::ExecuteThrowRock, ThrowRockCastTime, false);
}
// 1-B. 실제 돌 투척
void AVoidTriggerBoss::ExecuteThrowRock()
{
    if (!bIsActive) return;

    AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (PlayerActor && RockProjectileClass)
    {
        FVector StartLocation = GetActorLocation() + FVector(0.f, 0.f, 50.f); 
        FVector Direction = (PlayerActor->GetActorLocation() - StartLocation).GetSafeNormal();
        FRotator SpawnRotation = Direction.Rotation();

        GetWorld()->SpawnActor<AActor>(RockProjectileClass, StartLocation, SpawnRotation);
        UE_LOG(LogTemp, Warning, TEXT("보스 스킬: 돌 투척!"));
    }

    bIsCasting = false; // 스킬 끝, 다시 쫓아감
}

// 2-A. 돌진 준비
void AVoidTriggerBoss::PrepareDash()
{
    AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerActor) return;

    bIsCasting = true; 

    CachedDashDirection = (PlayerActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    CachedDashDirection.Z = 0.f; 

    UE_LOG(LogTemp, Warning, TEXT("보스 스킬: 돌진 기 모으는 중... (%f초)"), DashChargeTime);

    if (DashMontage)
    {
        PlayAnimMontage(DashMontage);
    }

    GetWorldTimerManager().SetTimer(ActionTimerHandle, this, &AVoidTriggerBoss::ExecuteDash, DashChargeTime, false);
}

// 2-B. 실제 돌진
void AVoidTriggerBoss::ExecuteDash()
{
    if (!bIsActive) return; 

    LaunchCharacter(CachedDashDirection * DashForce, true, true);
    
    bIsCasting = false; // 스킬 끝, 다시 쫓아감

    UE_LOG(LogTemp, Warning, TEXT("보스 스킬: 콰앙! 돌진 발사!"));
}

void AVoidTriggerBoss::Deactivate()
{
    // 1. 경험치 구슬 폭사 로직 (기존 유지)
    if (bIsActive && ExpOrbClass && GetWorld())
    {
        FVector BaseLocation = GetActorLocation();
        for (int32 i = 0; i < BossExpDropCount; i++)
        {
            // ... (생략)
        }
    }

    // 2. 타이머 정리 (기존 유지)
    GetWorldTimerManager().ClearTimer(SkillTimerHandle);
    GetWorldTimerManager().ClearTimer(ActionTimerHandle);
    bIsCasting = false; 

    // =======================================================
    // ★ [추가] 보스가 죽는 순간, 게임모드에게 발악 페이즈 시작을 명령!
    // =======================================================
    AVoidTriggerGameMode* GM = Cast<AVoidTriggerGameMode>(GetWorld()->GetAuthGameMode());
    if (GM)
    {
        GM->OnBossDefeated(); 
    }
    // =======================================================

    // 3. 부모 클래스(VoidTriggerEnemy)의 사망 처리 실행 (창고로 들어가기)
    Super::Deactivate();
}