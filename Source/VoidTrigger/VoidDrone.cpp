// VoidDrone.cpp

#include "VoidDrone.h"

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

	// 1. 빈 씬 컴포넌트를 루트로 생성
	USceneComponent* DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = DefaultRoot;

	// 2. 드론 메시는 루트의 자식으로 설정
	DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
	DroneMesh->SetupAttachment(RootComponent);
	DroneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. 감지 구체도 루트의 자식으로 설정
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

	// 자동 공격 반복
	GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&AVoidDrone::FindTargetAndShoot,
		FireRate,
		true
	);
}

void AVoidDrone::InitializeDrone(AActor* InOwner)
{
	OwnerPlayer = InOwner;
}

void AVoidDrone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Attach 상태라 직접 이동 안함
}

void AVoidDrone::FindTargetAndShoot()
{
	UE_LOG(LogTemp, Warning, TEXT("드론 탐색 실행"));

	TArray<AActor*> OverlappingActors;

	DetectionSphere->GetOverlappingActors(OverlappingActors);

	AActor* ClosestTarget = nullptr;

	float MinDistance = TNumericLimits<float>::Max();

	// =========================================================
	// 가장 가까운 몬스터 탐색
	// =========================================================

	for (AActor* TargetActor : OverlappingActors)
	{
		if (!TargetActor)
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("감지된 액터: %s"), *TargetActor->GetName());

		// Monster 태그 체크
		if (!TargetActor->ActorHasTag(FName("Monster")))
		{
			continue;
		}

		float Distance = FVector::Dist(
			GetActorLocation(),
			TargetActor->GetActorLocation()
		);

		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			ClosestTarget = TargetActor;
		}
	}

	// =========================================================
	// 공격
	// =========================================================

	if (ClosestTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("타겟 공격: %s"), *ClosestTarget->GetName());

		// 데미지 적용
		UGameplayStatics::ApplyDamage(
		   ClosestTarget,
		   Damage,
		   nullptr,
		   this,
		   UDamageType::StaticClass()
		);

		if (HitEffect)
		{
			// 적의 RootComponent에 부착하여 생성합니다.
			// 이렇게 하면 적이 맞아서 날아가더라도 이펙트가 적 몸에 붙어서 같이 움직입니다.
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				HitEffect,
				ClosestTarget->GetRootComponent(), // 부착할 대상
				NAME_None, // 특정 소켓 이름 (필요 없으면 None)
				FVector::ZeroVector, // 로컬 위치 오프셋
				FRotator::ZeroRotator, // 로컬 회전 오프셋
				EAttachLocation::SnapToTarget, // 부착 방식
				true // 자동 파괴 (이펙트 끝나면 사라짐)
			);
		}
		
		// 예광탄 이펙트
		if (BulletTracerEffect)
		{
			// 1. 소켓 위치 가져오기 (⭐이 부분을 수정합니다.)
			// 스태틱 메시 에디터에서 만드신 소켓의 이름("Muzzle")을 정확히 입력해야 합니다.
			FVector StartLocation = DroneMesh->GetSocketLocation(FName("Muzzle"));
			FVector EndLocation = ClosestTarget->GetActorLocation();

			// 2. 이펙트가 날아갈 방향 계산 (소켓 위치 기준)
			FRotator EffectRotation =
			   UKismetMathLibrary::FindLookAtRotation(
				  StartLocation,
				  EndLocation
			   );

			// 3. 소켓 위치에서 이펙트 스폰
			UNiagaraComponent* TracerComp =
			   UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				  GetWorld(),
				  BulletTracerEffect,
				  StartLocation,
				  EffectRotation
			   );

			if (TracerComp)
			{
				// 4. 도착점 설정 (기존과 동일)
				TracerComp->SetVariableVec3(
				   FName("User.BeamEnd"),
				   EndLocation
				);

				// 5. 시작점 설정 (⭐추가: 드론 소켓 위치로!)
				// 나이아가라 이펙트가 이 값을 사용하여 빔의 시작점을 잡습니다.
				TracerComp->SetVariableVec3(
				   FName("User.BeamStart"),
				   StartLocation
				);
			}
		}
	}
}