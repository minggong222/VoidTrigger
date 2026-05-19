// VoidDrone.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoidDrone.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class VOIDTRIGGER_API AVoidDrone : public AActor
{
	GENERATED_BODY()

public:

	AVoidDrone();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

public:

	// =========================================================
	// 컴포넌트
	// =========================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DroneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* DetectionSphere;

	// =========================================================
	// 드론 설정
	// =========================================================

	UPROPERTY()
	AActor* OwnerPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone")
	FVector HoverOffset;

	// =========================================================
	// 전투 설정
	// =========================================================

	FTimerHandle FireTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FireRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Damage = 15.0f;

	// =========================================================
	// 이펙트
	// =========================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* BulletTracerEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* HitEffect;
	// =========================================================
	// 함수
	// =========================================================

	void InitializeDrone(AActor* InOwner);

	UFUNCTION()
	void FindTargetAndShoot();
};