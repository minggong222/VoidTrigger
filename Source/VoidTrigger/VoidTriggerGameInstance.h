#pragma once

#include "CoreMinimal.h"
#include "VoidTriggerCharacter.h"
#include "Engine/GameInstance.h"
#include "VoidTriggerGameInstance.generated.h"

UCLASS()
class VOIDTRIGGER_API UVoidTriggerGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ★ [추가] 맵이 넘어가도 영원히 보존될 감도 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity = 1.0f;

	// 1. 유저가 선택한 무기 특성을 저장할 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armory")
	EStartingWeaponType StartingWeaponTrait = EStartingWeaponType::PierceShot;

	// 2. 무기고에서 특성을 골랐는지 확인하는 스위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armory")
	bool bHasStartingTrait = false;

};