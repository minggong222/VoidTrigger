// Fill out your copyright notice in the Description page of Project Settings.


#include "VoidTriggerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "VoidTriggerItem.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "MySaveGame.h"
#include "VoidTriggerGold.h"
#include "VoidTriggerGameInstance.h"

// Sets default values
AVoidTriggerCharacter::AVoidTriggerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

// 1. 카메라 생성 및 부착
    FPSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPSCamera"));
    FPSCamera->SetupAttachment(GetCapsuleComponent()); // 캐릭터의 몸통(캡슐)에 부착
    FPSCamera->SetRelativeLocation(FVector(0.f, 0.f, 60.f)); // 대략적인 사람 눈높이로 올림
    FPSCamera->bUsePawnControlRotation = true; // 마우스를 움직일 때 카메라가 따라 회전하도록 설정

	FPSMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPSMesh"));
	FPSMesh->SetupAttachment(FPSCamera); // 카메라에 딱 붙임!
	FPSMesh->bCastDynamicShadow = false; // 1인칭 팔의 그림자가 시야를 가리지 않게 끔
	FPSMesh->CastShadow = false;
	
    // 2. 가상의 총기 생성 및 부착
    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    GunMesh->SetupAttachment(FPSCamera); // 총을 카메라(눈)에 부착하여 시야를 항상 따라다니게 함
    GunMesh->SetRelativeLocation(FVector(40.f, 20.f, -20.f)); // 카메라 기준 앞쪽, 오른쪽, 아래쪽으로 배치 (오른손에 든 느낌)
	
	// 자석 컴포넌트 생성 및 캡슐(루트)에 부착
	MagnetCollision = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetCollision"));
	MagnetCollision->SetupAttachment(RootComponent);

	// 자석 범위 설정 (예: 300 = 반경 3미터)
	MagnetCollision->InitSphereRadius(300.f);

	// 겹침(Overlap)만 감지하도록 충돌 설정
	MagnetCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

