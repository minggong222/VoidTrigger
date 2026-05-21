#pragma once

#include "CoreMinimal.h"
#include "VoidTriggerCharacter.h" // EStartingWeaponType을 사용하기 위해 필요
#include "Engine/GameInstance.h"
#include "VoidTriggerGameInstance.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOverlaySettingChanged, bool, bIsOverlayEnabled);

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
	
	// 2. 개설한 주파수를 실제 변수로 선언 (방송 마이크)
	// 캐릭터들은 이 마이크에 자신의 귀(함수)를 연결(Bind)하게 됩니다.
	UPROPERTY(BlueprintAssignable)
	FOnOverlaySettingChanged OnOverlaySettingChanged;

	// 3. 현재 켜져있는지 꺼져있는지 기억해두는 메모지
	// 새로 스폰된 적이 "지금 설정 어떻게 되어있어?"라고 물어볼 때 대답해주는 용도입니다.
	UPROPERTY(BlueprintReadOnly)
	bool bCurrentOverlayState = true;

	// 4. UI 설정창(블루프린트)에서 버튼을 눌렀을 때 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetAndSaveOverlaySetting(bool bEnable);
	
	// 레이더에 표시될 적들의 목록
	UPROPERTY(BlueprintReadOnly, Category = "Radar")
	TArray<class AActor*> ActiveRadarTargets;

	// 적이 스폰될 때 호출
	void RegisterRadarTarget(AActor* Target);
    
	// 적이 죽거나 파괴될 때 호출
	void UnregisterRadarTarget(AActor* Target);
};	