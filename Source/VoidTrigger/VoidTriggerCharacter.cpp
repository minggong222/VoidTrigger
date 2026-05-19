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
#include "Engine/OverlapResult.h"

// Sets default values
AVoidTriggerCharacter::AVoidTriggerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 카메라 생성 및 부착
    FPSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPSCamera"));
    FPSCamera->SetupAttachment(GetCapsuleComponent());
    FPSCamera->SetRelativeLocation(FVector(0.f, 0.f, 60.f)); 
    FPSCamera->bUsePawnControlRotation = true; 

    FPSMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPSMesh"));
    FPSMesh->SetupAttachment(FPSCamera); 
    FPSMesh->bCastDynamicShadow = false; 
    FPSMesh->CastShadow = false;
    
    // 2. 가상의 총기 생성 및 부착
    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    GunMesh->SetupAttachment(FPSCamera); 
    GunMesh->SetRelativeLocation(FVector(40.f, 20.f, -20.f)); 
    
    // 자석 컴포넌트 생성 및 캡슐(루트)에 부착
    MagnetCollision = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetCollision"));
    MagnetCollision->SetupAttachment(RootComponent);

    MagnetCollision->InitSphereRadius(300.f);
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
       if (GI->bHasStartingTrait)
       {
          EquipStartingWeapon(GI->StartingWeaponTrait);
       }
    }
    
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
    CurrentStamina = MaxStamina;
    
    UMySaveGame* SaveData = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Save01"), 0));

    if (SaveData)
    {
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Damage")))
       {
          float DamageMultiplier = (*LvPtr) * 0.05f; 
          Damage = Damage + (Damage * DamageMultiplier);
          UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 화력 강화 Lv.%d 적용 완료! 현재 데미지: %f"), *LvPtr, Damage);
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Reload")))
       {
          float ReduceAmount = (*LvPtr) * 0.2f;
          ReloadTime = FMath::Max(0.3f, ReloadTime - ReduceAmount);
          UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 전술 장전 Lv.%d 적용! 재장전 시간: %f초"), *LvPtr, ReloadTime);
       }

       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Crit")))
       {
          CritChance += (*LvPtr) * 0.05f; 
          CritMultiplier += (*LvPtr) * 0.2f; 
          UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 급소 조준 Lv.%d 적용! 치명확률: %f / 배율: %f"), *LvPtr, CritChance, CritMultiplier);
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Mag")))
       {
          int32 ExtraAmmo = (*LvPtr) * 5;
          MaxAmmo += ExtraAmmo;
          CurrentAmmo = MaxAmmo; 
          UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 탄창 확장 Lv.%d 적용! 최대 탄창: %d발"), *LvPtr, MaxAmmo);
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Suppress")))
       {
          SuppressionLevel = *LvPtr;
          UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 제압 사격 Lv.%d 활성화!"), SuppressionLevel);
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Health")))
       {
          float HealthMultiplier = (*LvPtr) * 0.1f;
          MaxHP = MaxHP + (MaxHP * HealthMultiplier);
          CurrentHP = MaxHP;
          UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 체력 단련 Lv.%d 적용 완료! 최대 체력: %f"), *LvPtr, MaxHP);
       }
       
       // =======================================================
       // [생존 트리 Tier 2] 전술 기동: 이동 속도 증가 및 스테미나 증가
       // =======================================================
    	if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Speed")))
    	{
    		// 1. 이동 속도 증가
    		float SpeedBonus = (*LvPtr) * 50.0f;
    		MoveSpeed += SpeedBonus;
    		// [기존 코드 삭제됨] SprintSpeed += SpeedBonus; 
            
    		if (GetCharacterMovement())
    		{
    			GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    		}

    		// 2. 최대 스테미나 증가 (레벨당 30)
    		float StaminaBonus = (*LvPtr) * 30.0f;
    		MaxStamina += StaminaBonus;
    		CurrentStamina = MaxStamina;

    		UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 전술 기동 Lv.%d! 이동속도: %f / 최대 스테미나: %f"), *LvPtr, MoveSpeed, MaxStamina);
    	}
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Armor")))
       {
          DamageReductionRate = (*LvPtr) * 0.05f;
          DamageReductionRate = FMath::Min(0.5f, DamageReductionRate);
          UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 방호복 강화 Lv.%d! 피해 감소율: %f%%"), *LvPtr, DamageReductionRate * 100.f);
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Heal")))
       {
          LevelUpHealRate = (*LvPtr) * 0.1f;
          UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 전장 응급처치 Lv.%d! 레벨업 시 회복률: %f%%"), *LvPtr, LevelUpHealRate * 100.f);
       }
       
        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Revive")))
        {
            bHasReviveTrait = true;
            CurrentReviveCooldown = FMath::Max(30.0f, 120.0f - ((*LvPtr) * 20.0f));
            UE_LOG(LogTemp, Warning, TEXT("[특성 적용] ★불굴의 의지★ 활성화! 쿨타임: %f초"), CurrentReviveCooldown);
        }

        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Drop")))
        {
            ResourceDropBonus = (*LvPtr) * 0.15f;
            UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 자원 채굴 Lv.%d! 드랍 보너스: %f%%"), *LvPtr, ResourceDropBonus * 100.f);
        }

        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Rarity")))
        {
            SpecialDropBonus = (*LvPtr) * 0.01f;
            UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 보급품 탐색 Lv.%d (특수드랍 +%f%%)"), *LvPtr, SpecialDropBonus * 100.f);
        }

        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Discount")))
        {
            DiscountRate = (*LvPtr) * 0.05f;
            UE_LOG(LogTemp, Warning, TEXT("[특성 적용] 상인 로비 Lv.%d (상점 가격 %f%% 할인)"), *LvPtr, DiscountRate * 100.f);
        }

        if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Reroll")))
        {
            ExtraChoiceCount = (*LvPtr >= 3) ? 2 : 1; 
            MaxRerollCount = *LvPtr; 
            CurrentRerollCount = MaxRerollCount;
            UE_LOG(LogTemp, Error, TEXT("[특성 적용] 👑 전략적 선택 발동! 선택지+%d, 리롤 %d회"), ExtraChoiceCount, MaxRerollCount);
        }

       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_StartCash")))
       {
          for(int i = 0; i < *LvPtr; ++i)
          {
             GainExp(MaxExp); 
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

    // ==========================================
    // 스테미나 소모 및 회복 로직
    // ==========================================
    if (bIsSprinting && GetCharacterMovement()->Velocity.SizeSquared2D() > 0.f)
    {
        // 뛰고 있으면서 실제로 이동 중일 때만 스테미나 소모
        CurrentStamina -= StaminaConsumeRate * DeltaTime;

        if (CurrentStamina <= 0.f)
        {
            CurrentStamina = 0.f;
            StopSprint(); // 스테미나 고갈 시 달리기 강제 종료
        }
    }
    else
    {
        // 뛰고 있지 않을 때 스테미나 자동 회복
        if (CurrentStamina < MaxStamina)
        {
            CurrentStamina += StaminaRegenRate * DeltaTime;
            if (CurrentStamina > MaxStamina)
            {
                CurrentStamina = MaxStamina;
            }
        }
    }
}

// Called to bind functionality to input
void AVoidTriggerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
       EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVoidTriggerCharacter::Move);
       EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AVoidTriggerCharacter::Look);
       
       EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::StartFire);
       EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AVoidTriggerCharacter::StopFire);
       
       // 대시 대신 달리기 바인딩 (누를 때 Start, 뗄 때 Stop)
       if (SprintAction)
       {
           EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::StartSprint);
           EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AVoidTriggerCharacter::StopSprint);
       }
       
       EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::StartReload);
       if (PauseAction)
       {
          EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::TogglePause);
       }
       if (JumpAction)
       {
          EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
          EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
       }
    }
}

void AVoidTriggerCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
       AddMovementInput(GetActorForwardVector(), MovementVector.Y);
       AddMovementInput(GetActorRightVector(), MovementVector.X);
    }
}

void AVoidTriggerCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    
    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X * MouseSensitivity); 
        AddControllerPitchInput(LookAxisVector.Y * MouseSensitivity); 
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

        GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
        return;
    }

    CurrentAmmo--;

    FVector CameraLocation;
    FRotator CameraRotation;

    if (GetController())
    {
        GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);
    }
    else if (FPSCamera)
    {
        CameraLocation = FPSCamera->GetComponentLocation();
        CameraRotation = FPSCamera->GetComponentRotation();
    }

    FVector BaseForward = CameraRotation.Vector();

    if (FPSMesh == nullptr) return;

    FVector MuzzleLocation = FPSMesh->GetSocketLocation(TEXT("Muzzle"));

    if (MuzzleLocation.IsNearlyZero())
    {
        MuzzleLocation = CameraLocation + (BaseForward * 50.f);
    }

    AddControllerPitchInput(FMath::RandRange(-0.5f, -0.1f));
    AddControllerYawInput(FMath::RandRange(-0.3f, 0.3f));

    if (MuzzleFlashEffect)
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
        UGameplayStatics::PlaySoundAtLocation(
            this,
            FireSound,
            GetActorLocation()
        );
    }

    if (FireMontage)
    {
        if (UAnimInstance* AnimInstance = FPSMesh->GetAnimInstance())
        {
            AnimInstance->Montage_Play(FireMontage, 1.0f);
        }
    }

    for (int32 ShotIndex = 0; ShotIndex < ProjectileCount; ++ShotIndex)
    {
        FVector ShotDirection = BaseForward;

        if (ProjectileCount > 1)
        {
            if (ShotIndex > 0)
            {
                ShotDirection = FMath::VRandCone(
                    BaseForward,
                    FMath::DegreesToRadians(5.f)
                );
            }
        }

        FVector TraceEnd = CameraLocation + (ShotDirection * AttackRange);
        
        int32 CurrentHits = 0;
        bool bHasExploded = false;

        FVector DynamicStartLocation = CameraLocation;
        FVector VisualStartLocation = MuzzleLocation;

        TArray<AActor*> PiercedActors;
        PiercedActors.Add(this);

        float BulletRadius = 15.f; 
        FCollisionShape SphereShape = FCollisionShape::MakeSphere(BulletRadius);

        while (CurrentHits <= PierceCount)
        {
            FHitResult HitResult;
            FCollisionQueryParams CollisionParams;
            CollisionParams.AddIgnoredActors(PiercedActors);

            bool bHit = GetWorld()->SweepSingleByChannel(
                HitResult,
                DynamicStartLocation,
                TraceEnd,
                FQuat::Identity,
                ECC_Visibility,
                SphereShape,
                CollisionParams
            );

            if (bHit)
            {
                AActor* HitActor = HitResult.GetActor();

                if (HitActor)
                {
                    if (BulletTracerEffect)
                    {
                        FRotator TracerRotation = (HitResult.ImpactPoint - VisualStartLocation).Rotation();

                        UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                            GetWorld(),
                            BulletTracerEffect,
                            VisualStartLocation,
                            TracerRotation
                        );

                        if (TracerComp)
                        {
                            TracerComp->SetVariableVec3(FName("User.BeamEnd"), HitResult.ImpactPoint);
                        }
                    }

                    float FinalDamage = Damage;
                    bool bIsCritical = false;

                    if (FMath::FRand() <= CritChance)
                    {
                        FinalDamage *= CritMultiplier;
                        bIsCritical = true;
                    }

                    UGameplayStatics::ApplyDamage(
                        HitActor,
                        FinalDamage,
                        GetController(),
                        this,
                        UDamageType::StaticClass()
                    );

                    if (HitActor->ActorHasTag(FName("Monster")))
                    {
                        if (HitImpactEffect)
                        {
                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                                GetWorld(),
                                HitImpactEffect,
                                HitResult.ImpactPoint,
                                HitResult.ImpactNormal.Rotation()
                            );
                        }

                        if (bIsChainLightning)
                        {
                            FVector Origin = HitResult.ImpactPoint; 

                            FCollisionShape ChainSphereShape = FCollisionShape::MakeSphere(ChainRadius);
                            TArray<FOverlapResult> OverlapResults;
        
                            FCollisionQueryParams OverlapParams;
                            OverlapParams.AddIgnoredActor(this);     
                            OverlapParams.AddIgnoredActor(HitActor); 

                            bool bFoundChainTarget = GetWorld()->OverlapMultiByChannel(
                                OverlapResults, Origin, FQuat::Identity, ECC_Visibility, ChainSphereShape, OverlapParams
                            );

                            if (bFoundChainTarget)
                            {
                                for (const FOverlapResult& Result : OverlapResults)
                                {
                                    AActor* ChainTarget = Result.GetActor();
                                    if (ChainTarget && ChainTarget->ActorHasTag(FName("Monster")))
                                    {
                                        UGameplayStatics::ApplyDamage(ChainTarget, ChainDamage, GetController(), this, UDamageType::StaticClass());
                    
                                        if (HitImpactEffect)
                                        {
                                            FVector EffectLocation = ChainTarget->GetActorLocation();
                                            FRotator EffectRotation = (Origin - EffectLocation).Rotation();

                                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                                                GetWorld(), 
                                                HitImpactEffect, 
                                                EffectLocation, 
                                                EffectRotation
                                            );
                                        }

                                        if (BulletTracerEffect)
                                        {
                                            UNiagaraComponent* BounceTracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                                                GetWorld(), BulletTracerEffect, Origin
                                            );

                                            if (BounceTracerComp)
                                            {
                                                BounceTracerComp->SetVariableVec3(FName("User.BeamEnd"), ChainTarget->GetActorLocation());
                                            }
                                        }
                                        break; 
                                    }
                                }
                            }
                        }

                        if (bHasBlackHoleMod && FMath::FRand() <= 1.00f)
                        {
                           if (BlackHoleClass) 
                           {
                              GetWorld()->SpawnActor<AActor>(BlackHoleClass, HitResult.ImpactPoint, FRotator::ZeroRotator);
                           }
                        }
                        
                        DynamicStartLocation = HitResult.ImpactPoint + (ShotDirection * 5.f);
                        VisualStartLocation = HitResult.ImpactPoint;
                        
                        CurrentHits++;
                        PiercedActors.Add(HitActor); 
                    }
                    else
                    {
                        break;
                    }

                    if (ExplosionRadius > 0.f && !bHasExploded)
                    {
                        bHasExploded = true;

                        if (ExplosionEffect)
                        {
                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                                GetWorld(),
                                ExplosionEffect,
                                HitResult.ImpactPoint,
                                FRotator::ZeroRotator
                            );
                        }

                        if (ExplosionSound)
                        {
                            UGameplayStatics::PlaySoundAtLocation(
                                this,
                                ExplosionSound,
                                HitResult.ImpactPoint
                            );
                        }

                        TArray<AActor*> IgnoredActors;
                        IgnoredActors.Add(this);

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
                }
            }
            else
            {
                if (BulletTracerEffect)
                {
                    FRotator MissRotation = (TraceEnd - VisualStartLocation).Rotation();

                    UNiagaraComponent* MissTracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                        GetWorld(),
                        BulletTracerEffect,
                        VisualStartLocation,
                        MissRotation
                    );

                    if (MissTracerComp)
                    {
                        MissTracerComp->SetVariableVec3(FName("User.BeamEnd"), TraceEnd);
                    }
                }

                break; 
            }
        }
    }

    if (CurrentAmmo <= 0)
    {
        StartReload();
    }
}