// Called when the game starts or when spawned
void AVoidTriggerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	UVoidTriggerGameInstance* GI = Cast<UVoidTriggerGameInstance>(GetGameInstance());
	if (GI)
	{
		MouseSensitivity = GI->MouseSensitivity;
		UE_LOG(LogTemp, Warning, TEXT("불러온 감도: %f"), MouseSensitivity);
	}
	
	// 향상된 입력 시스템 연결
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		PlayerController->bShowMouseCursor = false;
       
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
	
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	
	UMySaveGame* SaveData = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Save01"), 0));

	if (SaveData)
	{
		// =======================================================
		// [공격 트리 Tier 1] 화력 강화: 데미지 5%씩 증가
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Damage")))
		{
			// 현재 레벨 * 0.05(5%) 만큼의 비율을 구합니다.
			float DamageMultiplier = (*LvPtr) * 0.05f; 
            
			// 기존 데미지에 퍼센트 증가분을 더해줍니다.
			Damage = Damage + (Damage * DamageMultiplier);
            
			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 화력 강화 Lv.%d 적용 완료! 현재 데미지: %f"), *LvPtr, Damage);
		}
		
		// =======================================================
		// [공격 트리 Tier 2-1] 전술 장전: 재장전 속도 감소
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Reload")))
		{
			// 예: 1레벨당 재장전 시간 0.2초씩 단축
			float ReduceAmount = (*LvPtr) * 0.2f;
            
			// FMath::Max를 써서 아무리 줄어도 최소 0.3초 밑으로는 안 내려가게 방어합니다.
			ReloadTime = FMath::Max(0.3f, ReloadTime - ReduceAmount);
            
			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 전술 장전 Lv.%d 적용! 재장전 시간: %f초"), *LvPtr, ReloadTime);
		}

		// =======================================================
		// [공격 트리 Tier 2-2] 급소 조준: 치명타 확률 및 배율 증가
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Crit")))
		{
			// 예: 1레벨당 치명타 확률 5%(0.05), 배율 0.2(20%)씩 증가
			CritChance += (*LvPtr) * 0.05f; 
			CritMultiplier += (*LvPtr) * 0.2f; 
            
			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 급소 조준 Lv.%d 적용! 치명확률: %f / 배율: %f"), *LvPtr, CritChance, CritMultiplier);
		}
		
		// =======================================================
		// [공격 트리 Tier 3] 탄창 확장: 최대 탄약 및 장탄수 증가
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Mag")))
		{
			// 예: 1레벨당 탄창이 5발씩 늘어납니다.
			int32 ExtraAmmo = (*LvPtr) * 5;
            
			MaxAmmo += ExtraAmmo;
			CurrentAmmo = MaxAmmo; // 시작할 때 꽉 찬 상태로 시작!
            
			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 탄창 확장 Lv.%d 적용! 최대 탄창: %d발"), *LvPtr, MaxAmmo);
		}
		
		// =======================================================
		// [공격 트리 Tier 4] 제압 사격: 적 슬로우 효과
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Suppress")))
		{
			SuppressionLevel = *LvPtr;
			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 제압 사격 Lv.%d 활성화!"), SuppressionLevel);
		}
		
		// =======================================================
		// [생존 트리 Tier 1] 체력 단련: 최대 체력 10%씩 증가
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Health")))
		{
			// 레벨당 10%(0.1)의 증가 비율 계산
			float HealthMultiplier = (*LvPtr) * 0.1f;
            
			// 기존 MaxHP에 퍼센트만큼 더해줍니다.
			MaxHP = MaxHP + (MaxHP * HealthMultiplier);
            
			// ★ 중요: 최대 체력이 늘어났으니, 현재 체력도 늘어난 수치로 가득 채워줍니다.
			CurrentHP = MaxHP;

			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 체력 단련 Lv.%d 적용 완료! 최대 체력: %f"), *LvPtr, MaxHP);
		}
		
		// =======================================================
		// [생존 트리 Tier 2] 전술 기동: 이동 속도 증가 및 대시 쿨타임 감소
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Speed")))
		{
			// 1. 이동 속도 증가: 레벨당 50.0f씩 영구 증가
			float SpeedBonus = (*LvPtr) * 50.0f;
			MoveSpeed += SpeedBonus;
            
			// 실제 캐릭터 무브먼트 컴포넌트에 반영 (중요!)
			if (GetCharacterMovement())
			{
				GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
			}

			// 2. 대시 쿨타임 감소: 레벨당 0.2초씩 감소 (기본 대시 쿨타임이 2~3초일 때)
			// DashCooldown 변수가 캐릭터 헤더에 선언되어 있어야 합니다.
			float CooldownReduction = (*LvPtr) * 0.2f;
            
			// 최소 쿨타임을 0.5초로 제한하여 무한 대시를 방지합니다.
			DashCooldown = FMath::Max(0.5f, DashCooldown - CooldownReduction);

			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 전술 기동 Lv.%d! 이동속도: %f / 대시쿨타임: %f초"), *LvPtr, MoveSpeed, DashCooldown);
		}
		
		// =======================================================
		// [생존 트리 Tier 2] 방호복 강화: 받는 피해 감소
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Armor")))
		{
			// 레벨당 5%(0.05)씩 피해 감소
			DamageReductionRate = (*LvPtr) * 0.05f;
            
			// 최대 감소치는 50%(0.5)로 제한 (무적이 되는 것 방지)
			DamageReductionRate = FMath::Min(0.5f, DamageReductionRate);

			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 방호복 강화 Lv.%d! 피해 감소율: %f%%"), *LvPtr, DamageReductionRate * 100.f);
		}
		
		// =======================================================
		// [생존 트리 Tier 3] 전장 응급처치: 레벨업 시 체력 회복
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Heal")))
		{
			// 레벨당 10%(0.1)씩 회복량 증가
			LevelUpHealRate = (*LvPtr) * 0.1f;

			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 전장 응급처치 Lv.%d! 레벨업 시 회복률: %f%%"), *LvPtr, LevelUpHealRate * 100.f);
		}
		
		// =======================================================
        // [생존 트리 Tier 4] 불굴의 의지: 조건부 부활 및 무적
        // =======================================================
        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Revive")))
        {
            bHasReviveTrait = true;
                  
            // 레벨당 쿨타임 20초씩 감소 (기본 120초 -> 1레벨 100초, 2레벨 80초...)
            CurrentReviveCooldown = FMath::Max(30.0f, 120.0f - ((*LvPtr) * 20.0f));

            UE_LOG(LogTemp, Warning, TEXT("[특성 적용] ★불굴의 의지★ 활성화! 쿨타임: %f초"), CurrentReviveCooldown);
        }

        // =======================================================
        // [유틸리티 트리 Tier 1] 자원 채굴: 드랍률 증가
        // =======================================================
        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Drop")))
        {
            // 레벨당 15%(0.15)씩 드랍률 보너스 적용
            ResourceDropBonus = (*LvPtr) * 0.15f;
                  
            UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 자원 채굴 Lv.%d! 드랍 보너스: %f%%"), *LvPtr, ResourceDropBonus * 100.f);
        }

        // =======================================================
        // [유틸리티 트리 Tier 2] 보급품 탐색: 특수 무기 등장 확률 증가
        // =======================================================
        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Rarity")))
        {
            // 레벨당 특수 아이템 드랍률 1% (0.01) 증가
            SpecialDropBonus = (*LvPtr) * 0.01f;
            UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 보급품 탐색 Lv.%d (특수드랍 +%f%%)"), *LvPtr, SpecialDropBonus * 100.f);
        }

        // =======================================================
        // [유틸리티 트리 Tier 3] 상인 로비: 상점 가격 할인
        // =======================================================
        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Discount")))
        {
            // 레벨당 5% 할인
            DiscountRate = (*LvPtr) * 0.05f;
            UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 상인 로비 Lv.%d (상점 가격 %f%% 할인)"), *LvPtr, DiscountRate * 100.f);
        }

        // =======================================================
        // [유틸리티 트리 Tier 4] 전략적 선택: 선택지 증가 및 리롤
        // =======================================================
        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Reroll")))
        {
            // 3레벨 이상이면 선택지 2개 추가, 아니면 1개 추가
            ExtraChoiceCount = (*LvPtr >= 3) ? 2 : 1; 
            
            // 레벨당 리롤 횟수 1회 제공
            MaxRerollCount = *LvPtr; 
            CurrentRerollCount = MaxRerollCount;

            UE_LOG(LogTemp, Error, TEXT("[특성 적용] 👑 전략적 선택 발동! 선택지+%d, 리롤 %d회"), ExtraChoiceCount, MaxRerollCount);
        }
		// =======================================================
		// [유틸리티 트리 Tier 2] 시작 보급: 매 회차 시작 시 레벨 증가
		// =======================================================
		if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_StartCash")))
		{
			// 레벨당 시작 레벨 1씩 추가
			for(int i = 0; i < *LvPtr; ++i)
			{
				GainExp(MaxExp); // 경험치를 꽉 채워서 강제 레벨업
			}
			UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 시작 보급 Lv.%d 적용 완료!"), *LvPtr);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("세이브 파일이 없습니다. 기본 스탯으로 시작합니다."));
	}
	
	if (HUDClass)
	{
		UUserWidget* HUD = CreateWidget<UUserWidget>(GetWorld(), HUDClass);
		if (HUD)
		{
			HUD->AddToViewport();
		}
	}
    
	if (MagnetCollision)
	{
		MagnetCollision->OnComponentBeginOverlap.AddDynamic(this, &AVoidTriggerCharacter::OnMagnetOverlap);
		MagnetCollision->SetSphereRadius(200.f); 
	}
	
}

