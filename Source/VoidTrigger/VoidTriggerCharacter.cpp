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
#include "VoidDrone.h"
#include "VoidTriggerGold.h"
#include "VoidTriggerGameInstance.h"
#include "VoidTriggerGameMode.h"
#include "Engine/OverlapResult.h"

AVoidTriggerCharacter::AVoidTriggerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
	
	CritChance = 0.05f; 
	CritMultiplier = 1.5f;

    FPSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPSCamera"));
    FPSCamera->SetupAttachment(GetCapsuleComponent());
    FPSCamera->SetRelativeLocation(FVector(0.f, 0.f, 60.f)); 
    FPSCamera->bUsePawnControlRotation = true; 

    FPSMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPSMesh"));
    FPSMesh->SetupAttachment(FPSCamera); 
    FPSMesh->bCastDynamicShadow = false; 
    FPSMesh->CastShadow = false;
    
    GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
    GunMesh->SetupAttachment(FPSCamera); 
    GunMesh->SetRelativeLocation(FVector(40.f, 20.f, -20.f)); 
    
    MagnetCollision = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetCollision"));
    MagnetCollision->SetupAttachment(RootComponent);
    MagnetCollision->InitSphereRadius(300.f);
    MagnetCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AVoidTriggerCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    UVoidTriggerGameInstance* GI = Cast<UVoidTriggerGameInstance>(GetGameInstance());
    if (GI)
    {
        MouseSensitivity = GI->MouseSensitivity;
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
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Reload")))
       {
          float ReduceAmount = (*LvPtr) * 0.2f;
          ReloadTime = FMath::Max(0.3f, ReloadTime - ReduceAmount);
       }

       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Crit")))
       {
          CritChance += (*LvPtr) * 0.05f; 
          CritMultiplier += (*LvPtr) * 0.2f; 
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Mag")))
       {
          int32 ExtraAmmo = (*LvPtr) * 5;
          MaxAmmo += ExtraAmmo;
          CurrentAmmo = MaxAmmo; 
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("ATK_Suppress")))
       {
          SuppressionLevel = *LvPtr;
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Health")))
       {
          float HealthMultiplier = (*LvPtr) * 0.1f;
          MaxHP = MaxHP + (MaxHP * HealthMultiplier);
          CurrentHP = MaxHP;
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Speed")))
       {
           float SpeedBonus = (*LvPtr) * 50.0f;
           MoveSpeed += SpeedBonus;
            
           if (GetCharacterMovement())
           {
              GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
           }

           float StaminaBonus = (*LvPtr) * 30.0f;
           MaxStamina += StaminaBonus;
           CurrentStamina = MaxStamina;
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Armor")))
       {
          DamageReductionRate = FMath::Min(0.5f, (*LvPtr) * 0.05f);
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Heal")))
       {
          LevelUpHealRate = (*LvPtr) * 0.1f;
       }
       
       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("SUR_Revive")))
       {
           bHasReviveTrait = true;
           CurrentReviveCooldown = FMath::Max(30.0f, 120.0f - ((*LvPtr) * 20.0f));
       }

       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Drop")))
       {
           ResourceDropBonus = (*LvPtr) * 0.15f;
       }

       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Rarity")))
       {
           SpecialDropBonus = (*LvPtr) * 0.01f;
       }

       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Discount")))
       {
           DiscountRate = (*LvPtr) * 0.05f;
       }

       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_Reroll")))
       {
           ExtraChoiceCount = (*LvPtr >= 3) ? 2 : 1; 
           MaxRerollCount = *LvPtr; 
           CurrentRerollCount = MaxRerollCount;
       }

       if (int32* LvPtr = SaveData->TraitLevels.Find(FName("UTL_StartCash")))
       {
          for(int i = 0; i < *LvPtr; ++i)
          {
             GainExp(MaxExp); 
          }
       }

    	// 영구 누적 데이터 동기화
    	SessionTotalKills = SaveData->TotalMonsterKills;
    	SessionTotalCrits = SaveData->TotalCriticalHits;

    	// 이미 해금된 무기인지 체크 (중복 저장 방지)
    	if (bool* bUnlocked = SaveData->UnlockedWeapons.Find(EStartingWeaponType::ScatterShot))   bAlreadyUnlocked_Scatter = *bUnlocked;
    	if (bool* bUnlocked = SaveData->UnlockedWeapons.Find(EStartingWeaponType::ExplosiveShot)) bAlreadyUnlocked_Explosive = *bUnlocked;
    	if (bool* bUnlocked = SaveData->UnlockedWeapons.Find(EStartingWeaponType::AcidFloor))     bAlreadyUnlocked_Acid = *bUnlocked;
    	if (bool* bUnlocked = SaveData->UnlockedWeapons.Find(EStartingWeaponType::AutoDrone))     bAlreadyUnlocked_Drone = *bUnlocked;
    	if (bool* bUnlocked = SaveData->UnlockedWeapons.Find(EStartingWeaponType::ChainLightning)) bAlreadyUnlocked_Chain = *bUnlocked;
    	if (bool* bUnlocked = SaveData->UnlockedWeapons.Find(EStartingWeaponType::BlackHoleGrenade)) bAlreadyUnlocked_BlackHole = *bUnlocked;
    	
       // 무기고 영구 저장 스탯 일괄 적용
       if (SaveData->bHasSavedStartingTrait && SaveData->SavedArmoryWeaponStats.Num() > 0)
       {
          InitializeArmoryWeapons(SaveData->SavedArmoryWeaponStats);
       }
    }
    
	// 1. 에디터에서 미니맵 클래스(WBP_Minimap)를 정상적으로 할당했는지 확인
	if (MinimapWidgetClass)
	{
		// 2. 위젯 생성 (블루프린트의 Create Widget 노드와 동일)
		MinimapWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), MinimapWidgetClass);
        
		if (MinimapWidgetInstance)
		{
			// 3. 화면에 띄우기 (블루프린트의 Add to Viewport 노드와 동일)
			MinimapWidgetInstance->AddToViewport();
            
			// 참고: ZOrder(화면 겹침 순서)를 정하고 싶다면 AddToViewport(0) 처럼 숫자를 넣으면 됩니다.
		}
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

void AVoidTriggerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
	
    if (bIsSprinting && GetCharacterMovement()->Velocity.SizeSquared2D() > 0.f)
    {
        CurrentStamina -= StaminaConsumeRate * DeltaTime;

        if (CurrentStamina <= 0.f)
        {
            CurrentStamina = 0.f;
            StopSprint();
        }
    }
    else
    {
        if (CurrentStamina < MaxStamina)
        {
            CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + (StaminaRegenRate * DeltaTime));
        }
    }
	
	// ==========================================
	// 업적 카운팅 (시간 누적)
	// ==========================================
	CurrentMatch_SurvivalTime += DeltaTime;

	if (bIsSprinting && GetCharacterMovement()->Velocity.SizeSquared2D() > 10.f)
	{
		CurrentMatch_SprintDuration += DeltaTime;
	}

	// 매 프레임 업적 체크
	CheckAndSaveArmoryUnlocks();
}

void AVoidTriggerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
       EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVoidTriggerCharacter::Move);
       EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AVoidTriggerCharacter::Look);
       
       EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AVoidTriggerCharacter::StartFire);
       EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AVoidTriggerCharacter::StopFire);
       
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
        UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashEffect, FPSMesh, FName("Muzzle"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
    }

    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
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

        if (ProjectileCount > 1 && ShotIndex > 0)
        {
            ShotDirection = FMath::VRandCone(BaseForward, FMath::DegreesToRadians(5.f));
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

            bool bHit = GetWorld()->SweepSingleByChannel(HitResult, DynamicStartLocation, TraceEnd, FQuat::Identity, ECC_Visibility, SphereShape, CollisionParams);

            if (bHit)
            {
                AActor* HitActor = HitResult.GetActor();
                if (HitActor)
                {
                    if (BulletTracerEffect)
                    {
                        FRotator TracerRotation = (HitResult.ImpactPoint - VisualStartLocation).Rotation();
                        UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BulletTracerEffect, VisualStartLocation, TracerRotation);
                        if (TracerComp)
                        {
                            TracerComp->SetVariableVec3(FName("User.BeamEnd"), HitResult.ImpactPoint);
                        }
                    }

                	float FinalDamage = Damage;
                	bool bIsCrit = (FMath::FRand() <= CritChance);

                	if (bIsCrit)
                	{
                		FinalDamage *= CritMultiplier;
                		SessionTotalCrits++;
                	}

                    UGameplayStatics::ApplyDamage(HitActor, FinalDamage, GetController(), this, UDamageType::StaticClass());
                	CurrentMatch_TotalDamageDealt += FinalDamage;
                	
                	
                    if (HitActor->ActorHasTag(FName("Monster")))
                    {
                        if (HitImpactEffect)
                        {
                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitImpactEffect, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation());
                        }

                    	OnHitMarkerEvent.Broadcast(bIsCrit);
                    	
                        if (bIsChainLightning && MaxChainCount > 0)
                        {
                            FVector Origin = HitResult.ImpactPoint; 
                            FCollisionShape ChainSphereShape = FCollisionShape::MakeSphere(ChainRadius);
                            TArray<FOverlapResult> OverlapResults;
        
                            FCollisionQueryParams OverlapParams;
                            OverlapParams.AddIgnoredActor(this);     
                            OverlapParams.AddIgnoredActor(HitActor); 

                            bool bFoundChainTarget = GetWorld()->OverlapMultiByChannel(OverlapResults, Origin, FQuat::Identity, ECC_Visibility, ChainSphereShape, OverlapParams);

                            if (bFoundChainTarget)
                            {
                                int32 ChainedCount = 0;
                                for (const FOverlapResult& Result : OverlapResults)
                                {
                                    if (ChainedCount >= MaxChainCount) break; // 스탯 포인트만큼 제한

                                    AActor* ChainTarget = Result.GetActor();
                                    if (ChainTarget && ChainTarget->ActorHasTag(FName("Monster")))
                                    {
                                        UGameplayStatics::ApplyDamage(ChainTarget, ChainDamage, GetController(), this, UDamageType::StaticClass());
                    
                                        if (HitImpactEffect)
                                        {
                                            FVector EffectLocation = ChainTarget->GetActorLocation();
                                            FRotator EffectRotation = (Origin - EffectLocation).Rotation();
                                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitImpactEffect, EffectLocation, EffectRotation);
                                        }

                                        if (BulletTracerEffect)
                                        {
                                            UNiagaraComponent* BounceTracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BulletTracerEffect, Origin);
                                            if (BounceTracerComp)
                                            {
                                                BounceTracerComp->SetVariableVec3(FName("User.BeamEnd"), ChainTarget->GetActorLocation());
                                            }
                                        }
                                        ChainedCount++; 
                                    }
                                }
                            }
                        }

                        // ★ 블랙홀 수류탄: 레벨(포인트)에 비례한 확률 적용
                        if (bHasBlackHoleMod && FMath::FRand() <= BlackHoleChance)
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
                            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, HitResult.ImpactPoint, FRotator::ZeroRotator);
                        }

                        if (ExplosionSound)
                        {
                            UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, HitResult.ImpactPoint);
                        }

                        TArray<AActor*> IgnoredActors;
                        IgnoredActors.Add(this);

                        UGameplayStatics::ApplyRadialDamage(GetWorld(), ExplosionDamage, HitResult.ImpactPoint, ExplosionRadius, UDamageType::StaticClass(), IgnoredActors, this, GetController());
                    }
                }
            }
            else
            {
                if (BulletTracerEffect)
                {
                    FRotator MissRotation = (TraceEnd - VisualStartLocation).Rotation();
                    UNiagaraComponent* MissTracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BulletTracerEffect, VisualStartLocation, MissRotation);
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
    if (CurrentExp >= MaxExp)
    {
       LevelUp();
    }
}

