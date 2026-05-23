#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
// ★ [중요] EStartingWeaponType Enum을 인식하기 위해 캐릭터 헤더를 인클루드합니다.
#include "VoidTriggerCharacter.h" 
#include "MySaveGame.generated.h"

UCLASS()
class VOIDTRIGGER_API UMySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UMySaveGame();
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

	// ==========================================
	// [추가] 무기고 특수 무기 스탯 데이터
	// ==========================================
    
	// 무기 타입별로 몇 포인트를 투자했는지 영구 저장하는 맵
	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Armory")
	TMap<EStartingWeaponType, int32> SavedArmoryWeaponStats;

	// 무기고 세팅을 한 번이라도 저장했는지 판별하는 스위치
	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Armory")
	bool bHasSavedStartingTrait = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Armory")
	TMap<EStartingWeaponType, bool> UnlockedWeapons;
	
	UPROPERTY(BlueprintReadWrite, Category = "SaveData|ArmoryUnlock")
	int32 TotalMonsterKills = 0;

	UPROPERTY(BlueprintReadWrite, Category = "SaveData|ArmoryUnlock")
	int32 TotalCriticalHits = 0;
	
	// 세로 티어 세트가 완성된 개수를 세어 총 획득 포인트를 계산 (최대 5개)
	UFUNCTION(BlueprintPure, Category = "SaveData|Armory")
	int32 GetTotalEarnedArmoryPoints() const;

	// 총 획득 포인트에서 이미 무기들에 투자한 포인트를 빼고 남은 가용 포인트 계산
	UFUNCTION(BlueprintPure, Category = "SaveData|Armory")
	int32 GetAvailableArmoryPoints() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    bool bEnableOverlay = true;
	
	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Records")
	int32 BestSurvivalTime = 0; // 최고 생존 시간 (초 단위)

	UPROPERTY(BlueprintReadWrite, Category = "SaveData|Records")
	int32 BestRunKills = 0;     // 한 게임 최고 처치 수
};