// Called every frame
void AVoidTriggerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AVoidTriggerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 걷기 연결
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVoidTriggerCharacter::Move);
		// 시야 회전 연결
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AVoidTriggerCharacter::Look);
		// --- 사격 연결 (마우스를 누르는 순간 'Started'에 반응) ---
		//	EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::Fire);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::StartFire);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AVoidTriggerCharacter::StopFire);
		
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::Dash);
		if (PauseAction)
		{
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::TogglePause);
		}
		if (JumpAction)
		{
			// 스페이스바를 누른 순간 (Started) -> ACharacter의 내장 함수인 Jump 실행
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            
			// 스페이스바에서 손을 뗀 순간 (Completed) -> ACharacter의 내장 함수인 StopJumping 실행
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
	}
}

void AVoidTriggerCharacter::Move(const FInputActionValue& Value)
{
	// 입력된 WASD 값을 2D 벡터로 가져옴
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 앞뒤(W,S) 이동
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		// 좌우(A,D) 이동
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AVoidTriggerCharacter::Look(const FInputActionValue& Value)
{
	// 입력된 마우스 값을 2D 벡터로 가져옴
	FVector2D LookAxisVector = Value.Get<FVector2D>();
    
    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X * MouseSensitivity);   // ★ 감도 곱하기!
        AddControllerPitchInput(LookAxisVector.Y * MouseSensitivity); // ★ 감도 곱하기!
    }
}

