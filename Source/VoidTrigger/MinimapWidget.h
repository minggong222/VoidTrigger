// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

/**
 * 
 */
UCLASS()
class VOIDTRIGGER_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* RadarCanvas;

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	TSubclassOf<UUserWidget> EnemyIconClass;

	// 레이더 최대 탐지 반경 (언리얼 유닛, 3000 = 30m)
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float MaxRadarRange = 3000.0f;

	// 레이더 UI의 픽셀 반지름 (예: UI 크기가 300x300이면 150)
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float RadarUIRadius = 150.0f;

private:
	// UI 아이콘 재사용을 위한 오브젝트 풀
	UPROPERTY()
	TArray<UUserWidget*> IconPool;
};