void AVoidTriggerCharacter::StartFire()
{
    Fire();
    GetWorldTimerManager().SetTimer(AutoFireTimerHandle, this, &AVoidTriggerCharacter::Fire, FireRate, true);
}

void AVoidTriggerCharacter::StopFire()
{
    GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
}

void AVoidTriggerCharacter::OnMagnetOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
       if (AVoidTriggerItem* Item = Cast<AVoidTriggerItem>(OtherActor))
       {
          Item->StartFlying();
       }
        
       if (AVoidTriggerGold* Gold = Cast<AVoidTriggerGold>(OtherActor))
       {
          Gold->StartFlying();
       }
    }
}

void AVoidTriggerCharacter::GainExp(float Amount)
{
    CurrentExp += Amount;
    UE_LOG(LogTemp, Log, TEXT("경험치 획득: %f (현재: %f / 목표: %f)"), Amount, CurrentExp, MaxExp);

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
    
    CurrentExp -= MaxExp;
    Level++;
    MaxExp *= 1.2f;

    UE_LOG(LogTemp, Error, TEXT("★ 레벨업! 현재 레벨: %d ★"), Level);

    PendingLevelUps++;

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

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(LevelUpWidgetInstance->TakeWidget());
    PC->SetInputMode(InputMode);
    PC->bShowMouseCursor = true;
}

