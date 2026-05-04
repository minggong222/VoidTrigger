// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoidTriggerItem.generated.h"

UCLASS()
class VOIDTRIGGER_API AVoidTriggerItem : public AActor
{
	GENERATED_BODY()
    
public: 
	// 생성자
	AVoidTriggerItem();

protected:
	// 게임 시작 시 호출
	virtual void BeginPlay() override;

public: 
	// 매 프레임 호출
	virtual void Tick(float DeltaTime) override;

	// --- 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UStaticMeshComponent* MeshComponent;

	// --- 아이템 설정 (블루프린트에서 수정 가능) ---
	UPROPERTY(EditAnywhere, Category = "Item Settings")
	float MoveSpeed = 10.0f; // 플레이어를 따라가는 보간 속도

	UPROPERTY(EditAnywhere, Category = "Item Settings")
	float RotationSpeed = 150.0f; // 바닥에서 회전하는 속도

	UPROPERTY(EditAnywhere, Category = "Item Settings")
	float BobbingAmplitude = 0.5f; // 위아래로 흔들리는 진폭

	// --- 상태 변수 ---
	bool bIsFlying = false;

	// 날아가기 시작하는 트리거 함수
	void StartFlying();
	
private:
	// 상하 이동(Bobbing) 계산을 위한 누적 시간
	float RunningTime = 0.f;
};