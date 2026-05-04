#include "VoidTriggerEnemyProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "VoidTriggerCharacter.h"

AVoidTriggerEnemyProjectile::AVoidTriggerEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// 충돌체 생성
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;
	CollisionComp->InitSphereRadius(20.f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	// 메쉬 생성
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 이동 컴포넌트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 900.f;
	ProjectileMovement->MaxSpeed = 900.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 5.f;
}

void AVoidTriggerEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();

	InitialLifeSpan = LifeSeconds;

	if (CollisionComp)
	{
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AVoidTriggerEnemyProjectile::OnProjectileOverlap);
	}
}

void AVoidTriggerEnemyProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AVoidTriggerEnemyProjectile::OnProjectileOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	// 플레이어만 맞히고 싶으면 캐스트로 제한
	AVoidTriggerCharacter* PlayerCharacter = Cast<AVoidTriggerCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		UGameplayStatics::ApplyDamage(
			PlayerCharacter,
			Damage,
			GetInstigatorController(),
			this,
			nullptr
		);

		Destroy();
		return;
	}

	// 벽 등 다른 것과 닿았을 때도 제거
	if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
	{
		Destroy();
	}
}