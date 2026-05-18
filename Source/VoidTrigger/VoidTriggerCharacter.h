// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"

class USphereComponent;

#include "VoidTriggerCharacter.generated.h"

UENUM(BlueprintType)
enum class ELevelUpUpgradeType : uint8
{
	ReloadSpeed		UMETA(DisplayName = "재장전 속도 증가"),
	MagazineSize	UMETA(DisplayName = "탄창 수 증가"),
	Damage			UMETA(DisplayName = "공격력 증가"),
	MoveSpeed		UMETA(DisplayName = "이동속도 증가"),
	MagnetRange		UMETA(DisplayName = "자력 범위 증가"),
	MaxHP			UMETA(DisplayName = "최대 체력 증가")
};

// 2. ★ 로비 무기고 전용 Enum을 새로 만듭니다!
UENUM(BlueprintType)
enum class EStartingWeaponType : uint8
{
	PierceShot        UMETA(DisplayName = "관통탄"),
	ScatterShot       UMETA(DisplayName = "산탄 사격"),
	ExplosiveShot     UMETA(DisplayName = "폭발탄"),
	ChainLightning    UMETA(DisplayName = "체인 라이트닝"),
	BlackHoleGrenade  UMETA(DisplayName = "블랙홀 수류탄"),
	AcidFloor         UMETA(DisplayName = "산성 화염 장판"),
	AutoDrone         UMETA(DisplayName = "자동공격 드론")
};

UCLASS()
class VOIDTRIGGER_API AVoidTriggerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVoidTriggerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    // 1인칭 시점을 담당할 카메라
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* FPSCamera;

    // 가상의 총기 (나중에 큐브 도형을 씌울 곳)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    class UStaticMeshComponent* GunMesh;
	
	// --- 입력 에셋 (에디터에서 넣어줄 변수들) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;
	
	// 자석 (아이템 획득 범위) 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	USphereComponent* MagnetCollision;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> HUDClass;

	// 레벨업 선택 UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> LevelUpWidgetClass;

	UPROPERTY()
	UUserWidget* LevelUpWidgetInstance;

	// 자석 범위에 무언가 닿았을 때 실행될 함수 (UFUNCTION 필수)
	UFUNCTION()
	void OnMagnetOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ShootAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* DashAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PauseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ReloadAction;
	
	// [추가] 띄울 설정창 UI 클래스 (에디터에서 WBP_Settings 지정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> SettingsWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;
	
	// [추가] 생성된 위젯을 기억할 포인터 (플립플롭 B핀 역할)
	UPROPERTY()
	UUserWidget* SettingsWidgetInstance;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class USoundBase* FireSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UNiagaraSystem* MuzzleFlashEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	class UNiagaraSystem* HitImpactEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	UNiagaraSystem* ExplosionEffect;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	USoundBase* ExplosionSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float MouseSensitivity = 1.0f;
	
	// 재장전 중인지 여부
	bool bIsReloading = false;

	// 재장전 걸리는 시간 (2초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Status")
	float ReloadTime = 2.0f;

	// [추가] 사격을 반복해 줄 타이머
	FTimerHandle AutoFireTimerHandle;

	// [추가] 연사 속도 (예: 0.1f = 0.1초마다 1발 = 초당 10발)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float FireRate = 0.1f;

	// [추가] 마우스를 누를 때 / 뗄 때 실행할 함수
	void StartFire();
	void StopFire();
	
	// 재장전을 실제로 실행할 함수
	void StartReload();
	void FinishReload();

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser) override;

	void Die();
	
	// --- 대시(Dash) 시스템 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashStrength = 3000.0f; // 대시의 세기 (밀어내는 힘)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashCooldown = 5.0f; // 대시 재사용 대기시간

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float InvincibleDuration = 0.5f; // 대시 중 무적 시간 (0.5초면 충분히 깁니다)

	// 상태 체크 변수
	bool bCanDash = true;
	bool bIsInvincible = false;

	// 타이머 핸들
	FTimerHandle DashCooldownTimer;
	FTimerHandle InvincibleTimer;
	
