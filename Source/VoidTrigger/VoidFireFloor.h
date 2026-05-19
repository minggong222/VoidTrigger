#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoidFireFloor.generated.h"

class USphereComponent;
class UNiagaraComponent;

UCLASS()
class VOIDTRIGGER_API AVoidFireFloor : public AActor
{
	GENERATED_BODY()
    
public:    
	AVoidFireFloor();

protected:
	virtual void BeginPlay() override;

public:
	// 장판의 범위를 설정할 투명한 구체 충돌체
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* DamageCollision;

	// 불길을 표현할 나이아가라 이펙트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* FireEffect;

	// 1회 타격당 데미지 (초당 데미지가 아님)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DamagePerTick = 5.0f; 

	// 데미지가 들어가는 주기 (0.5초마다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DamageTickRate = 0.5f;

	// 장판 크기 (반경 150 = 1.5미터)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FloorRadius = 150.0f;

	FTimerHandle DamageTimerHandle;

	// 범위 내 적들에게 화염 데미지를 주는 함수
	UFUNCTION()
	void DealBurnDamage();
};