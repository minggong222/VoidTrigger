// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VoidTriggerEnemy.h" // 부모 클래스 포함
#include "VoidTriggerBoss.generated.h"

UCLASS()
class VOIDTRIGGER_API AVoidTriggerBoss : public AVoidTriggerEnemy
{
    GENERATED_BODY()

public:
    AVoidTriggerBoss();

protected:
    virtual void BeginPlay() override;

    // 스킬 준비 중 이동을 멈추기 위해 부모의 이동 함수 덮어쓰기
    virtual void HandleMovement(float DeltaTime) override;

public:
    // --- 보스 스킬 설정 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Skills")
    TSubclassOf<AActor> RockProjectileClass; 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Skills")
    float SkillCooldown = 3.0f; 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Skills")
    float DashForce = 20000.f; 

    // 스킬 준비(기 모으기) 시간
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Skills")
    float ThrowRockCastTime = 0.5f; // 돌 던지기 준비 시간

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Skills")
    float DashChargeTime = 2.5f; // 돌진 기 모으는 시간

    // --- 드랍 설정 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Drops")
    int32 BossExpDropCount = 20; 

    // --- 보스 스킬 애니메이션 (몽타주) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Animation")
    class UAnimMontage* ThrowRockMontage; // 돌 던지기 애니메이션

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss Animation")
    class UAnimMontage* DashMontage; // 돌진 기 모으기 애니메이션
    
    // 타이머 핸들 및 상태 변수
    FTimerHandle SkillTimerHandle;
    FTimerHandle ActionTimerHandle; // 돌진/돌던지기 실행용 공용 타이머
    
    bool bIsCasting = false; // 현재 스킬을 준비 중인지 체크
    FVector CachedDashDirection; // 돌진 시 타겟팅한 플레이어 방향 저장

    // 스킬 함수들
    void UseRandomSkill();
    
    // 1. 돌 던지기 로직
    void PrepareThrowRock();
    void ExecuteThrowRock();

    // 2. 돌진 로직
    void PrepareDash();
    void ExecuteDash();

    virtual void Deactivate() override;
    virtual void Activate(FVector NewLocation) override;
};