public:
	// --- 성장 시스템 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Growth")
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Growth")
	float CurrentExp = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Growth")
	float MaxExp = 100.f; // 레벨 1에서 필요한 경험치

	// --- 사격 관련 스탯 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackRange = 5000.f; // 사정거리 (50미터)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Damage = 20.f; // 공격력

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float CritChance = 0.0f; // 기본 치명타 확률은 0% (또는 원하시는 수치)

	// 치명타 피해 배율 (예: 1.5면 데미지 1.5배)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float CritMultiplier = 1.5f;
	
	// 제압 사격 레벨 (0이면 발동 안 함)
	int32 SuppressionLevel = 0;

	// 레벨당 이동 속도 감소치 (예: 0.1이면 10% 감소)
	float SuppressionSlowPower = 0.15f;
	
	// 관통 가능한 횟수 (0이면 닿자마자 소멸, 1이면 1마리 뚫고 2마리째에 소멸)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	int32 PierceCount = 0;

	// 한 번 사격 시 발사되는 총알 수 (기본 1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	int32 ProjectileCount = 1;

	// 폭발 반경 (0이면 폭발탄이 없는 상태)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	float ExplosionRadius = 0.f;

	// 폭발 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	float ExplosionDamage = 0.f;
	
	// 이동 속도 업그레이드용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Status")
	float MoveSpeed = 600.f;

	// --- 체력 시스템 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Status")
	float CurrentHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Status")
	float MaxHP = 100.f;
	
	// 불굴의 의지 활성화 여부 (조건 충족 시 true)
	bool bHasReviveTrait = false;
    
	// 부활 재사용 대기 중인지 확인
	bool bCanRevive = true;

	// 부활 시 부여할 무적 시간 (기본 3초)
	UPROPERTY(EditAnywhere, Category = "Player|Status")
	float ReviveInvincibleDuration = 3.0f;

	// 현재 부활 쿨타임 (레벨에 따라 변함)
	float CurrentReviveCooldown = 120.0f;
	
	// 피해 감소율 (0.0 = 0%, 0.2 = 20% 감소)
	float DamageReductionRate = 0.0f;
	
	// 레벨업 시 회복할 체력 비율 (0.2 = 최대 체력의 20% 회복)
	float LevelUpHealRate = 0.0f;
	
	// --- [유틸리티 트리 관련 변수] ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	float ResourceDropBonus = 0.0f; // (기존) 자원 채굴

	// [Tier 2] 보급품 탐색 (특수 아이템 드랍률 증가)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Stats")
	float SpecialDropBonus = 0.0f; 

	// [Tier 3] 상인 로비 (상점 할인율: 0.1 = 10% 할인)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Stats")
	float DiscountRate = 0.0f;

	// [Tier 4] 전략적 선택 (선택지 개수 증가 및 리롤 횟수)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Stats")
	int32 ExtraChoiceCount = 0; 
    
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	int32 MaxRerollCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Player|Stats")
	int32 CurrentRerollCount = 0; // 이번 판에 남은 리롤 횟수
	
	// --- 탄창 시스템 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Status")
	int32 CurrentAmmo = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Status")
	int32 MaxAmmo = 30;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* FireMontage;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* ReloadMontage;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* FPSMesh;
	
	// --- 사격 실행 함수 ---
	void Fire();

	// --- 움직임 실행 함수 ---
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// 경험치를 얻는 함수
	UFUNCTION(BlueprintCallable, Category = "Player|Growth")
	void GainExp(float Amount);

	int32 PendingLevelUps = 0;
	
	// 레벨업 처리를 하는 함수
	UFUNCTION(BlueprintCallable, Category = "Player|Growth")
	void LevelUp();

	void ShowLevelUpUI();
	// 선택한 업그레이드 적용
	UFUNCTION(BlueprintCallable, Category = "Player|Growth")
	void ApplyLevelUpUpgrade(ELevelUpUpgradeType UpgradeType);

	// 레벨업 UI 닫고 게임 재개
	UFUNCTION(BlueprintCallable, Category = "Player|Growth")
	void CloseLevelUpUI();
	
	// 무작위로 'Count' 개수만큼의 업그레이드 항목을 뽑아서 배열로 반환합니다.
	UFUNCTION(BlueprintCallable, Category = "Player|Growth")
	TArray<ELevelUpUpgradeType> GetRandomUpgrades(int32 Count);
	
	UFUNCTION(BlueprintPure, Category = "Movement|Dash")
	float GetDashCooldownPercent() const;
	
	void TogglePause(const FInputActionValue& Value);

	void Dash();
	void ResetDash();
	void EndInvincibility();
	
	void EquipStartingWeapon(EStartingWeaponType WeaponType);
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	bool bIsChainLightning = false;

	// 연쇄 번개가 튕길 반경 (예: 5미터 = 500.f)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	float ChainRadius = 500.f;

	// 연쇄 도탄(팅김) 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	float ChainDamage = 10.f;

	// ★ [추가] 총구에서 적, 혹은 적에서 적에게 날아갈 '총알 궤적' 이펙트 (나이아가라 리본/빔)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects|Special")
	class UNiagaraSystem* BulletTracerEffect;
	
	// AVoidTriggerCharacter.h
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	bool bHasBlackHoleMod = false; // 무기고에서 선택하면 true로 변경

	UPROPERTY(EditAnywhere, Category = "Combat|Special")
	TSubclassOf<AActor> BlackHoleClass;
};