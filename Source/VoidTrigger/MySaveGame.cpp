// Fill out your copyright notice in the Description page of Project Settings.


#include "MySaveGame.h"

UMySaveGame::UMySaveGame()
{
	// 1. 기본 재화 및 옵션 세팅 초기화
	TotalGold = 0;
	SavedSens = 1.0f;
	SavedBGM = 1.0f;
	SavedSFX = 1.0f;

	// 2. 로비 영구 특성 레벨 맵 비우기 (초기엔 아무것도 찍지 않은 상태)
	TraitLevels.Empty();

	// 3. 무기고 포인트 분배 데이터 초기화
	bHasSavedStartingTrait = false;
	SavedArmoryWeaponStats.Empty();
    
	// 기본적으로 모든 무기의 투자 포인트는 0으로 시작하도록 세팅
	SavedArmoryWeaponStats.Add(EStartingWeaponType::PierceShot, 0);
	SavedArmoryWeaponStats.Add(EStartingWeaponType::ScatterShot, 0);
	SavedArmoryWeaponStats.Add(EStartingWeaponType::ExplosiveShot, 0);
	SavedArmoryWeaponStats.Add(EStartingWeaponType::ChainLightning, 0);
	SavedArmoryWeaponStats.Add(EStartingWeaponType::BlackHoleGrenade, 0);
	SavedArmoryWeaponStats.Add(EStartingWeaponType::AcidFloor, 0);
	SavedArmoryWeaponStats.Add(EStartingWeaponType::AutoDrone, 0);

	// 4. ★ [최종 반영] 무기고 특수 무기 업적 해금 상태 초기화
	UnlockedWeapons.Empty();
    
	// 관통탄만 처음부터 이용 가능하도록 True 처리, 나머지는 전부 잠금(False)
	UnlockedWeapons.Add(EStartingWeaponType::PierceShot, true);
    
	UnlockedWeapons.Add(EStartingWeaponType::ScatterShot, false);
	UnlockedWeapons.Add(EStartingWeaponType::ExplosiveShot, false);
	UnlockedWeapons.Add(EStartingWeaponType::ChainLightning, false);
	UnlockedWeapons.Add(EStartingWeaponType::BlackHoleGrenade, false);
	UnlockedWeapons.Add(EStartingWeaponType::AcidFloor, false);
	UnlockedWeapons.Add(EStartingWeaponType::AutoDrone, false);
	
	TotalMonsterKills = 0;
	TotalCriticalHits = 0;
	bEnableOverlay = true;
}

int32 UMySaveGame::GetTotalEarnedArmoryPoints() const
{
    int32 TotalPoints = 0;
    const int32 MaxTraitLevel = 5; // 데이터 테이블 상의 만렙 기준 수치

    // ---------------------------------------------------------
    // [1티어 세트] 화력 강화 / 체력 단련 / 자원 채굴
    // ---------------------------------------------------------
    if (TraitLevels.FindRef("ATK_Damage") == MaxTraitLevel &&
        TraitLevels.FindRef("SUR_Health") == MaxTraitLevel &&
        TraitLevels.FindRef("UTL_Drop") == MaxTraitLevel)
    {
        TotalPoints += 1;
    }

    // ---------------------------------------------------------
    // [2티어 세트] 전술 장전 / 전술 기동 / 보급품 탐색
    // ---------------------------------------------------------
    if (TraitLevels.FindRef("ATK_Reload") == MaxTraitLevel &&
        TraitLevels.FindRef("SUR_Speed") == MaxTraitLevel &&
        TraitLevels.FindRef("UTL_Rarity") == MaxTraitLevel)
    {
        TotalPoints += 1;
    }

    // ---------------------------------------------------------
    // [3티어 세트] 급소 조준 / 방호복 강화 / 시작 보급
    // ---------------------------------------------------------
    if (TraitLevels.FindRef("ATK_Crit") == MaxTraitLevel &&
        TraitLevels.FindRef("SUR_Armor") == MaxTraitLevel &&
        TraitLevels.FindRef("UTL_StartCash") == MaxTraitLevel)
    {
        TotalPoints += 1;
    }

    // ---------------------------------------------------------
    // [4티어 세트] 탄창 확장 / 전장 응급처치 / 상인 로비
    // ---------------------------------------------------------
    if (TraitLevels.FindRef("ATK_Mag") == MaxTraitLevel &&
        TraitLevels.FindRef("SUR_Heal") == MaxTraitLevel &&
        TraitLevels.FindRef("UTL_Discount") == MaxTraitLevel)
    {
        TotalPoints += 1;
    }

    // ---------------------------------------------------------
    // [5티어 세트] 제압 사격 / 불굴의 의지 / 전략적 선택
    // ---------------------------------------------------------
    if (TraitLevels.FindRef("ATK_Suppress") == MaxTraitLevel &&
        TraitLevels.FindRef("SUR_Revive") == MaxTraitLevel &&
        TraitLevels.FindRef("UTL_Reroll") == MaxTraitLevel) // 테이블 고유 오타 반영 (l 세 개)
    {
        TotalPoints += 1;
    }

    return TotalPoints;
}

int32 UMySaveGame::GetAvailableArmoryPoints() const
{
    // 1. 세로 티어 세트 완성으로 얻은 총 포인트 (0 ~ 5개)
    int32 EarnedPoints = GetTotalEarnedArmoryPoints();

    // 2. 현재 무기들에 이미 분배해서 사용 중인 총 포인트 합산
    int32 SpentPoints = 0;
    for (const auto& Pair : SavedArmoryWeaponStats)
    {
        SpentPoints += Pair.Value;
    }

    // 3. 가용 포인트 = 총 획득 포인트 - 소모한 포인트 (데이터 오류 방지를 위한 Max 방어벽)
    return FMath::Max(0, EarnedPoints - SpentPoints);
}