// Fill out your copyright notice in the Description page of Project Settings.


#include "VoidBlackHole.h"

#include "Engine/OverlapResult.h"

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
			if (TargetActor && TargetActor->ActorHasTag(FName("Monster")))
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
