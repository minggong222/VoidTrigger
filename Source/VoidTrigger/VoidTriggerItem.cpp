// Fill out your copyright notice in the Description page of Project Settings.

#include "VoidTriggerItem.h"
#include "VoidTriggerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AVoidTriggerItem::AVoidTriggerItem()
{
    // Tick 활성화
    PrimaryActorTick.bCanEverTick = true;

    // 1. 충돌체 설정
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    RootComponent = CollisionComponent;
    CollisionComponent->SetSphereRadius(32.f);

    // 2. 메쉬 설정
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComponent->SetupAttachment(RootComponent);
    // 물리 충돌은 끄고 겹침(Overlap)만 처리하거나, 필요에 따라 Profile 설정
    MeshComponent->SetCollisionProfileName(TEXT("NoCollision")); 
}

void AVoidTriggerItem::BeginPlay()
{
    Super::BeginPlay();

    // 스폰 시 아주 약간 공중으로 랜덤하게 튀어 오르는 느낌 추가
    FVector RandomDir = FVector(
       FMath::FRandRange(-0.5f, 0.5f),
       FMath::FRandRange(-0.5f, 0.5f),
       FMath::FRandRange(0.5f, 1.f)
    ).GetSafeNormal();

    AddActorWorldOffset(RandomDir * 30.f);
}

void AVoidTriggerItem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    RunningTime += DeltaTime;

    if (bIsFlying)
    {
        // --- [모드 A] 플레이어에게 자석처럼 끌려감 ---
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (PlayerPawn)
        {
            FVector CurrentLoc = GetActorLocation();
            FVector TargetLoc = PlayerPawn->GetActorLocation();

            // 위치 이동
            FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, MoveSpeed);
            SetActorLocation(NewLoc);

            // 🔹 날아갈 때 항상 플레이어를 정면으로 바라보게 함
            FVector Direction = (TargetLoc - CurrentLoc).GetSafeNormal();
            if (!Direction.IsNearlyZero())
            {
                FRotator TargetRotation = Direction.Rotation();
                FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 15.0f);
                SetActorRotation(NewRotation);
            }

            // 거리 체크 후 획득 처리
            float Distance = FVector::Dist(NewLoc, TargetLoc);
            if (Distance < 50.f)
            {
                if (AVoidTriggerCharacter* Player = Cast<AVoidTriggerCharacter>(PlayerPawn))
                {
                    Player->GainExp(10.f);
                }
                Destroy();
            }
        }
    }
    else
    {
        // --- [모드 B] 바닥에서 대기 (시각 효과 강화) ---
        
        // 1. 뱅글뱅글 회전 (평면 메쉬가 옆에서 안 보이는 문제 해결)
        AddActorLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));

        // 2. 둥둥 뜨기 (사인 곡선 이용)
        float VerticalOffset = FMath::Sin(RunningTime * 3.0f) * BobbingAmplitude;
        AddActorLocalOffset(FVector(0.f, 0.f, VerticalOffset));
    }
}

void AVoidTriggerItem::StartFlying()
{
    bIsFlying = true;
}