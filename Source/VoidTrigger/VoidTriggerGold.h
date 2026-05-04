#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoidTriggerGold.generated.h"

UCLASS()
class VOIDTRIGGER_API AVoidTriggerGold : public AActor
{
	GENERATED_BODY()
    
public: 
	AVoidTriggerGold();

protected:
	virtual void BeginPlay() override;

public: 
	virtual void Tick(float DeltaTime) override;

	// --- 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UStaticMeshComponent* MeshComponent;

	// --- 설정 ---
	UPROPERTY(EditAnywhere, Category = "Gold Settings")
	float MoveSpeed = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Gold Settings")
	int32 GoldValue = 10; // 이 아이템 하나당 획득할 골드 양

	bool bIsFlying = false;
	void StartFlying();

private:
	float RunningTime = 0.f;
};