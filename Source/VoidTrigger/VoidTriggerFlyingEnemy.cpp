#include "VoidTriggerFlyingEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

AVoidTriggerFlyingEnemy::AVoidTriggerFlyingEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->AirControl = 1.f;
	GetCharacterMovement()->BrakingDecelerationFlying = 2048.f;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
}

void AVoidTriggerFlyingEnemy::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);

	// 개체마다 흔들리는 타이밍이 약간 다르게
	HoverTimeOffset = FMath::FRandRange(0.f, 1000.f);
}

void AVoidTriggerFlyingEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive)
	{
		return;
	}

	MoveAroundPlayer(DeltaTime);
	TryShoot();
}

void AVoidTriggerFlyingEnemy::Activate(FVector NewLocation)
{
	Super::Activate(NewLocation);

	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);

	bCanShoot = true;
}

void AVoidTriggerFlyingEnemy::MoveAroundPlayer(float DeltaTime)
{
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerActor)
	{
		return;
	}

	FVector MyLocation = GetActorLocation();
	FVector PlayerLocation = PlayerActor->GetActorLocation();

	FVector ToPlayer = PlayerLocation - MyLocation;
	FVector HorizontalToPlayer(ToPlayer.X, ToPlayer.Y, 0.f);
	float Distance = HorizontalToPlayer.Size();

	FVector MoveDir = FVector::ZeroVector;

	const float MinDistance = 500.f;

	if (Distance < MinDistance)
	{
		MoveDir = -HorizontalToPlayer.GetSafeNormal();
	}
	else if (Distance < DesiredRange - RangeTolerance)
	{
		MoveDir = -HorizontalToPlayer.GetSafeNormal();
	}
	else if (Distance > DesiredRange + RangeTolerance)
	{
		MoveDir = HorizontalToPlayer.GetSafeNormal();
	}
	else
	{
		FVector SideDir = FVector::CrossProduct(FVector::UpVector, HorizontalToPlayer.GetSafeNormal());
		MoveDir = SideDir;
	}

	FVector NewLocation = MyLocation + MoveDir * FlySpeed * DeltaTime;

	float Time = GetWorld()->GetTimeSeconds() + HoverTimeOffset;
	float HoverOffset = FMath::Sin(Time * HoverFrequency) * HoverAmplitude;
	float TargetZ = PlayerLocation.Z + HoverHeight + HoverOffset;

	NewLocation.Z = FMath::FInterpTo(MyLocation.Z, TargetZ, DeltaTime, HeightAdjustSpeed);

	SetActorLocation(NewLocation);

	FRotator LookAtRotation = (PlayerLocation - NewLocation).Rotation();
	SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));
}

void AVoidTriggerFlyingEnemy::HandleMovement(float DeltaTime)
{
	MoveAroundPlayer(DeltaTime);
}

void AVoidTriggerFlyingEnemy::TryShoot()
{
	if (!bCanShoot || !ProjectileClass || !bIsActive)
	{
		return;
	}

	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerActor)
	{
		return;
	}

	float Distance = FVector::Dist2D(GetActorLocation(), PlayerActor->GetActorLocation());
	if (Distance > DesiredRange + 150.f)
	{
		return;
	}

	bCanShoot = false;

	FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 80.f;
	FRotator SpawnRotation = (PlayerActor->GetActorLocation() - SpawnLocation).Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	// 1. 발사체 스폰
	GetWorld()->SpawnActor<AActor>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	// 2. 발사 사운드 재생 추가
	if (FireSoundAsset) // 헤더에 선언한 USoundBase* 변수
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, 
			FireSoundAsset, 
			SpawnLocation, // 발사체가 생성된 바로 그 위치에서 소리 발생
			FRotator::ZeroRotator,
			1.0f, // 볼륨
			1.0f, // 피치
			0.0f, // 시작 시간
			AttenuationAsset // 헤더에 선언한 USoundAttenuation* 변수
		);
	}

	GetWorldTimerManager().SetTimer(
	   ShootTimerHandle,
	   this,
	   &AVoidTriggerFlyingEnemy::ResetShootCooldown,
	   ShootInterval,
	   false
	);
}

void AVoidTriggerFlyingEnemy::ResetShootCooldown()
{
	bCanShoot = true;
}

void AVoidTriggerFlyingEnemy::NotifyActorBeginOverlap(AActor* OtherActor)
{
	// 아무것도 안 함
}