void AVoidTriggerCharacter::Fire()
{
    if (bIsReloading || CurrentAmmo <= 0) 
    {
       if (!bIsReloading && CurrentAmmo <= 0) 
       {
          StartReload();
       }
        
       // ★ 총알이 없거나 장전 중이면 연사 타이머도 아예 꺼버립니다! (헛방 도는 것 방지)
       GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
       return;
    }

    CurrentAmmo--;
    
    // --- [나이아가라 총구 이펙트 스폰 시작] ---
    if (MuzzleFlashEffect != nullptr && FPSMesh != nullptr)
    {
       UNiagaraFunctionLibrary::SpawnSystemAttached(
          MuzzleFlashEffect,              
          FPSMesh,                        
          FName("Muzzle"),                
          FVector::ZeroVector,            
          FRotator::ZeroRotator,          
          EAttachLocation::SnapToTarget,  
          true                            
       );
    }
    
    if (FireSound)
    {
       // 내 캐릭터의 현재 위치에서 사운드를 재생합니다.
       UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
    }
    // --- [나이아가라 이펙트 스폰 끝] ---

    if (FireMontage)
    {
       if (UAnimInstance* AnimInstance = FPSMesh->GetAnimInstance())
       {
          AnimInstance->Montage_Play(FireMontage, 1.0f);
       }
    }
    
    if (FPSCamera == nullptr) return;

    FVector StartLocation = FPSCamera->GetComponentLocation();
    FVector BaseForward = FPSCamera->GetForwardVector(); // 기본 조준 방향

    // --- [산탄] ProjectileCount 만큼 반복해서 총알을 쏩니다 ---
    for (int32 ShotIndex = 0; ShotIndex < ProjectileCount; ++ShotIndex)
    {
        FVector ShotDirection = BaseForward;
        if (ProjectileCount > 1)
        {
            ShotDirection = FMath::VRandCone(BaseForward, FMath::DegreesToRadians(5.f)); 
        }

        FVector EndLocation = StartLocation + (ShotDirection * AttackRange);

        FCollisionQueryParams CollisionParams;
        CollisionParams.AddIgnoredActor(this); // 나 자신 무시

        int32 CurrentHits = 0;     // 현재 이 총알이 관통한 적의 수
        bool bHasExploded = false; // ★ 이 총알 궤적에서 폭발이 일어났는지 확인하는 스위치

        // --- [관통] 맞춘 적을 무시하고 뒤로 계속 뚫고 지나가는 While 루프 ---
        while (CurrentHits <= PierceCount)
        {
            FHitResult HitResult;
            
            // Multi가 아닌 Single 트레이스 사용!
            bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);

            if (bHit)
            {
                AActor* HitActor = HitResult.GetActor();
                if (HitActor)
                {
                    // 1. 기본 데미지 및 크리티컬 계산
                    float FinalDamage = Damage; 
                    bool bIsCritical = false;

                    if (FMath::FRand() <= CritChance)
                    {
                       FinalDamage = Damage * CritMultiplier;
                       bIsCritical = true;
                    }

                    UGameplayStatics::ApplyDamage(HitActor, FinalDamage, GetController(), this, UDamageType::StaticClass());

                    if (bIsCritical)
                    {
                       UE_LOG(LogTemp, Warning, TEXT("💥 크리티컬 적중!! 최종 데미지: %f"), FinalDamage);
                    }
                    else
                    {
                       UE_LOG(LogTemp, Log, TEXT("일반 적중. 데미지: %f"), FinalDamage);
                    }

                    // ========================================================
                    // --- [🩸 몬스터 피격 & 관통 처리 & 제압 사격] ---
                    if (HitActor->ActorHasTag(FName("Monster")))
                    {
                        if (HitImpactEffect)
                        {
                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                                GetWorld(),
                                HitImpactEffect,
                                HitResult.ImpactPoint,             // 맞은 정확한 위치
                                HitResult.ImpactNormal.Rotation()  // 튀어나오는 방향
                            );
                        }
                        
                        // 제압 사격: 적 이동 속도 감소
                        if (SuppressionLevel > 0)
                        {
                            ACharacter* Enemy = Cast<ACharacter>(HitActor);
                            if (Enemy && Enemy->GetCharacterMovement())
                            {
                                if (!Enemy->ActorHasTag(FName("Slowed")))
                                {
                                    float OriginalSpeed = Enemy->GetCharacterMovement()->MaxWalkSpeed;
                                    float SlowMultiplier = 1.0f - (SuppressionLevel * SuppressionSlowPower);
                                    float NewSpeed = OriginalSpeed * FMath::Max(0.2f, SlowMultiplier);

                                    Enemy->GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
                                    Enemy->Tags.Add(FName("Slowed"));

                                    UE_LOG(LogTemp, Log, TEXT("🎯 제압 사격 발동! 속도: %f -> %f"), OriginalSpeed, NewSpeed);

                                    FTimerHandle SlowTimerHandle;
                                    FTimerDelegate SlowDelegate;
                                    TWeakObjectPtr<ACharacter> WeakEnemy = Enemy; 

                                    SlowDelegate.BindLambda([WeakEnemy, OriginalSpeed]()
                                    {
                                        if (WeakEnemy.IsValid() && WeakEnemy->GetCharacterMovement())
                                        {
                                            WeakEnemy->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeed;
                                            WeakEnemy->Tags.Remove(FName("Slowed")); 
                                            UE_LOG(LogTemp, Log, TEXT("✅ 제압 사격 효과 종료. 원래 속도로 복구!"));
                                        }
                                    });

                                    GetWorld()->GetTimerManager().SetTimer(SlowTimerHandle, SlowDelegate, 1.0f, false);
                                }
                            }
                        }

                        // ★ [관통 핵심] 몬스터를 맞췄으므로 횟수를 올리고, 무시 목록에 넣어 다음 트레이스 때는 통과하게 만듭니다.
                        CurrentHits++; 
                        CollisionParams.AddIgnoredActor(HitActor); 
                    }
                    else
                    {
                        // 몬스터가 아닌 벽/바닥을 맞췄다면 더 이상 관통 불가 -> 루프 탈출
                        break;
                    }
                    // ========================================================

                    // ========================================================
                    // --- [💥 폭발탄 (단 한 번만 터짐)] ---
                    if (ExplosionRadius > 0.f && !bHasExploded)
                    {
                        bHasExploded = true; // 터짐 스위치 ON (다음 관통된 적에선 안 터짐)

                        if (ExplosionEffect)
                        {
                           UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                               GetWorld(), ExplosionEffect, HitResult.ImpactPoint, FRotator::ZeroRotator
                           );
                        }
                        
                        if (ExplosionSound)
                        {
                           UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, HitResult.ImpactPoint);
                        }
                        
                        TArray<AActor*> IgnoredActors; 
                        IgnoredActors.Add(this);       // 나 자신은 폭발 무시

                        UGameplayStatics::ApplyRadialDamage(
                           GetWorld(), 
                           ExplosionDamage,                   
                           HitResult.ImpactPoint,  
                           ExplosionRadius,                   
                           UDamageType::StaticClass(), 
                           IgnoredActors, 
                           this, 
                           GetController()
                        );
                    }
                    // ========================================================
                }
            }
            else
            {
                // 아무것도 맞지 않았다면 루프 탈출
                break;
            }
        } // 관통 While 루프 끝
    } // 산탄 For 루프 끝

    if (CurrentAmmo <= 0) StartReload();
}

