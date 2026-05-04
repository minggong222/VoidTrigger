#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VoidTriggerGameMode.generated.h"

class AVoidTriggerEnemy;

// =======================================================
// 1. 단일 적 스폰 데이터 (가중치 포함)
// =======================================================
USTRUCT(BlueprintType)
struct FEnemySpawnData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AVoidTriggerEnemy> EnemyClass;

    // 가중치 (예: 슬라임 10, 보스 1 이면 슬라임이 10배 더 자주 나옴)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnWeight = 1.0f; 
};

// =======================================================
// 2. [신규] 한 웨이브의 모든 정보를 담는 디렉터 구조체
// =======================================================
USTRUCT(BlueprintType)
struct FWaveDirectorData
{
    GENERATED_BODY()

    // 이 웨이브에 등장할 적들의 리스트와 가중치
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FEnemySpawnData> WaveEnemyList;

    // 이 웨이브의 스폰 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WaveSpawnInterval = 1.0f;
};

// =======================================================
// 3. 오브젝트 풀링을 위한 TArray 래퍼 구조체
// =======================================================
USTRUCT()
struct FEnemyPoolArray
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<AVoidTriggerEnemy*> Pool;
};

// =======================================================
// 게임 모드 클래스
// =======================================================
UCLASS()
class VOIDTRIGGER_API AVoidTriggerGameMode : public AGameModeBase
{
    GENERATED_BODY()
    
public:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game|Time")
    int32 ElapsedGameTime = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game|Wave")
    int32 CurrentWave = 1;

    // ★ [신규] 에디터에서 자유롭게 기획할 수 있는 '웨이브 진행 단계' 배열
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Director")
    TArray<FWaveDirectorData> WaveStages;

    // 플레이어와의 스폰 거리
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
    float SpawnDistance = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Director")
    TSubclassOf<AVoidTriggerEnemy> BossClass;
    
    bool bIsBossSpawned = false;
    // 보스가 죽었을 때 호출할 함수
    void OnBossDefeated();

    // 현재 발악(엔드리스) 모드인지 확인하는 플래그
    bool bIsEndlessMode = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Director")
    float HealthIncreasePerWave = 0.2f; // 기본값 0.2 = 웨이브당 20%씩 증가
    // 어떤 적이든 자동으로 분류해서 담아주는 만능 풀(창고)
    UPROPERTY()
    TMap<UClass*, FEnemyPoolArray> EnemyPoolMap;
    
    FTimerHandle SpawnTimerHandle;
    FTimerHandle GameTimerHandle;

    void SpawnEnemy();
    void UpdateGameTimer();
    void GameOver();

private:
    // 내부적으로 현재 굴러가고 있는 스폰 리스트와 간격 (에디터 노출 X)
    TArray<FEnemySpawnData> CurrentEnemySpawnList;
    float CurrentSpawnInterval = 1.0f;
};