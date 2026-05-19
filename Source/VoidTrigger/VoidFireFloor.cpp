#include "VoidFireFloor.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

// ==========================================
// 생성자: 컴포넌트 조립 및 기본 세팅
// ==========================================
AVoidFireFloor::AVoidFireFloor()
{
    // 장판은 매 프레임(Tick) 돌아갈 필요가 없으므로 꺼줍니다 (최적화)
    PrimaryActorTick.bCanEverTick = false; 
    
    // 생성된 지 4초가 지나면 언리얼이 알아서 이 액터를 파괴(소멸)시킵니다.
    InitialLifeSpan = 4.0f; 

    // 1. 충돌체(구체) 생성 및 루트로 설정
    DamageCollision = CreateDefaultSubobject<USphereComponent>(TEXT("DamageCollision"));
    RootComponent = DamageCollision;
    DamageCollision->InitSphereRadius(FloorRadius);
    
    // 캐릭터나 적이 통과할 수 있도록 Overlap 설정
    DamageCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    // 2. 나이아가라 이펙트 컴포넌트 생성 및 부착
    FireEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireEffect"));
    FireEffect->SetupAttachment(RootComponent);
}

// ==========================================
// 게임 시작 시 타이머 작동
// ==========================================
void AVoidFireFloor::BeginPlay()
{
    Super::BeginPlay();

    // DamageTickRate(0.5초) 간격으로 DealBurnDamage 함수를 반복 실행
    GetWorldTimerManager().SetTimer(
        DamageTimerHandle, 
        this, 
        &AVoidFireFloor::DealBurnDamage, 
        DamageTickRate, 
        true
    );
}

// ==========================================
// 실제 데미지를 주는 로직
// ==========================================
void AVoidFireFloor::DealBurnDamage()
{
    // 현재 DamageCollision(구체) 안에 겹쳐있는 모든 액터를 배열로 가져옵니다.
    TArray<AActor*> OverlappingActors;
    DamageCollision->GetOverlappingActors(OverlappingActors);

    for (AActor* TargetActor : OverlappingActors)
    {
        // 몬스터 태그가 있고(AND), 공중몹(Flying) 태그가 없는 녀석만 타격!
        if (TargetActor && 
            TargetActor->ActorHasTag(FName("Monster")) && 
            !TargetActor->ActorHasTag(FName("Flying")))
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