// ==========================================
// [추가] 마우스 왼쪽 버튼을 눌렀을 때
// ==========================================
void AVoidTriggerCharacter::StartFire()
{
	// 1. 누르자마자 딜레이 없이 일단 한 발 발사!
	Fire();

	// 2. 타이머를 켜서 FireRate(예: 0.1초) 간격으로 Fire() 함수를 '반복(true)' 실행!
	GetWorldTimerManager().SetTimer(AutoFireTimerHandle, this, &AVoidTriggerCharacter::Fire, FireRate, true);
}

// ==========================================
// [추가] 마우스 왼쪽 버튼에서 손을 뗐을 때
// ==========================================
void AVoidTriggerCharacter::StopFire()
{
	// 타이머를 강제로 지워서 연사를 멈춥니다.
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
}

void AVoidTriggerCharacter::OnMagnetOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 나 자신(플레이어)과 닿은 것은 무시합니다.
	if (OtherActor && OtherActor != this)
	{
		// 닿은 물체가 '경험치 구슬(VoidTriggerItem)'인지 확인합니다.
		if (AVoidTriggerItem* Item = Cast<AVoidTriggerItem>(OtherActor))
		{
			Item->StartFlying();
		}
        
		// 새로 만든 골드 아이템 체크 (이걸 추가해야 골드도 날아옵니다!)
		if (AVoidTriggerGold* Gold = Cast<AVoidTriggerGold>(OtherActor))
		{
			Gold->StartFlying();
		}
	}
}

void AVoidTriggerCharacter::GainExp(float Amount)
{
	CurrentExp += Amount;

	// 로그로 현재 상황 확인
	UE_LOG(LogTemp, Log, TEXT("경험치 획득: %f (현재: %f / 목표: %f)"), Amount, CurrentExp, MaxExp);

	// 경험치가 꽉 찼다면 레벨업!
	if (CurrentExp >= MaxExp)
	{
		LevelUp();
	}
}

void AVoidTriggerCharacter::LevelUp()
{
	
	if (LevelUpHealRate > 0.0f)
	{
		float HealAmount = MaxHP * LevelUpHealRate;
		CurrentHP = FMath::Min(MaxHP, CurrentHP + HealAmount);
        
		UE_LOG(LogTemp, Warning, TEXT("💊 전장 응급처치 발동! %f 체력 회복 (현재: %f)"), HealAmount, CurrentHP);
	}
	
	// 남은 경험치를 다음 레벨로 이월
	CurrentExp -= MaxExp;
	Level++;

	// 다음 레벨에 필요한 경험치 증가 (예: 이전 단계보다 20%씩 증가)
	MaxExp *= 1.2f;

	UE_LOG(LogTemp, Error, TEXT("★ 레벨업! 현재 레벨: %d ★"), Level);

	// 1. 대기열에 추가
	PendingLevelUps++;

	// 2. 만약 현재 다른 레벨업 UI가 떠 있지 않다면 UI를 띄움
	// (이미 떠 있다면, 나중에 CloseLevelUpUI가 알아서 다음 창을 띄울 것임)
	if (PendingLevelUps == 1)
	{
		ShowLevelUpUI();
	}
}

void AVoidTriggerCharacter::ShowLevelUpUI()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !LevelUpWidgetClass) return;

	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
	
	PC->FlushPressedKeys();
	
	UGameplayStatics::SetGamePaused(GetWorld(), true);

	if (!LevelUpWidgetInstance)
	{
		LevelUpWidgetInstance = CreateWidget<UUserWidget>(PC, LevelUpWidgetClass);
	}

	if (LevelUpWidgetInstance && !LevelUpWidgetInstance->IsInViewport())
	{
		LevelUpWidgetInstance->AddToViewport(100);
	}

	// 마우스 및 입력 세팅
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(LevelUpWidgetInstance->TakeWidget());
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;
}

