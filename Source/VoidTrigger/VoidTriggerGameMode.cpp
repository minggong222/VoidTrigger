#include "VoidTriggerGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "VoidTriggerEnemy.h"

void AVoidTriggerGameMode::BeginPlay()
{
    Super::BeginPlay();

    // ★ [핵심] 게임 시작 시 1웨이브(Index 0) 데이터 장전
    if (WaveStages.Num() > 0)
    {
        CurrentEnemySpawnList = WaveStages[0].WaveEnemyList;
        CurrentSpawnInterval = WaveStages[0].WaveSpawnInterval;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode: 설정된 WaveStages 데이터가 없습니다!"));
    }

    GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AVoidTriggerGameMode::SpawnEnemy, CurrentSpawnInterval, true);
    GetWorldTimerManager().SetTimer(GameTimerHandle, this, &AVoidTriggerGameMode::UpdateGameTimer, 1.0f, true);
}

void AVoidTriggerGameMode::UpdateGameTimer()
{
    ElapsedGameTime++;
    
    if (bIsBossSpawned && !bIsEndlessMode) 
    {
        return; 
    }
    
    // ★ [추가] 발악 페이즈라면 매초마다 스폰 주기를 미친듯이 줄입니다!
    if (bIsEndlessMode)
    {
        // 최저 0.1초 쿨타임까지 도달하도록 0.02초씩 계속 줄임
        CurrentSpawnInterval = FMath::Max(0.1f, CurrentSpawnInterval - 0.02f);
        
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
        GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AVoidTriggerGameMode::SpawnEnemy, CurrentSpawnInterval, true);
        return; // 아래의 일반 웨이브 로직은 타지 않음
    }
    
    int32 NewWave = (ElapsedGameTime / 120) + 1; // 30초마다 웨이브 증가

    // 웨이브가 변경되었다면?
    if (NewWave > CurrentWave)
    {
        CurrentWave = NewWave;
        UE_LOG(LogTemp, Warning, TEXT("★★★ 웨이브 %d 시작! ★★★"), CurrentWave);

        // ★ [핵심] 준비된 웨이브 데이터가 있는지 확인 (배열 인덱스는 0부터 시작하므로 -1)
        if (WaveStages.IsValidIndex(CurrentWave - 1))
        {
            FWaveDirectorData CurrentWaveData = WaveStages[CurrentWave - 1];
            
            // 이번 웨이브의 리스트와 스폰 속도로 갈아끼우기
            CurrentEnemySpawnList = CurrentWaveData.WaveEnemyList;
            CurrentSpawnInterval = CurrentWaveData.WaveSpawnInterval;

            // 스폰 타이머 재시작
            GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
            GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AVoidTriggerGameMode::SpawnEnemy, CurrentSpawnInterval, true);
        }
        else
        {
            // 준비된 기획 단계를 모두 넘겼다면, 기존처럼 무한 스케일링 (속도만 더 빠르게)
            CurrentSpawnInterval = FMath::Max(0.2f, CurrentSpawnInterval - 0.15f);
            
            GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
            GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AVoidTriggerGameMode::SpawnEnemy, CurrentSpawnInterval, true);
        }
    }
}

void AVoidTriggerGameMode::SpawnEnemy()
{
    // ★ 기존 EnemySpawnList 대신, 현재 장전된 'CurrentEnemySpawnList'를 사용합니다.
    if (CurrentEnemySpawnList.IsEmpty()) return;

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn) return;

    // --- 1. 가중치 기반 랜덤(Weighted Random) 알고리즘 ---
    float TotalWeight = 0.f;
    for (const FEnemySpawnData& Data : CurrentEnemySpawnList)
    {
        TotalWeight += Data.SpawnWeight;
    }

    float RandomValue = FMath::FRandRange(0.f, TotalWeight);
    TSubclassOf<AVoidTriggerEnemy> SelectedClass = nullptr;

    for (const FEnemySpawnData& Data : CurrentEnemySpawnList)
    {
        RandomValue -= Data.SpawnWeight;
        if (RandomValue <= 0.f)
        {
            SelectedClass = Data.EnemyClass;
            break;
        }
    }

    if (!SelectedClass) return;

    if (BossClass != nullptr && SelectedClass == BossClass)
    {
        // ★ [추가] 혹시라도 타이머가 겹쳐서 두 마리가 나오려는 걸 방지
        if (bIsBossSpawned) return; 

        bIsBossSpawned = true; // ★ [추가] 보스 등장 상태로 전환!

        // 타이머를 꺼서 이 함수가 더 이상 자동으로 반복 실행되지 않게 막습니다.
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
        UE_LOG(LogTemp, Error, TEXT("👑 보스 등장! 일반 몬스터 자동 스폰을 중지합니다."));
    }
    
    // --- 2. 위치 계산 ---
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    float RandomAngle = FMath::RandRange(0.f, 360.f);
    FVector SpawnDirection = FVector(FMath::Cos(FMath::DegreesToRadians(RandomAngle)), FMath::Sin(FMath::DegreesToRadians(RandomAngle)), 0.f);
    FVector SpawnLocation = PlayerLocation + (SpawnDirection * SpawnDistance);

    // ★ [핵심 추가] 스폰 위치를 플레이어 허리에서 바닥으로 강제 하강시킵니다.
    SpawnLocation.Z -= 90.f;

    // --- 3. 유동적 오브젝트 풀링 (TMap 활용) ---
    UClass* ClassPtr = SelectedClass.Get();
    AVoidTriggerEnemy* EnemyToSpawn = nullptr;

    if (FEnemyPoolArray* PoolArray = EnemyPoolMap.Find(ClassPtr))
    {
        for (AVoidTriggerEnemy* Enemy : PoolArray->Pool)
        {
            if (Enemy && !Enemy->bIsActive) 
            {
                EnemyToSpawn = Enemy;
                break;
            }
        }
    }

    // B. 재활용
    if (EnemyToSpawn)
    {
        EnemyToSpawn->Activate(SpawnLocation); 
    }
    // C. 없으면 생성 후 창고에 등록
    else
    {
        EnemyToSpawn = GetWorld()->SpawnActor<AVoidTriggerEnemy>(SelectedClass, SpawnLocation, FRotator::ZeroRotator);
        if (EnemyToSpawn)
        {
            EnemyPoolMap.FindOrAdd(ClassPtr).Pool.Add(EnemyToSpawn);
        }
    }

    // =======================================================
    // ★ [핵심] 재활용해서 나왔든 새로 태어났든, 세상에 나오기 직전에 체력 펌핑!
    // =======================================================
    if (EnemyToSpawn)
    {
        EnemyToSpawn->ApplyWaveScaling(CurrentWave, HealthIncreasePerWave);
    }
}

void AVoidTriggerGameMode::OnBossDefeated()
{
    UE_LOG(LogTemp, Error, TEXT("💀 보스 처치! 끝없는 공허(발악 페이즈)가 시작됩니다..."));
    
    bIsEndlessMode = true;

    // 스폰 속도를 0.5초로 팍 줄여버립니다.
    CurrentSpawnInterval = 0.5f; 

    // 마지막 웨이브(가장 강한 몹들) 리스트를 강제로 장전
    if (WaveStages.Num() > 0)
    {
        CurrentEnemySpawnList = WaveStages.Last().WaveEnemyList;
    }

    // 스폰 타이머 지옥 모드로 재가동!
    GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AVoidTriggerGameMode::SpawnEnemy, CurrentSpawnInterval, true);
}

void AVoidTriggerGameMode::GameOver() 
{
    // 게임 오버 로직
}