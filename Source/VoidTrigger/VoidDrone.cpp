#include "VoidDrone.h"
#include "VoidTriggerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AVoidDrone::AVoidDrone()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;

    USceneComponent* DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = DefaultRoot;

    DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
    DroneMesh->SetupAttachment(RootComponent);
    DroneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetupAttachment(RootComponent);
    DetectionSphere->InitSphereRadius(1500.f);
    DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    HoverOffset = FVector(1000.f, -500.f, -100.f);
}

void AVoidDrone::BeginPlay()
{
    Super::BeginPlay();
    // 타이머 가동은 플레이어 스탯을 받아온 후 InitializeDrone에서 실행합니다.
}

void AVoidDrone::InitializeDrone(AActor* InOwner)
{
    OwnerPlayer = InOwner;

    if (AVoidTriggerCharacter* PlayerChar = Cast<AVoidTriggerCharacter>(InOwner))
    {
        // 무기고 스탯에서 드론 공속을 가져와서 타이머에 적용
        float ActualFireRate = PlayerChar->DroneAttackRate;
        
        GetWorldTimerManager().SetTimer(
           FireTimerHandle,
           this,
           &AVoidDrone::FindTargetAndShoot,
           ActualFireRate,
           true
        );
        
        UE_LOG(LogTemp, Warning, TEXT("드론 가동 시작! 공격 주기: %f초"), ActualFireRate);
    }
}

void AVoidDrone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AVoidDrone::FindTargetAndShoot()
{
    TArray<AActor*> OverlappingActors;
    DetectionSphere->GetOverlappingActors(OverlappingActors);

    AActor* ClosestTarget = nullptr;
    float MinDistance = TNumericLimits<float>::Max();

    for (AActor* TargetActor : OverlappingActors)
    {
       if (!TargetActor || !TargetActor->ActorHasTag(FName("Monster")))
       {
          continue;
       }

       float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());

       if (Distance < MinDistance)
       {
          MinDistance = Distance;
          ClosestTarget = TargetActor;
       }
    }

    if (ClosestTarget)
    {
       UGameplayStatics::ApplyDamage(
          ClosestTarget,
          Damage, 
          nullptr,
          this,
          UDamageType::StaticClass()
       );

       if (HitEffect)
       {
          UNiagaraFunctionLibrary::SpawnSystemAttached(
             HitEffect,
             ClosestTarget->GetRootComponent(), 
             NAME_None, 
             FVector::ZeroVector, 
             FRotator::ZeroRotator, 
             EAttachLocation::SnapToTarget, 
             true 
          );
       }
       
       if (BulletTracerEffect)
       {
          FVector StartLocation = DroneMesh->GetSocketLocation(FName("Muzzle"));
          FVector EndLocation = ClosestTarget->GetActorLocation();

          FRotator EffectRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, EndLocation);

          UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
               GetWorld(),
               BulletTracerEffect,
               StartLocation,
               EffectRotation
          );

          if (TracerComp)
          {
             TracerComp->SetVariableVec3(FName("User.BeamEnd"), EndLocation);
             TracerComp->SetVariableVec3(FName("User.BeamStart"), StartLocation);
          }
       }
    }
}