void AVoidTriggerCharacter::StartReload()
{
	if (bIsReloading) return;

	bIsReloading = true;
	UE_LOG(LogTemp, Warning, TEXT("재장전 시작..."));

	if (ReloadMontage)
	{
		if (UAnimInstance* AnimInstance = FPSMesh->GetAnimInstance())
		{
			// ★ 1. 몽타주의 원래 재생 길이(초)를 가져옵니다.
			float MontageLength = ReloadMontage->GetPlayLength();

			// ★ 2. 애니메이션 배속을 계산합니다.
			// (예: 몽타주가 2초고 ReloadTime이 1초면, PlayRate는 2.0배속이 됩니다)
			float PlayRate = MontageLength / ReloadTime;

			// ★ 3. 기존의 1.0f 대신, 계산된 PlayRate를 넣어 재생합니다.
			AnimInstance->Montage_Play(ReloadMontage, PlayRate);
          
			UE_LOG(LogTemp, Log, TEXT("재장전 애니메이션 배속: %f"), PlayRate);
		}
	}
	// ReloadTime(2초) 후에 FinishReload 함수를 호출합니다.
	FTimerHandle ReloadTimerHandle;
	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AVoidTriggerCharacter::FinishReload, ReloadTime, false);
}

void AVoidTriggerCharacter::FinishReload()
{
	bIsReloading = false;
	CurrentAmmo = MaxAmmo; // 탄알 충전!
	UE_LOG(LogTemp, Log, TEXT("재장전 완료!"));
}

float AVoidTriggerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsInvincible)
	{
		UE_LOG(LogTemp, Warning, TEXT("대시 회피 성공! 데미지 무시"));
		return 0.0f;
	}
	float ReducedDamage = DamageAmount * (1.0f - DamageReductionRate);
	
	// ★ 치명적 피해 감지 로직 ★
	if (CurrentHP - ReducedDamage <= 0.0f && bHasReviveTrait && bCanRevive)
	{
		CurrentHP = 1.0f;   // 체력 1로 고정
		bCanRevive = false; // 쿨타임 시작
		bIsInvincible = true; // 무적 시작

		UE_LOG(LogTemp, Error, TEXT("🔥 불굴의 의지 발동! 1초의 무적 시간을 얻습니다."));

		// 1. 무적 해제 타이머 (ReviveInvincibleDuration초 후 실행)
		GetWorldTimerManager().SetTimer(InvincibleTimer, this, &AVoidTriggerCharacter::EndInvincibility, ReviveInvincibleDuration, false);

		// 2. 부활 쿨타임 리셋 타이머
		FTimerHandle ReviveTimerHandle;
		GetWorldTimerManager().SetTimer(ReviveTimerHandle, [this]() {
			bCanRevive = true;
			UE_LOG(LogTemp, Warning, TEXT("🛡️ 불굴의 의지 재사용 가능!"));
		}, CurrentReviveCooldown, false);

		return 0.0f; // 이번 데미지는 무효화
	}
	
	float ActualDamage = Super::TakeDamage(ReducedDamage, DamageEvent, EventInstigator, DamageCauser);

	CurrentHP -= ActualDamage;
	UE_LOG(LogTemp, Warning, TEXT("플레이어 피격! 원본데미지: %f -> 적용데미지: %f / 남은체력: %f"), DamageAmount, ActualDamage, CurrentHP);

	// 체력이 0이 되면 죽음 처리
	if (CurrentHP <= 0.f)
	{
		Die();
	}

	return ActualDamage;
}

void AVoidTriggerCharacter::Die()
{
	UE_LOG(LogTemp, Error, TEXT("게임 오버!"));
    
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 1. 입력 끄기 (더 이상 움직이거나 총 쏘지 못하게)
	DisableInput(PC);
    
	// 2. 시간 멈추기 (배경 몹들도 다 멈춤)
	UGameplayStatics::SetGamePaused(GetWorld(), true);
    
	// 3. 게임 오버 UI 띄우기
	if (GameOverWidgetClass)
	{
		UUserWidget* GameOverWidget = CreateWidget<UUserWidget>(PC, GameOverWidgetClass);
		if (GameOverWidget)
		{
			GameOverWidget->AddToViewport(100); // 제일 맨 앞에 보이게 ZOrder 100
		}
	}

	// 4. 마우스 커서 켜고 UI 조작 모드로 변경
	PC->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	PC->SetInputMode(InputMode);
}

