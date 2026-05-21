// Fill out your copyright notice in the Description page of Project Settings.


#include "VoidTriggerGameInstance.h"

#include "MySaveGame.h"
#include "Kismet/GameplayStatics.h"

void UVoidTriggerGameInstance::SetAndSaveOverlaySetting(bool bEnable)
{
	// 1. 현재 상태 캐싱 변수 업데이트
	bCurrentOverlayState = bEnable;

	// 2. 델리게이트 브로드캐스트 (맵에 있는 모든 액터에게 알림)
	OnOverlaySettingChanged.Broadcast(bEnable);

	// 3. 디스크에 저장 (SaveGame 로직)
	if (UMySaveGame* SaveInst = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass())))
	{
		SaveInst->bEnableOverlay = bEnable;
		UGameplayStatics::SaveGameToSlot(SaveInst, TEXT("SettingsSlot"), 0);
	}
}
