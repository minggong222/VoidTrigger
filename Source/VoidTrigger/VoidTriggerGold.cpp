#include "VoidTriggerGold.h"
#include "VoidTriggerCharacter.h"
#include "MySaveGame.h" // 본인이 만든 세이브 게임 클래스 헤더
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AVoidTriggerGold::AVoidTriggerGold()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    RootComponent = CollisionComponent;
    CollisionComponent->SetSphereRadius(32.f);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetCollisionProfileName(TEXT("NoCollision")); 
}

void AVoidTriggerGold::BeginPlay() { Super::BeginPlay(); }

void AVoidTriggerGold::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RunningTime += DeltaTime;

    if (bIsFlying)
    {
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (PlayerPawn)
        {
            FVector CurrentLoc = GetActorLocation();
            FVector TargetLoc = PlayerPawn->GetActorLocation();

            // 1. 플레이어에게 자석처럼 날아감
            FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, MoveSpeed);
            SetActorLocation(NewLoc);

            // 2. 획득 판정 (거리 체크)
            float Distance = FVector::Dist(NewLoc, TargetLoc);
            if (Distance < 50.f)
            {
                // --- [세이브 데이터 연동 로직] ---
                // 슬롯 이름 "Save01"은 본인이 설정한 이름과 맞춰야 합니다.
                UMySaveGame* SaveData = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Save01"), 0));
                
                if (SaveData)
                {
                    // 세이브 파일에 골드 추가
                    SaveData->TotalGold += GoldValue;
                    
                    // 수정한 데이터를 다시 슬롯에 저장 (이 과정이 있어야 영구 저장됨)
                    UGameplayStatics::SaveGameToSlot(SaveData, TEXT("Save01"), 0);
                    
                    UE_LOG(LogTemp, Warning, TEXT("골드 획득! 현재 총 골드: %d"), SaveData->TotalGold);
                }
                
                Destroy();
            }
        }
    }
    else
    {
        // 바닥에서 대기 중일 때 (회전 + 둥둥 뜨기)
        AddActorLocalRotation(FRotator(0.f, 150.f * DeltaTime, 0.f));
        float VerticalOffset = FMath::Sin(RunningTime * 3.0f) * 0.5f;
        AddActorLocalOffset(FVector(0.f, 0.f, VerticalOffset));
    }
}

void AVoidTriggerGold::StartFlying() { bIsFlying = true; }