void AVoidTriggerCharacter::ApplyLevelUpUpgrade(ELevelUpUpgradeType UpgradeType)
{
	switch (UpgradeType)
	{
	case ELevelUpUpgradeType::ReloadSpeed:
		ReloadTime = FMath::Max(0.2f, ReloadTime - 0.2f);
		break;

	case ELevelUpUpgradeType::MagazineSize:
		MaxAmmo += 5;
		CurrentAmmo += 5; // 즉시 늘어난 만큼 채워주고 싶으면
		CurrentAmmo = FMath::Clamp(CurrentAmmo, 0, MaxAmmo);
		break;

	case ELevelUpUpgradeType::Damage:
		Damage += 5.f;
		break;

	case ELevelUpUpgradeType::MoveSpeed:
		MoveSpeed += 50.f;
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
		break;

	case ELevelUpUpgradeType::MagnetRange:
		if (MagnetCollision)
		{
			float NewRadius = MagnetCollision->GetScaledSphereRadius() + 75.f;
			MagnetCollision->SetSphereRadius(NewRadius);
		}
		break;

	case ELevelUpUpgradeType::MaxHP:
		MaxHP += 20.f;
		CurrentHP += 20.f;
		CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
		break;
		
	case ELevelUpUpgradeType::PierceShot:
		PierceCount += 1; // 뚫는 횟수 1 증가
		UE_LOG(LogTemp, Warning, TEXT("특수 능력: 관통탄 획득!"));
		break;

	case ELevelUpUpgradeType::ScatterShot:
		ProjectileCount += 1; // 발사체 수 1 증가
		UE_LOG(LogTemp, Warning, TEXT("특수 능력: 산탄 사격 획득!"));
		break;

	case ELevelUpUpgradeType::ExplosiveShot:
		// 처음 먹었을 때: 기본 폭발 능력 부여
		if (ExplosionRadius == 0.f)
		{
			ExplosionRadius = 250.f;
			ExplosionDamage = 15.f;
			UE_LOG(LogTemp, Warning, TEXT("특수 능력: 폭발탄 최초 획득!"));
		}
		// 두 번째 이상 먹었을 때: 폭발 능력 강화
		else
		{
			ExplosionRadius += 50.f; // 범위 증가
			ExplosionDamage += 10.f; // 데미지 증가
			UE_LOG(LogTemp, Warning, TEXT("특수 능력: 폭발탄 강화! (범위: %f, 딜: %f)"), ExplosionRadius, ExplosionDamage);
		}
		break;
	
	default:
		break;
	}

	CloseLevelUpUI();
}

void AVoidTriggerCharacter::CloseLevelUpUI()
{
	// 1. 하나 처리했으니 대기열 감소
	PendingLevelUps--;

	// ★ 2. 핵심 해결책: 기존 UI를 화면에서 내리고 메모리에서 완전히 날려버립니다!
	if (LevelUpWidgetInstance)
	{
		if (LevelUpWidgetInstance->IsInViewport())
		{
			LevelUpWidgetInstance->RemoveFromParent();
		}
		LevelUpWidgetInstance = nullptr; // 변수를 비워버림 -> 다음 번에 강제로 새로 만들게 됨
	}

	// 3. 아직 남은 레벨업이 있다면?
	if (PendingLevelUps > 0)
	{
		// LevelUpWidgetInstance가 nullptr이 되었으므로, 
		// 여기서 완전히 새로운 위젯을 만들면서 카드를 새로(랜덤하게) 뽑게 됩니다!
		ShowLevelUpUI(); 
	}
	else
	{
		// 4. 더 이상 남은 게 없을 때만 게임 재개!
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
			UGameplayStatics::SetGamePaused(GetWorld(), false);
		}
	}
}

TArray<ELevelUpUpgradeType> AVoidTriggerCharacter::GetRandomUpgrades(int32 Count)
{
	TArray<ELevelUpUpgradeType> SelectedUpgrades;
	TArray<ELevelUpUpgradeType> LotteryPool; // 추첨함

	// 1. 일반 업그레이드는 추첨함에 10장씩 넣습니다. (당첨 확률 높음)
	TArray<ELevelUpUpgradeType> NormalUpgrades = {
		ELevelUpUpgradeType::ReloadSpeed, ELevelUpUpgradeType::MagazineSize,
		ELevelUpUpgradeType::Damage, ELevelUpUpgradeType::MoveSpeed,
		ELevelUpUpgradeType::MagnetRange, ELevelUpUpgradeType::MaxHP
	};
	for (ELevelUpUpgradeType Upgrade : NormalUpgrades)
	{
		for (int i = 0; i < 10; i++) LotteryPool.Add(Upgrade);
	}

	// 2. 특수 업그레이드는 추첨함에 딱 1장씩만 넣습니다. (당첨 확률 매우 낮음)
	TArray<ELevelUpUpgradeType> SpecialUpgrades = {
		ELevelUpUpgradeType::PierceShot, 
		ELevelUpUpgradeType::ScatterShot, 
		ELevelUpUpgradeType::ExplosiveShot
	};
	for (ELevelUpUpgradeType Upgrade : SpecialUpgrades)
	{
		LotteryPool.Add(Upgrade);
	}

	// 3. 3개를 뽑습니다. (중복 방지 로직 포함)
	while (SelectedUpgrades.Num() < Count && LotteryPool.Num() > 0)
	{
		// 무작위로 하나 뽑기
		int32 RandomIndex = FMath::RandRange(0, LotteryPool.Num() - 1);
		ELevelUpUpgradeType Picked = LotteryPool[RandomIndex];
        
		SelectedUpgrades.Add(Picked);

		// ★중요: UI에 같은 항목이 2개 뜨지 않게 하려면, 뽑힌 항목은 추첨함에서 전부 빼버립니다.
		LotteryPool.Remove(Picked); 
	}

	return SelectedUpgrades;
}