void AVoidTriggerCharacter::LevelUp()
{
	float TotalHealRate = 0.1f + LevelUpHealRate; 
	float HealAmount = MaxHP * TotalHealRate;
	CurrentHP = FMath::Min(MaxHP, CurrentHP + HealAmount);
    
    CurrentExp -= MaxExp;
    Level++;
    MaxExp *= 1.1f;
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

    if (ReloadMontage)
    {
       if (UAnimInstance* AnimInstance = FPSMesh->GetAnimInstance())
       {
          float MontageLength = ReloadMontage->GetPlayLength();
          float PlayRate = MontageLength / ReloadTime;
          AnimInstance->Montage_Play(ReloadMontage, PlayRate);
       }
    }
    FTimerHandle ReloadTimerHandle;
    GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AVoidTriggerCharacter::FinishReload, ReloadTime, false);
}

void AVoidTriggerCharacter::FinishReload()
{
    bIsReloading = false;
    CurrentAmmo = MaxAmmo; 
}

float AVoidTriggerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsInvincible) return 0.0f;
    
    float ReducedDamage = DamageAmount * (1.0f - DamageReductionRate);
    
    if (CurrentHP - ReducedDamage <= 0.0f && bHasReviveTrait && bCanRevive)
    {
       CurrentHP = 1.0f;   
       bCanRevive = false; 
       bIsInvincible = true; 

       GetWorldTimerManager().SetTimer(InvincibleTimer, this, &AVoidTriggerCharacter::EndInvincibility, ReviveInvincibleDuration, false);

       FTimerHandle ReviveTimerHandle;
       GetWorldTimerManager().SetTimer(ReviveTimerHandle, [this]() {
          bCanRevive = true;
       }, CurrentReviveCooldown, false);

       return 0.0f; 
    }
    
    float ActualDamage = Super::TakeDamage(ReducedDamage, DamageEvent, EventInstigator, DamageCauser);
    CurrentHP -= ActualDamage;

    if (CurrentHP <= 0.f)
    {
       Die();
    }
    return ActualDamage;
}

