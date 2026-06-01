#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TraitData.generated.h"

// 1. 언리얼 엔진에게 "이건 블루프린트에서도 쓸 수 있는 구조체야!"라고 알려주는 이름표입니다.
USTRUCT(BlueprintType)
struct FTraitData : public FTableRowBase
{
	// 2. 엔진이 내부적으로 필요한 코드를 자동 생성해 주는 마법의 주문입니다. 무조건 써야 합니다.
	GENERATED_BODY()

public:
	// 3. 에디터에서 값을 수정하고(EditAnywhere), 블루프린트에서 읽고 쓰게(BlueprintReadWrite) 해주는 설정입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trait")
	FName TraitID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trait")
	FText TraitName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trait")
	int32 MaxLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trait")
	int32 BaseCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trait")
	FName PrerequisiteTraitID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trait")
	int32 PrerequisiteLevel;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trait")
	FText Description;
};