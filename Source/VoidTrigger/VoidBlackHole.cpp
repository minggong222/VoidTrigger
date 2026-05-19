// Fill out your copyright notice in the Description page of Project Settings.


#include "VoidBlackHole.h"

#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
// Sets default values
AVoidBlackHole::AVoidBlackHole()
{
	PrimaryActorTick.bCanEverTick = true;

	// 블랙홀 유지 시간 (예: 3초 뒤 자동 소멸)
	InitialLifeSpan = 3.0f;
}

// Called when the game starts or when spawned
void AVoidBlackHole::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(
		DamageTimerHandle, 
		this, 
		&AVoidBlackHole::DealDamageToTrappedEnemies, 
		DamageTickRate, 
		true
	);
}

// Called every frame
void AVoidBlackHole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 블랙홀의 중심 위치
	FVector CenterLocation = GetActorLocation();

	// 끌어당길 반경 (예: 1000cm = 10미터)
	float PullRadius = 1000.f; 
    
	// 끌어당기는 속도 (수치가 클수록 맹렬하게 빨려옴)
	float PullSpeed = 400.f; 

	// 반경 내의 액터 탐색
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(PullRadius);
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bFoundActors = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		CenterLocation,
		FQuat::Identity,
		ECC_Visibility,
		SphereShape,
		Params
	);

	if (bFoundActors)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* TargetActor = Result.GetActor();
            
			// 몬스터 태그가 있는 대상만 끌어당김
			if (TargetActor && TargetActor->ActorHasTag(FName("Monster")) && !TargetActor->ActorHasTag(FName("Flying")))
			{
				FVector TargetLocation = TargetActor->GetActorLocation();
                
				// 몬스터에서 블랙홀을 향하는 방향 벡터
				FVector PullDirection = (CenterLocation - TargetLocation).GetSafeNormal();

				// 부드럽게 끌려오도록 위치 계산 (장애물/벽에 막히도록 Sweep 옵션 true)
				FVector NewLocation = TargetLocation + (PullDirection * PullSpeed * DeltaTime);
				TargetActor->SetActorLocation(NewLocation, true);
			}
		}
	}
}


void AVoidBlackHole::DealDamageToTrappedEnemies()
{
	FVector CenterLocation = GetActorLocation();
    
	// 기존 Tick에 있던 끌어당기기 반경과 똑같이 맞춰줍니다 (예: 1000.f)
	float DamageRadius = 1000.f; 

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(DamageRadius);
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// 반경 내의 액터 탐색
	bool bFoundActors = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		CenterLocation,
		FQuat::Identity,
		ECC_Visibility,
		SphereShape,
		Params
	);

	if (bFoundActors)
	{
		// 0.5초마다 들어가야 할 실제 데미지 계산 (초당 15뎀 * 0.5 = 1타격당 7.5뎀)
		float ActualTickDamage = DamagePerSecond * DamageTickRate;

		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* TargetActor = Result.GetActor();
            
			// 몬스터에게만 데미지 적용
			if (TargetActor && TargetActor->ActorHasTag(FName("Monster")))
			{
				UGameplayStatics::ApplyDamage(
					TargetActor, 
					ActualTickDamage, 
					nullptr, // 무기를 쏜 주인을 넘겨줄 수도 있지만, 블랙홀 자체 데미지이므로 일단 nullptr
					this, 
					UDamageType::StaticClass()
				);
			}
		}
	}
}