#pragma once

#include "CoreMinimal.h"
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
};