void AVoidTriggerCharacter::StartReload()
{
    if (bIsReloading || CurrentAmmo == MaxAmmo) return;

    bIsReloading = true;
    UE_LOG(LogTemp, Warning, TEXT("재장전 시작..."));

    if (ReloadMontage)
    {
       if (UAnimInstance* AnimInstance = FPSMesh->GetAnimInstance())
       {
          float MontageLength = ReloadMontage->GetPlayLength();
          float PlayRate = MontageLength / ReloadTime;
          AnimInstance->Montage_Play(ReloadMontage, PlayRate);
          UE_LOG(LogTemp, Log, TEXT("재장전 애니메이션 배속: %f"), PlayRate);
       }
    }
    FTimerHandle ReloadTimerHandle;
    GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AVoidTriggerCharacter::FinishReload, ReloadTime, false);
}

void AVoidTriggerCharacter::FinishReload()
{
    bIsReloading = false;
    CurrentAmmo = MaxAmmo; 
    UE_LOG(LogTemp, Log, TEXT("재장전 완료!"));
}

float AVoidTriggerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsInvincible)
    {
       UE_LOG(LogTemp, Warning, TEXT("무적 상태! 데미지 무시"));
       return 0.0f;
    }
    
    float ReducedDamage = DamageAmount * (1.0f - DamageReductionRate);
    
    if (CurrentHP - ReducedDamage <= 0.0f && bHasReviveTrait && bCanRevive)
    {
       CurrentHP = 1.0f;   
       bCanRevive = false; 
       bIsInvincible = true; 

       UE_LOG(LogTemp, Error, TEXT("🔥 불굴의 의지 발동! 무적 시간을 얻습니다."));

       GetWorldTimerManager().SetTimer(InvincibleTimer, this, &AVoidTriggerCharacter::EndInvincibility, ReviveInvincibleDuration, false);

       FTimerHandle ReviveTimerHandle;
       GetWorldTimerManager().SetTimer(ReviveTimerHandle, [this]() {
          bCanRevive = true;
          UE_LOG(LogTemp, Warning, TEXT("🛡️ 불굴의 의지 재사용 가능!"));
       }, CurrentReviveCooldown, false);

       return 0.0f; 
    }
    
    float ActualDamage = Super::TakeDamage(ReducedDamage, DamageEvent, EventInstigator, DamageCauser);

    CurrentHP -= ActualDamage;
    UE_LOG(LogTemp, Warning, TEXT("플레이어 피격! 원본데미지: %f -> 적용데미지: %f / 남은체력: %f"), DamageAmount, ActualDamage, CurrentHP);

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

    DisableInput(PC);
    UGameplayStatics::SetGamePaused(GetWorld(), true);
    
    if (GameOverWidgetClass)
    {
       UUserWidget* GameOverWidget = CreateWidget<UUserWidget>(PC, GameOverWidgetClass);
       if (GameOverWidget)
       {
          GameOverWidget->AddToViewport(100); 
       }
    }

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
       CurrentAmmo += 5; 
       CurrentAmmo = FMath::Clamp(CurrentAmmo, 0, MaxAmmo);
       break;

    case ELevelUpUpgradeType::Damage:
       Damage += 5.f;
       break;

    case ELevelUpUpgradeType::MoveSpeed:
       MoveSpeed += 50.f;
       if (!bIsSprinting) // 달리기 중이 아닐 때만 즉시 적용
       {
           GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
       }
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
       
    default:
       break;
    }

    CloseLevelUpUI();
}

