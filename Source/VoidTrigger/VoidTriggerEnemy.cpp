// Fill out your copyright notice in the Description page of Project Settings.

#include "VoidTriggerEnemy.h"
#include "VoidTriggerCharacter.h"
#include "VoidTriggerDropItem.h"
#include "Kismet/GameplayStatics.h" // 플레이어 찾기와 데미지 처리에 필요

// Sets default values
AVoidTriggerEnemy::AVoidTriggerEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // 오디오 컴포넌트 생성 및 루트(또는 메쉬)에 부착
    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("EnemyAudioSource"));
    AudioComponent->SetupAttachment(RootComponent); // 이제 적이 움직이면 소리도 따라다님
        
    // 게임 시작 시 바로 소리가 나지 않게 설정
    AudioComponent->bAutoActivate = false;
}

// Called when the game starts or when spawned
void AVoidTriggerEnemy::BeginPlay()
{
    Super::BeginPlay();
    
    // ★ [추가] 처음 태어날 때 블루프린트에 설정된 내 원래 체력을 기록해 둡니다.
    if (BaseMaxHP < 0.0f)
    {
        BaseMaxHP = MaxHealth;
    }

    CurrentHealth = MaxHealth;
    
    // 3. 게임 시작 시 타이머 설정 (5초마다 반복 실행)
    GetWorldTimerManager().SetTimer(
       AudioTimerHandle, 
       this, 
       &AVoidTriggerEnemy::PlayPeriodicSound, 
       SoundInterval, 
       true // 반복 여부 (true면 5초마다 계속 실행)
    );
}

// Called every frame
void AVoidTriggerEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsActive)
    {
       return;
    }

    HandleMovement(DeltaTime);
}

void AVoidTriggerEnemy::HandleMovement(float DeltaTime)
{
    AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerActor)
    {
       return;
    }

    FVector Direction = (PlayerActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    AddMovementInput(Direction);
}

// Called to bind functionality to input
void AVoidTriggerEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float AVoidTriggerEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (!bIsActive)
    {
       return 0.f;
    }

    Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    CurrentHealth -= DamageAmount;

    // UE_LOG(LogTemp, Warning, TEXT("적 피격! 남은 체력: %f"), CurrentHealth);

    if (CurrentHealth <= 0.f)
    {
       Deactivate();
    }

    return DamageAmount;
}

// --- 수정된 Deactivate 내용 ---
void AVoidTriggerEnemy::Deactivate()
{
    if (!bIsActive) return;
    bIsActive = false;

    // 1. 드롭 위치 계산 (기존 LineTrace 로직 유지)
    FVector DropLocation = GetActorLocation();
    if (GetWorld())
    {
       FHitResult Hit;
       FVector Start = GetActorLocation();
       FVector End = Start - FVector(0.f, 0.f, 2000.f);
       FCollisionQueryParams QueryParams;
       QueryParams.AddIgnoredActor(this);

       if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
       {
          DropLocation = Hit.Location + FVector(0.f, 0.f, 20.f);
       }
    }

    // 2. ★ 플레이어의 특성 보너스 가져오기 (골드 & 특수 아이템 분리)
    AVoidTriggerCharacter* Player = Cast<AVoidTriggerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    float GoldDropBonus = Player ? Player->ResourceDropBonus : 0.0f;
    float SpecialBonus = Player ? Player->SpecialDropBonus : 0.0f; // 새로 추가된 특수 보너스

    // 3. ★ 특수 아이템 드랍 판정 (기본 2% + 보급품 탐색 보너스)
    float SpecialRoll = FMath::FRand(); 
    if (SpecialRoll <= (0.02f + SpecialBonus)) 
    {
       if (SpecialItemClass && GetWorld())
       {
          AVoidTriggerDropItem* SpawnedItem = GetWorld()->SpawnActor<AVoidTriggerDropItem>(SpecialItemClass, DropLocation, FRotator::ZeroRotator);
          if (SpawnedItem)
          {
             int32 RandomItem = FMath::RandRange(0, 2);
             SpawnedItem->ItemType = static_cast<ESpecialItemType>(RandomItem);
             SpawnedItem->UpdateAppearance();
             
             // UE_LOG(LogTemp, Warning, TEXT("🎁 특수 아이템 드랍 성공! (확률: %f)"), 0.02f + SpecialBonus);
          }
       }
    }
    
    // 4. 경험치 구슬 드랍 (기본 100%)
    if (ExpOrbClass)
    {
       GetWorld()->SpawnActor<AActor>(ExpOrbClass, DropLocation, FRotator::ZeroRotator);
    }

    // 5. 골드 드랍 판정 (기본 확률 + 자원 채굴 보너스)
    float GoldRoll = FMath::FRand();
    if (GoldRoll <= (BaseGoldDropRate + GoldDropBonus))
    {
       if (GoldClass)
       {
          GetWorld()->SpawnActor<AActor>(GoldClass, DropLocation, FRotator::ZeroRotator);
          // UE_LOG(LogTemp, Log, TEXT("💰 골드 드랍 성공! (확률: %f)"), BaseGoldDropRate + GoldDropBonus);
       }
    }
    
    // 6. 비활성화 처리
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    GetWorldTimerManager().ClearTimer(AudioTimerHandle);
}

void AVoidTriggerEnemy::Activate(FVector NewLocation)
{
    bIsActive = true;
    
    CurrentHealth = MaxHealth;
    SetActorLocation(NewLocation);
    
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(true);
}

void AVoidTriggerEnemy::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);

    AVoidTriggerCharacter* Player = Cast<AVoidTriggerCharacter>(OtherActor);
    if (Player && bCanAttack)
    {
       UGameplayStatics::ApplyDamage(Player, AttackDamage, GetController(), this, UDamageType::StaticClass());

       bCanAttack = false;
       FTimerHandle AttackTimerHandle;
       GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &AVoidTriggerEnemy::ResetAttack, AttackDelay, false);
    }
}

void AVoidTriggerEnemy::ResetAttack()
{
    bCanAttack = true;
}

void AVoidTriggerEnemy::PlayPeriodicSound()
{
    // 이미 만들어둔 오디오 컴포넌트와 사운드 큐 사용
    if (EnemySoundCue && AudioComponent)
    {
       // 사운드 큐 내부의 랜덤 노드가 호출될 때마다 다른 소리를 골라줌
       AudioComponent->SetSound(EnemySoundCue);
       AudioComponent->Play();
    }
}

// =======================================================
// ★ [추가] 웨이브 단계에 맞춰 체력을 스케일링하는 함수
// =======================================================
void AVoidTriggerEnemy::ApplyWaveScaling(int32 WaveLevel, float HealthIncreaseRate)
{
    // 혹시라도 BeginPlay 전에 호출될 경우를 대비해 확실히 저장
    if (BaseMaxHP < 0.0f)
    {
        BaseMaxHP = MaxHealth;
    }

    // 증가율 계산 (1웨이브는 1.0배, 2웨이브부터 증가)
    float Multiplier = 1.0f + ((WaveLevel - 1) * HealthIncreaseRate);
    
    // 항상 변질되지 않은 원래 체력에 곱해줌
    MaxHealth = BaseMaxHP * Multiplier;
    
    // 늘어난 최대 체력만큼 현재 체력도 가득 채워줌
    CurrentHealth = MaxHealth;
}