void AVoidTriggerCharacter::Die()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

	AVoidTriggerGameMode* GM = Cast<AVoidTriggerGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GM)
	{
		GM->GameOver(); // 여기서 최고 기록 갱신 및 디스크 저장이 일어남
	}
	
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
       if (!bIsSprinting) GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
       break;
    case ELevelUpUpgradeType::MagnetRange:
       if (MagnetCollision)
       {
          MagnetCollision->SetSphereRadius(MagnetCollision->GetScaledSphereRadius() + 75.f);
       }
       break;
    case ELevelUpUpgradeType::MaxHP:
       MaxHP += 20.f;
       CurrentHP += 20.f;
       CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
       break;
    case ELevelUpUpgradeType::CriticalChance:
    	CritChance = FMath::Min(1.0f, CritChance + 0.05f);
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
		LotteryPool.Add(Upgrade);
    }

	if (CritChance < 1.0f)
	{
		LotteryPool.Add(ELevelUpUpgradeType::CriticalChance);
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

void AVoidTriggerCharacter::StartSprint()
{
    if (CurrentStamina > 0.f)
    {
       bIsSprinting = true;
       if (GetCharacterMovement())
       {
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

void AVoidTriggerCharacter::SpawnFireTrail()
{
    if (!FireFloorClass) return;

    if (GetCharacterMovement()->Velocity.SizeSquared2D() > 10.f)
    {
       FVector SpawnLocation = GetActorLocation();
       SpawnLocation.Z -= 80.f; 
       GetWorld()->SpawnActor<AActor>(FireFloorClass, SpawnLocation, FRotator::ZeroRotator);
    }
}

void AVoidTriggerCharacter::InitializeArmoryWeapons(const TMap<EStartingWeaponType, int32>& ArmoryStats)
{
    SpecialWeaponLevels = ArmoryStats;

    for (const auto& Stat : ArmoryStats)
    {
        EStartingWeaponType WeaponType = Stat.Key;
        int32 WeaponLevel = Stat.Value; 

        if (WeaponLevel <= 0) continue;

        switch (WeaponType)
        {
        case EStartingWeaponType::PierceShot:
            PierceCount += WeaponLevel;
            break;

        case EStartingWeaponType::ScatterShot:
            ProjectileCount += WeaponLevel;
            break;

        case EStartingWeaponType::ExplosiveShot:
            ExplosionRadius = 100.f + (50.f * WeaponLevel);
            ExplosionDamage = 15.f * WeaponLevel;
            break;
           
        case EStartingWeaponType::ChainLightning:
            bIsChainLightning = true;
            MaxChainCount = WeaponLevel;
            break;
           
        case EStartingWeaponType::BlackHoleGrenade:
            bHasBlackHoleMod = true;
            BlackHoleChance = 0.03f * WeaponLevel;
            break;
            
        case EStartingWeaponType::AcidFloor:
            bHasAcidFloorMod = true;
            AcidFloorDamage = 10.f * WeaponLevel;
            GetWorldTimerManager().SetTimer(FireTrailTimer, this, &AVoidTriggerCharacter::SpawnFireTrail, 0.3f, true);
            break;
            
        case EStartingWeaponType::AutoDrone:
        	DroneAttackRate = FMath::Max(0.5f, 2.0f - (0.3f * WeaponLevel));

        	if (DroneClass)
        	{
        		FVector SpawnLocation = GetActorLocation();
        		AVoidDrone* SpawnedDrone = GetWorld()->SpawnActor<AVoidDrone>(DroneClass, SpawnLocation, FRotator::ZeroRotator);
                
        		if (SpawnedDrone)
        		{
        			// 이제 드론이 초기화될 때 바뀐 공속(DroneAttackRate)을 정상적으로 가져갑니다.
        			SpawnedDrone->InitializeDrone(this);
        			SpawnedDrone->PrimaryActorTick.TickGroup = TG_PostUpdateWork;

        			UCameraComponent* PlayerCamera = FindComponentByClass<UCameraComponent>();
        			if (PlayerCamera)
        			{
        				SpawnedDrone->AttachToComponent(PlayerCamera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        			}
        			else
        			{
        				SpawnedDrone->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        			}

        			SpawnedDrone->SetActorRelativeLocation(SpawnedDrone->HoverOffset);
        			SpawnedDrone->SetActorRelativeRotation(FRotator::ZeroRotator);
        		}
        	}
        	break;
        }
    }
}

void AVoidTriggerCharacter::NotifyMonsterKill(AActor* KilledMonster)
{
    if (!KilledMonster) return;

    // 1. 총 킬 수 누적
    SessionTotalKills++;

    // 2. 근접 처치(5m = 500 units) 체크
    float Distance = FVector::Dist(GetActorLocation(), KilledMonster->GetActorLocation());
    if (Distance <= 500.f)
    {
        CurrentMatch_CloseKills++;
    }

    // 킬이 발생했을 때도 체크
    CheckAndSaveArmoryUnlocks();
}

void AVoidTriggerCharacter::CheckAndSaveArmoryUnlocks()
{
    bool bNeedToSave = false;

    // 1. 산탄 사격 (5m 이내 100마리)
    if (!bAlreadyUnlocked_Scatter && CurrentMatch_CloseKills >= 100)
    {
        bAlreadyUnlocked_Scatter = true;
        bNeedToSave = true;
        UE_LOG(LogTemp, Error, TEXT("🎉 업적 달성! [산탄 사격] 해금!"));
    }

    // 2. 폭발탄 (단일 판 누적 데미지 50,000)
    if (!bAlreadyUnlocked_Explosive && CurrentMatch_TotalDamageDealt >= 50000.f)
    {
        bAlreadyUnlocked_Explosive = true;
        bNeedToSave = true;
        UE_LOG(LogTemp, Error, TEXT("🎉 업적 달성! [폭발탄] 해금!"));
    }

    // 3. 화염 장판 (달리기 6분)
    if (!bAlreadyUnlocked_Acid && CurrentMatch_SprintDuration >= 360.f)
    {
        bAlreadyUnlocked_Acid = true;
        bNeedToSave = true;
        UE_LOG(LogTemp, Error, TEXT("🎉 업적 달성! [산성 화염 장판] 해금!"));
    }

    // 4. 자동공격 드론 (10분 생존)
    if (!bAlreadyUnlocked_Drone && CurrentMatch_SurvivalTime >= 600.f)
    {
        bAlreadyUnlocked_Drone = true;
        bNeedToSave = true;
        UE_LOG(LogTemp, Error, TEXT("🎉 업적 달성! [자동공격 드론] 해금!"));
    }

    // 5. 체인 라이트닝 (치명타 누적 1,500회)
    if (!bAlreadyUnlocked_Chain && SessionTotalCrits >= 1500)
    {
        bAlreadyUnlocked_Chain = true;
        bNeedToSave = true;
        UE_LOG(LogTemp, Error, TEXT("🎉 업적 달성! [체인 라이트닝] 해금!"));
    }

    // 6. 블랙홀 수류탄 (킬 누적 5,000마리)
    if (!bAlreadyUnlocked_BlackHole && SessionTotalKills >= 5000)
    {
        bAlreadyUnlocked_BlackHole = true;
        bNeedToSave = true;
        UE_LOG(LogTemp, Error, TEXT("🎉 업적 달성! [블랙홀 수류탄] 해금!"));
    }

    // 달성한 업적이 있어서 갱신이 필요하다면 세이브 파일에 덮어쓰기
    if (bNeedToSave)
    {
        if (UMySaveGame* SaveData = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Save01"), 0)))
        {
            SaveData->UnlockedWeapons.Add(EStartingWeaponType::ScatterShot, bAlreadyUnlocked_Scatter);
            SaveData->UnlockedWeapons.Add(EStartingWeaponType::ExplosiveShot, bAlreadyUnlocked_Explosive);
            SaveData->UnlockedWeapons.Add(EStartingWeaponType::AcidFloor, bAlreadyUnlocked_Acid);
            SaveData->UnlockedWeapons.Add(EStartingWeaponType::AutoDrone, bAlreadyUnlocked_Drone);
            SaveData->UnlockedWeapons.Add(EStartingWeaponType::ChainLightning, bAlreadyUnlocked_Chain);
            SaveData->UnlockedWeapons.Add(EStartingWeaponType::BlackHoleGrenade, bAlreadyUnlocked_BlackHole);

            SaveData->TotalMonsterKills = SessionTotalKills;
            SaveData->TotalCriticalHits = SessionTotalCrits;

            UGameplayStatics::SaveGameToSlot(SaveData, TEXT("Save01"), 0);
        }
    }
}