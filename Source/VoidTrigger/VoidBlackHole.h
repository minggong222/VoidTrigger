// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoidBlackHole.generated.h"

UCLASS()
class VOIDTRIGGER_API AVoidBlackHole : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVoidBlackHole();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|BlackHole")
	float DamagePerSecond = 15.0f; // 초당 데미지

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|BlackHole")
	float DamageTickRate = 0.5f; // 데미지 주기 (0.5초마다 타격)

	FTimerHandle DamageTimerHandle;

	// 반경 안의 적에게 데미지를 가하는 함수
	UFUNCTION()
	void DealDamageToTrappedEnemies();

};
