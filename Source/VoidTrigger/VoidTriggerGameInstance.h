#pragma once

#include "CoreMinimal.h"
#include "VoidTriggerCharacter.h" // EStartingWeaponType을 사용하기 위해 필요
#include "Engine/GameInstance.h"
#include "VoidTriggerGameInstance.generated.h"

UCLASS()
class VOIDTRIGGER_API UVoidTriggerGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ★ 맵이 넘어가도 영원히 보존될 감도 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity = 1.0f;

	// ★ [변경됨] 유저가 분배한 무기고 스탯을 저장할 Map 변수
	// (예: PierceShot -> 1, ExplosiveShot -> 2, AutoDrone -> 2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armory")
	TMap<EStartingWeaponType, int32> ArmoryWeaponStats;

	// 무기고에서 특성을 세팅 완료했는지 확인하는 스위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armory")
	bool bHasStartingTrait = false;
};