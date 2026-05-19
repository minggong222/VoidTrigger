#include "VoidFireFloor.h"
#include "VoidTriggerCharacter.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

AVoidFireFloor::AVoidFireFloor()
{
    PrimaryActorTick.bCanEverTick = false; 
    InitialLifeSpan = 4.0f; 

    DamageCollision = CreateDefaultSubobject<USphereComponent>(TEXT("DamageCollision"));
    RootComponent = DamageCollision;
    DamageCollision->InitSphereRadius(FloorRadius);
    DamageCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    FireEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireEffect"));
    FireEffect->SetupAttachment(RootComponent);
}

void AVoidFireFloor::BeginPlay()
{
    Super::BeginPlay();

    // 플레이어를 찾아 무기고에 저장된 화염 장판 데미지 적용
    AVoidTriggerCharacter* PlayerChar = Cast<AVoidTriggerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (PlayerChar)
    {
        DamagePerTick = PlayerChar->AcidFloorDamage;
    }

    GetWorldTimerManager().SetTimer(
        DamageTimerHandle, 
        this, 
        &AVoidFireFloor::DealBurnDamage, 
        DamageTickRate, 
        true
    );
}

void AVoidFireFloor::DealBurnDamage()
{
    TArray<AActor*> OverlappingActors;
    DamageCollision->GetOverlappingActors(OverlappingActors);

    for (AActor* TargetActor : OverlappingActors)
    {
        if (TargetActor && TargetActor->ActorHasTag(FName("Monster")) && !TargetActor->ActorHasTag(FName("Flying")))
        {
            UGameplayStatics::ApplyDamage(
                TargetActor,
                DamagePerTick,
                nullptr,
                this,
                UDamageType::StaticClass()
            );
        }
    }
}