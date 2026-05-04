#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MySaveGame.generated.h"

UCLASS()
class VOIDTRIGGER_API UMySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 보유 중인 전체 재화
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	int32 TotalGold = 0;

	// 특성별 현재 레벨 (Key: 특성ID, Value: 레벨)
	// TMap은 파이썬의 Dictionary나 C#의 Dictionary와 같습니다.
	UPROPERTY(BlueprintReadWrite, Category = "SaveData")
	TMap<FName, int32> TraitLevels;
	
	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Settings")
	float SavedSens = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Settings")
	float SavedBGM = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Settings")
	float SavedSFX = 1.0f;
};