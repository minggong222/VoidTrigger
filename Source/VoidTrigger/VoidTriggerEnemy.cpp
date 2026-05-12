// Fill out your copyright notice in the Description page of Project Settings.

#include "VoidTriggerEnemy.h"
#include "VoidTriggerCharacter.h"
#include "VoidTriggerDropItem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PawnMovementComponent.h" // 추가: 무브먼트 제어를 위해 필요

// Sets default values
AVoidTriggerEnemy::AVoidTriggerEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    
    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("EnemyAudioSource"));
    AudioComponent->SetupAttachment(RootComponent);
    AudioComponent->bAutoActivate = false;
}

// Called when the game starts or when spawned
void AVoidTriggerEnemy::BeginPlay()
{
    Super::BeginPlay();
    
    if (BaseMaxHP < 0.0f)
    {
        BaseMaxHP = MaxHealth;
    }

    // 맵에 처음 배치되었을 때 활성화 상태라면 시작 (보통 스폰 매니저가 제어함)
    if (bIsActive)
    {
        Activate(GetActorLocation());
    }
    else
    {
        // 시작부터 풀링 대기 상태라면 완전히 꺼둠
        Deactivate();
    }
}

// Called every frame
void AVoidTriggerEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 풀링 대기 상태면 절대 이동 연산을 하지 않음
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

    if (CurrentHealth <= 0.f)
    {
       Deactivate();
    }

    return DamageAmount;
}

void AVoidTriggerEnemy::Deactivate()
{
    if (!bIsActive) return;
    bIsActive = false;

    // --- 드롭 로직 유지 ---
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

    AVoidTriggerCharacter* Player = Cast<AVoidTriggerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    float GoldDropBonus = Player ? Player->ResourceDropBonus : 0.0f;
    float SpecialBonus = Player ? Player->SpecialDropBonus : 0.0f;

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
          }
       }
    }
    
    if (ExpOrbClass)
    {
       GetWorld()->SpawnActor<AActor>(ExpOrbClass, DropLocation, FRotator::ZeroRotator);
    }

    float GoldRoll = FMath::FRand();
    if (GoldRoll <= (BaseGoldDropRate + GoldDropBonus))
    {
       if (GoldClass)
       {
          GetWorld()->SpawnActor<AActor>(GoldClass, DropLocation, FRotator::ZeroRotator);
       }
    }
    
    // --- 최적화: 풀링 대기 상태로 전환 (완전한 비활성화) ---
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetActorTickEnabled(false);
    
    // 무브먼트 컴포넌트 강제 중지 (매우 중요)
    if (UPawnMovementComponent* MovementComp = GetMovementComponent())
    {
        MovementComp->Deactivate();
    }
	
	// 2. 타이머 멈추기 (더 이상 소리 재생 시도 안 함)
	GetWorldTimerManager().ClearTimer(AudioTimerHandle);

	// 3. ★ 현재 재생 중인 소리 즉시 멈추기 ★
	if (AudioComponent && AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}
}

void AVoidTriggerEnemy::Activate(FVector NewLocation)
{
    bIsActive = true;
    
    CurrentHealth = MaxHealth;
    SetActorLocation(NewLocation);
    
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    SetActorTickEnabled(true);
    
    // 무브먼트 컴포넌트 재활성화
    if (UPawnMovementComponent* MovementComp = GetMovementComponent())
    {
        MovementComp->Activate();
    }

    // --- 오디오 로직 수정 (재사용 시 타이머 재시작) ---
    GetWorldTimerManager().ClearTimer(AudioTimerHandle); // 혹시 모를 중복 방지
    
    // 1. 활성화 즉시 소리 한 번 재생
    PlayPeriodicSound(); 
    
    // 2. 이후 설정된 간격(SoundInterval)마다 반복 재생
    GetWorldTimerManager().SetTimer(
       AudioTimerHandle, 
       this, 
       &AVoidTriggerEnemy::PlayPeriodicSound, 
       SoundInterval, 
       true 
    );
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
    if (EnemySoundCue && AudioComponent)
    {
       AudioComponent->SetSound(EnemySoundCue);
       AudioComponent->Play();
    }
}

void AVoidTriggerEnemy::ApplyWaveScaling(int32 WaveLevel, float HealthIncreaseRate)
{
    if (BaseMaxHP < 0.0f)
    {
        BaseMaxHP = MaxHealth;
    }

    float Multiplier = 1.0f + ((WaveLevel - 1) * HealthIncreaseRate);
    MaxHealth = BaseMaxHP * Multiplier;
    CurrentHealth = MaxHealth;
}