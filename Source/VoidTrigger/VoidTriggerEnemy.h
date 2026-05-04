// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/AudioComponent.h"
#include "VoidTriggerEnemy.generated.h"

UCLASS()
class VOIDTRIGGER_API AVoidTriggerEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    // Sets default values for this character's properties
    AVoidTriggerEnemy();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    
    virtual void HandleMovement(float DeltaTime);

    // 1. 적에게 부착될 오디오 컴포넌트 (위치 추적용)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
    UAudioComponent* AudioComponent;

    // 2. 에디터에서 할당할 사운드 큐 에셋
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* EnemySoundCue;

    // 1. 타이머 핸들 선언
    FTimerHandle AudioTimerHandle;

    // 2. 5초마다 실행될 함수
    void PlayPeriodicSound();

    // 재생 간격 (원하시는 대로 조절 가능)
    UPROPERTY(EditAnywhere, Category = "Audio")
    float SoundInterval = 5.0f;
public: 
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // --- 적 스탯 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float MaxHealth = 50.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    float CurrentHealth;

    // ★ [추가] 풀링 시 체력 무한 증식 방지를 위한 '기본 쌩얼 체력'
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    float BaseMaxHP = -1.0f;

    // ★ [추가] 웨이브 단계에 맞춰 체력을 뻥튀기하는 함수
    void ApplyWaveScaling(int32 WaveLevel, float HealthIncreaseRate);

    // --- 드랍 아이템 분리 (중요!) ---
    // 1. 기존 경험치 구슬용 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
    TSubclassOf<AActor> ExpOrbClass;
    
    // 2. 새로 만든 특수 아이템(자석, 폭탄, 힐)용 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
    TSubclassOf<class AVoidTriggerDropItem> SpecialItemClass;
    
    // ★ [추가] 골드 아이템 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
    TSubclassOf<AActor> GoldClass;

    // ★ [추가] 기본 골드 드랍 확률 (0.3 = 30%)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
    float BaseGoldDropRate = 0.3f;
    
public:
    // 적이 플레이어에게 입힐 데미지
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackDamage = 10.f;

    // 공격 쿨타임 (한 번 치고 나서 다시 칠 때까지의 시간)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float AttackDelay = 1.0f;

    // 공격 가능한 상태인지 확인
    bool bCanAttack = true;

    // 공격 쿨타임 초기화 함수
    void ResetAttack();
    
    // --- 데미지 받기 (언리얼 기본 함수 덮어쓰기) ---
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    // 현재 이 적이 창고에 있는지, 맵에서 활동 중인지 체크하는 변수
    bool bIsActive = true;

    // 죽었을 때: 창고로 들어가는 함수 (Destroy 대체)
    virtual void Deactivate();

    // 창고에서 꺼낼 때: 다시 스폰되는 함수
    virtual void Activate(FVector NewLocation);
    virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
};