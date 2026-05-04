#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoidTriggerDropItem.generated.h"

// 3가지 특수 아이템 종류
UENUM(BlueprintType)
enum class ESpecialItemType : uint8
{
	Magnet      UMETA(DisplayName = "자석 (경험치 끌어당기기)"),
	Nuke        UMETA(DisplayName = "폭탄 (전체 화면 공격)"),
	Heal        UMETA(DisplayName = "구급상자 (체력 회복)")
};

UCLASS()
class VOIDTRIGGER_API AVoidTriggerDropItem : public AActor
{
	GENERATED_BODY()
    
public:	
	AVoidTriggerDropItem();

protected:
	virtual void BeginPlay() override;

	// 플레이어와 닿았는지 체크할 충돌체
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionSphere;

	// 아이템의 외형 (블루프린트에서 설정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ItemMesh;

public:	
	// 에디터에서 이 아이템이 자석인지, 폭탄인지 설정할 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Data")
	ESpecialItemType ItemType;

	// 닿았을 때 실행될 함수
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
						UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
						bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Item")
	void UpdateAppearance();
};