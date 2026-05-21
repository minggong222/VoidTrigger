#include "VoidTriggerDropItem.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "VoidTriggerCharacter.h" // 플레이어 클래스 헤더
#include "VoidTriggerEnemy.h"    // 적 클래스 헤더 (폭탄용)
#include "VoidTriggerGold.h"
#include "VoidTriggerItem.h"
// #include "VoidTriggerExpOrb.h" // 경험치 구슬 클래스 헤더 (자석용 - 본인 클래스명으로 변경하세요!)

AVoidTriggerDropItem::AVoidTriggerDropItem()
{
    PrimaryActorTick.bCanEverTick = false;

    // 충돌체(Sphere) 생성 및 크기 설정
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->SetSphereRadius(60.f); // 먹는 범위
    CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    // 시각적 메시 생성
    ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
    ItemMesh->SetupAttachment(RootComponent);
    ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 메시는 충돌 무시

    ItemType = ESpecialItemType::Heal; // 기본값
}

void AVoidTriggerDropItem::BeginPlay()
{
    Super::BeginPlay();
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AVoidTriggerDropItem::OnOverlapBegin);
}

void AVoidTriggerDropItem::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
                                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                                          bool bFromSweep, const FHitResult& SweepResult)
{
    // 닿은 액터가 플레이어인지 확인
    AVoidTriggerCharacter* Player = Cast<AVoidTriggerCharacter>(OtherActor);
    if (!Player) return;

    // --------------------------------------------------
    // 1. 자석 (Magnet)
    // --------------------------------------------------
    if (ItemType == ESpecialItemType::Magnet)
    {
        UE_LOG(LogTemp, Warning, TEXT("아이템 획득: 자석! 모든 경험치를 끌어당깁니다."));
        
        // 맵에 있는 모든 경험치 구슬을 찾아서 플레이어에게 날아가라고 명령
        TArray<AActor*> AllOrbs;
        
        // [주의] AVoidTriggerExpOrb 부분은 실제 사용 중인 경험치 클래스 이름으로 바꾸세요!
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVoidTriggerItem::StaticClass(), AllOrbs);
        
        for (AActor* OrbActor : AllOrbs)
        {
            AVoidTriggerItem* Orb = Cast<AVoidTriggerItem>(OrbActor);
            if (Orb) Orb->StartFlying(); // 경험치 구슬이 플레이어를 향해 날아가는 함수 호출
        }
        
        TArray<AActor*> AllGolds;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVoidTriggerGold::StaticClass(), AllGolds);
        
        for (AActor* GoldActor : AllGolds)
        {
            AVoidTriggerGold* Gold = Cast<AVoidTriggerGold>(GoldActor);
            if (Gold) Gold->StartFlying(); // 동전 클래스에 구현된 날아오기 함수 호출
        }
    }
    // --------------------------------------------------
    // 2. 폭탄 (Nuke)
    // --------------------------------------------------
    else if (ItemType == ESpecialItemType::Nuke)
    {
        UE_LOG(LogTemp, Warning, TEXT("아이템 획득: 폭탄! 화면 안의 적을 섬멸합니다."));
        
        // 맵에 있는 모든 적을 찾습니다.
        TArray<AActor*> AllEnemies;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVoidTriggerEnemy::StaticClass(), AllEnemies);

        for (AActor* EnemyActor : AllEnemies)
        {
            // 그냥 Destroy()하면 경험치를 안 떨구므로, 어마어마한 데미지를 줘서 죽입니다.
            UGameplayStatics::ApplyDamage(EnemyActor, 9999.f, Player->GetController(), this, UDamageType::StaticClass());
        }
    }
    // --------------------------------------------------
    // 3. 구급상자 (Heal)
    // --------------------------------------------------
    else if (ItemType == ESpecialItemType::Heal)
    {
        UE_LOG(LogTemp, Warning, TEXT("아이템 획득: 구급상자! 체력 30%% 회복."));
        
        // 플레이어의 체력을 최대 체력의 30%만큼 회복
        // (Player 클래스 안에 CurrentHealth와 MaxHealth 변수가 있다고 가정)
        float HealAmount = Player->MaxHP * 0.3f;
        Player->CurrentHP = FMath::Clamp(Player->CurrentHP + HealAmount, 0.f, Player->MaxHP);
    }

    // 아이템 효과 발동 후 자신은 삭제
    Destroy();
}