#pragma once

#include "CoreMinimal.h"
#include "VoidTriggerEnemy.h"
#include "VoidTriggerFlyingEnemy.generated.h"

UCLASS()
class VOIDTRIGGER_API AVoidTriggerFlyingEnemy : public AVoidTriggerEnemy
{
	GENERATED_BODY()

public:
	AVoidTriggerFlyingEnemy();

protected:
	virtual void BeginPlay() override;protected:
	virtual void HandleMovement(float DeltaTime) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void Activate(FVector NewLocation) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	// 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying")
	float FlySpeed = 250.f;

	// 플레이어와 유지할 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying")
	float DesiredRange = 1000.f;

	// 거리 오차 허용값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying")
	float RangeTolerance = 150.f;

	// 기본 공중 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying")
	float HoverHeight = 1000.f;

	// 위아래 흔들림 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying")
	float HoverAmplitude = 60.f;

	// 위아래 흔들림 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying")
	float HoverFrequency = 2.f;

	// 높이 보정 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flying")
	float HeightAdjustSpeed = 4.f;

	// 발사체 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TSubclassOf<AActor> ProjectileClass;

	// 발사 간격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float ShootInterval = 2.0f;

	// 발사 시 재생할 사운드 에셋 (Sound Cue 또는 Sound Wave)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* FireSoundAsset;

	// 사운드 감쇠 설정 (3D 서라운드 효과를 위해 필수)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundAttenuation* AttenuationAsset;
	
	// 발사 가능 여부
	bool bCanShoot = true;

	// 시작 위치 기준 시간 오프셋
	float HoverTimeOffset = 0.f;

	FTimerHandle ShootTimerHandle;

	void MoveAroundPlayer(float DeltaTime);
	void TryShoot();
	void ResetShootCooldown();
};