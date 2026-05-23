#include "VoidTriggerGameMode.h"

#include "MySaveGame.h"
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
    
    if (bIsEndlessMode)
    {
        // 1. 스폰 속도 감소 (최저 0.1초까지)
        CurrentSpawnInterval = FMath::Max(0.1f, CurrentSpawnInterval - 0.02f);
        
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
        GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AVoidTriggerGameMode::SpawnEnemy, CurrentSpawnInterval, true);
        
        // ==========================================
        // ★ [핵심 추가] 시간에 따른 무한 체력 스케일링
        // ==========================================
        EndlessTimeCounter++;
        
        // 5초마다 한 번씩 체력을 올리고 싶다면 (시간은 기획에 맞게 조절하세요!)
        if (EndlessTimeCounter >= 15) 
        {
            CurrentWave++; // 체력 계산식에 쓰이는 웨이브 수치를 강제로 계속 올려버림!
            EndlessTimeCounter = 0; // 카운터 초기화
            
            UE_LOG(LogTemp, Warning, TEXT("발악 페이즈: 적 체력 스케일링 상승! (현재 적용 웨이브: %d)"), CurrentWave);
        }
        
        return; // 발악 페이즈일 때는 아래의 일반 웨이브 로직을 타지 않고 바로 종료
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

void AVoidTriggerGameMode::AddKillCount()
{
    CurrentRunKills++;
}

void AVoidTriggerGameMode::GameOver() 
{
    UMySaveGame* SaveGameInstance = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("SaveSlot"), 0));
    
    if (!SaveGameInstance) 
    {
        // 세이브가 없으면 새로 생성
        SaveGameInstance = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass()));
    }

    // 2. 최고 기록 갱신 여부 체크
    bool bIsNewRecord = false;

    // 최고 생존 시간 갱신 (ElapsedGameTime은 초 단위로 계속 오르고 있던 변수)
    if (ElapsedGameTime > SaveGameInstance->BestSurvivalTime)
    {
        SaveGameInstance->BestSurvivalTime = ElapsedGameTime;
        bIsNewRecord = true;
    }

    // 최고 처치 수 갱신
    if (CurrentRunKills > SaveGameInstance->BestRunKills)
    {
        SaveGameInstance->BestRunKills = CurrentRunKills;
        bIsNewRecord = true;
    }

    // 3. 누적 킬 수도 올려줍니다 (무기 해금용)
    SaveGameInstance->TotalMonsterKills += CurrentRunKills;

    // 4. 세이브 데이터 저장
    UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("SaveSlot"), 0);

    // 로그 출력 (디버그용)
    UE_LOG(LogTemp, Warning, TEXT("게임 오버! 생존: %d초, 킬: %d"), ElapsedGameTime, CurrentRunKills);
    if (bIsNewRecord)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎉 신기록 달성! 🎉"));
    }
}