void AVoidTriggerCharacter::CloseLevelUpUI()
{
    PendingLevelUps--;

    if (LevelUpWidgetInstance)
    {
       if (LevelUpWidgetInstance->IsInViewport())
       {
          LevelUpWidgetInstance->RemoveFromParent();
       }
       LevelUpWidgetInstance = nullptr; 
    }

    if (PendingLevelUps > 0)
    {
       ShowLevelUpUI(); 
    }
    else
    {
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
    TArray<ELevelUpUpgradeType> LotteryPool; 

    TArray<ELevelUpUpgradeType> NormalUpgrades = {
       ELevelUpUpgradeType::ReloadSpeed, ELevelUpUpgradeType::MagazineSize,
       ELevelUpUpgradeType::Damage, ELevelUpUpgradeType::MoveSpeed,
       ELevelUpUpgradeType::MagnetRange, ELevelUpUpgradeType::MaxHP
    };
    for (ELevelUpUpgradeType Upgrade : NormalUpgrades)
    {
       for (int i = 0; i < 10; i++) LotteryPool.Add(Upgrade);
    }

    while (SelectedUpgrades.Num() < Count && LotteryPool.Num() > 0)
    {
       int32 RandomIndex = FMath::RandRange(0, LotteryPool.Num() - 1);
       ELevelUpUpgradeType Picked = LotteryPool[RandomIndex];
        
       SelectedUpgrades.Add(Picked);
       LotteryPool.Remove(Picked); 
    }

    return SelectedUpgrades;
}

// ==========================================
// 스테미나 및 달리기 관련 함수
// ==========================================
void AVoidTriggerCharacter::StartSprint()
{
	if (CurrentStamina > 0.f)
	{
		bIsSprinting = true;
		if (GetCharacterMovement())
		{
			// ★ 핵심 변경: 현재 걷기 속도에 배율(1.5배)을 곱해서 적용합니다.
			GetCharacterMovement()->MaxWalkSpeed = MoveSpeed * SprintMultiplier;
		}
	}
}

void AVoidTriggerCharacter::StopSprint()
{
    bIsSprinting = false;
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
    }
}

