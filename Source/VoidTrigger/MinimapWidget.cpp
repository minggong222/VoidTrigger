#include "MinimapWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "VoidTriggerGameInstance.h"

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    APawn* PlayerPawn = GetOwningPlayerPawn();
    UVoidTriggerGameInstance* GameInst = Cast<UVoidTriggerGameInstance>(GetGameInstance());
    
    if (!PlayerPawn || !GameInst || !RadarCanvas || !EnemyIconClass) return;

    FVector PlayerLoc = PlayerPawn->GetActorLocation();
    float PlayerYaw = PlayerPawn->GetControlRotation().Yaw;

    int32 ActiveIconIndex = 0; // 이번 프레임에 실제로 화면에 그릴 아이콘의 개수

    // 1. 명부에 있는 적들만 순회 (탐색 비용 제로)
    for (AActor* Target : GameInst->ActiveRadarTargets)
    {
        if (!IsValid(Target)) continue;

        FVector TargetLoc = Target->GetActorLocation();
        FVector RelativeLoc = TargetLoc - PlayerLoc;

        // 2. 최적화: 레이더 범위를 벗어난 적은 계산 스킵 (빠른 거리 계산을 위해 SizeSquared 사용)
        if (RelativeLoc.SizeSquared() > FMath::Square(MaxRadarRange))
        {
            continue;
        }

        // 3. 오브젝트 풀링: 아이콘이 부족하면 새로 만들고, 아니면 기존 것 재사용
        if (ActiveIconIndex >= IconPool.Num())
        {
            UUserWidget* NewIcon = CreateWidget<UUserWidget>(this, EnemyIconClass);
            RadarCanvas->AddChild(NewIcon);
            
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NewIcon->Slot))
            {
                CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
                CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
                CanvasSlot->SetAutoSize(true); // <--- 이 줄을 꼭 추가해 주세요! (내부 컨텐츠 크기에 맞춤)

            }
            IconPool.Add(NewIcon);
        }

        // 재사용할 아이콘 꺼내기
        UUserWidget* ActiveIcon = IconPool[ActiveIconIndex];
        ActiveIcon->SetVisibility(ESlateVisibility::HitTestInvisible); // 화면에 표시

        // 4. 2D 좌표 변환 (플레이어 카메라 회전값 기준)
        FVector RotatedLoc = FRotator(0.f, -PlayerYaw, 0.f).RotateVector(RelativeLoc);

        // 5. 실제 거리(MaxRadarRange)를 UI 픽셀 크기(RadarUIRadius)에 맞춰 비율 축소
        // 언리얼 UI는 아래쪽이 +Y 이므로 X값에 마이너스를 붙여 대입
        float ScaleRatio = RadarUIRadius / MaxRadarRange;
        FVector2D UILoc(RotatedLoc.Y * ScaleRatio, -RotatedLoc.X * ScaleRatio);

        if (UCanvasPanelSlot* IconSlot = Cast<UCanvasPanelSlot>(ActiveIcon->Slot))
        {
            IconSlot->SetPosition(UILoc);
        }

        ActiveIconIndex++;
    }

    // 6. 남은 잉여 아이콘들 숨기기 
    // (예: 저번 프레임엔 5명이 범위 내에 있었는데, 이번 프레임엔 3명이라면 2개는 화면에서 숨김)
    for (int32 i = ActiveIconIndex; i < IconPool.Num(); ++i)
    {
        IconPool[i]->SetVisibility(ESlateVisibility::Collapsed);
    }
}