void AVoidTriggerCharacter::Dash()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("대시 버튼 눌림!!!"));

	if (!bCanDash || GetCharacterMovement()->IsFalling()) return;
	// 쿨타임 중이거나 공중에 떠있을 때는 대시 불가 (필요시 공중 대시 허용 가능)
	if (!bCanDash || GetCharacterMovement()->IsFalling()) return;

	bCanDash = false;
	bIsInvincible = true;

	// 1. 대시 방향 계산 (플레이어가 누르고 있는 WASD 방향)
	FVector DashDirection = GetLastMovementInputVector();
    
	// 만약 가만히 서있는 상태라면 앞쪽(Forward)으로 설정
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetActorForwardVector();
	}
	DashDirection.Normalize(); // 벡터의 길이를 1로 맞춤

	// 2. 캐릭터 발사! (방향 * 힘)
	// 두 번째, 세 번째 매개변수(true)는 기존에 가지고 있던 관성을 무시하고 이 힘으로 덮어씌운다는 뜻입니다.
	LaunchCharacter(DashDirection * DashStrength, true, true);

	// 3. 무적 시간 타이머 시작
	GetWorldTimerManager().SetTimer(InvincibleTimer, this, &AVoidTriggerCharacter::EndInvincibility, InvincibleDuration, false);

	// 4. 쿨타임 타이머 시작
	GetWorldTimerManager().SetTimer(DashCooldownTimer, this, &AVoidTriggerCharacter::ResetDash, DashCooldown, false);
}

void AVoidTriggerCharacter::EndInvincibility()
{
	bIsInvincible = false;
	// 디버그 로그 (나중에 지우셔도 됩니다)
	UE_LOG(LogTemp, Warning, TEXT("무적 시간 종료!"));
}

void AVoidTriggerCharacter::ResetDash()
{
	bCanDash = true;
	UE_LOG(LogTemp, Warning, TEXT("대시 쿨타임 완료! 다시 사용 가능."));
}

float AVoidTriggerCharacter::GetDashCooldownPercent() const
{
	// 1. 대시가 이미 가능하면 게이지 100% 꽉 찬 상태
	if (bCanDash) 
	{
		return 1.0f; 
	}

	// 2. 쿨타임이 도는 중이면 (흐른 시간 / 전체 쿨타임 시간)을 계산해서 반환
	float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(DashCooldownTimer);
    
	// 혹시라도 오류로 쿨타임이 0이 되는 것을 방지
	if (DashCooldown <= 0.0f) return 1.0f; 

	return ElapsedTime / DashCooldown;
}

void AVoidTriggerCharacter::TogglePause(const FInputActionValue& Value)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 현재 게임이 일시정지 상태인지 확인
	bool bIsPaused = UGameplayStatics::IsGamePaused(GetWorld());

	if (!bIsPaused) 
	{
		// ==========================================
		// 🟢 플립플롭 A: 게임 일시정지 & 설정창 열기
		// ==========================================
		GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
		PC->FlushPressedKeys();

		UGameplayStatics::SetGamePaused(GetWorld(), true);

		// 위젯이 아직 생성되지 않았다면 생성
		if (SettingsWidgetClass && !SettingsWidgetInstance)
		{
			SettingsWidgetInstance = CreateWidget<UUserWidget>(PC, SettingsWidgetClass);
		}

		// 화면에 띄우기
		if (SettingsWidgetInstance && !SettingsWidgetInstance->IsInViewport())
		{
			SettingsWidgetInstance->AddToViewport(1); // ZOrder 1로 맨 위에 띄움
		}

		// 마우스 커서 보이게 하고, 입력 모드를 UI+Game으로 변경
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		if (SettingsWidgetInstance)
		{
			InputMode.SetWidgetToFocus(SettingsWidgetInstance->TakeWidget());
		}
		else
		{
			InputMode.SetWidgetToFocus(nullptr);
		}
		PC->SetInputMode(InputMode);
	}
	else 
	{
		// ==========================================
		// 🔴 플립플롭 B: 게임 재개 & 설정창 닫기
		// ==========================================
		UGameplayStatics::SetGamePaused(GetWorld(), false);

		// 화면에서 지우기
		if (SettingsWidgetInstance && SettingsWidgetInstance->IsInViewport())
		{
			SettingsWidgetInstance->RemoveFromParent();
		}

		// 마우스 커서 숨기고, 입력 모드를 게임 전용으로 복구
		PC->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}