float AVoidTriggerCharacter::GetStaminaPercent() const
{
    if (MaxStamina <= 0.f) return 0.f;
    return CurrentStamina / MaxStamina;
}

void AVoidTriggerCharacter::EndInvincibility()
{
    bIsInvincible = false;
    UE_LOG(LogTemp, Warning, TEXT("무적 시간 종료!"));
}

void AVoidTriggerCharacter::TogglePause(const FInputActionValue& Value)
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    bool bIsPaused = UGameplayStatics::IsGamePaused(GetWorld());

    if (!bIsPaused) 
    {
       GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
       PC->FlushPressedKeys();

       UGameplayStatics::SetGamePaused(GetWorld(), true);

       if (SettingsWidgetClass && !SettingsWidgetInstance)
       {
          SettingsWidgetInstance = CreateWidget<UUserWidget>(PC, SettingsWidgetClass);
       }

       if (SettingsWidgetInstance && !SettingsWidgetInstance->IsInViewport())
       {
          SettingsWidgetInstance->AddToViewport(1); 
       }

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
       UGameplayStatics::SetGamePaused(GetWorld(), false);

       if (SettingsWidgetInstance && SettingsWidgetInstance->IsInViewport())
       {
          SettingsWidgetInstance->RemoveFromParent();
       }

       PC->bShowMouseCursor = false;
       FInputModeGameOnly InputMode;
       PC->SetInputMode(InputMode);
    }
}

void AVoidTriggerCharacter::EquipStartingWeapon(EStartingWeaponType WeaponType)
{
    switch (WeaponType)
    {
    case EStartingWeaponType::PierceShot:
       PierceCount += 1;
       UE_LOG(LogTemp, Warning, TEXT("[무기고] 관통탄 장착 완료!"));
       break;

    case EStartingWeaponType::ScatterShot:
       ProjectileCount += 1; 
       UE_LOG(LogTemp, Warning, TEXT("[무기고] 산탄 사격 장착 완료!"));
       break;

    case EStartingWeaponType::ExplosiveShot:
       ExplosionRadius = 250.f;
       ExplosionDamage = 15.f;
       UE_LOG(LogTemp, Warning, TEXT("[무기고] 폭발탄 장착 완료!"));
       break;
       
    case EStartingWeaponType::ChainLightning:
       bIsChainLightning = true;
       UE_LOG(LogTemp, Warning, TEXT("[무기고] 도탄 장착 완료!"));
       break;
       
    case EStartingWeaponType::BlackHoleGrenade:
       bHasBlackHoleMod = true;
       UE_LOG(LogTemp, Warning, TEXT("[무기고] 블랙홀 장착 완료!"));
       break;
    }
}