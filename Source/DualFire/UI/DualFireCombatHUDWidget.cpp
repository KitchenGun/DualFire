// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireCombatHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonActionWidget.h"
#include "CommonTextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Core/DualFireViewportLayout.h"
#include "Engine/Texture2D.h"
#include "Health/HealthComponent.h"
#include "Input/CommonUIInputTypes.h"
#include "InputAction.h"
#include "Weapon/WeaponComponent.h"

namespace
{
constexpr FLinearColor HealthColor(0.9f, 0.15f, 0.15f, 1.0f);
constexpr FLinearColor ShieldColor(0.1f, 0.7f, 1.0f, 1.0f);
constexpr FLinearColor EmptySegmentColor(0.08f, 0.08f, 0.08f, 0.8f);

}

FText UDualFireCombatHUDWidget::GetAttributeBadgeLabel(const TArray<EDualFireAttribute>& Attributes)
{
	const bool bGround = Attributes.Contains(EDualFireAttribute::Ground);
	const bool bAir = Attributes.Contains(EDualFireAttribute::Air);
	if (bGround && bAir)
	{
		return NSLOCTEXT("DualFireCombatHUD", "AntiAirAndGround", "AA/AG");
	}
	if (bGround)
	{
		return NSLOCTEXT("DualFireCombatHUD", "AntiGround", "AG");
	}
	if (bAir)
	{
		return NSLOCTEXT("DualFireCombatHUD", "AntiAir", "AA");
	}
	return FText::GetEmpty();
}

FLinearColor UDualFireCombatHUDWidget::GetAttributeBadgeColor(const TArray<EDualFireAttribute>& Attributes)
{
	const bool bGround = Attributes.Contains(EDualFireAttribute::Ground);
	const bool bAir = Attributes.Contains(EDualFireAttribute::Air);
	if (bGround && bAir)
	{
		return FLinearColor::White;
	}
	if (bGround)
	{
		return FLinearColor(1.0f, 0.42f, 0.05f);
	}
	if (bAir)
	{
		return FLinearColor(0.05f, 0.82f, 0.9f);
	}
	return FLinearColor::White;
}

TOptional<FUIInputConfig> UDualFireCombatHUDWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently);
}

void UDualFireCombatHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (IsValid(Special1ActionWidget) && IsValid(Special1InputAction))
	{
		Special1ActionWidget->SetEnhancedInputAction(Special1InputAction);
	}
	if (IsValid(Special2ActionWidget) && IsValid(Special2InputAction))
	{
		Special2ActionWidget->SetEnhancedInputAction(Special2InputAction);
	}
}

void UDualFireCombatHUDWidget::NativeDestruct()
{
	UnbindObservedHealth();
	UnbindObservedWeapon();
	ObservedPawn.Reset();
	Super::NativeDestruct();
}

void UDualFireCombatHUDWidget::SetObservedPawn(APawn* InPawn)
{
	if (ObservedPawn.Get() == InPawn)
	{
		return;
	}

	UnbindObservedHealth();
	UnbindObservedWeapon();
	ObservedPawn = InPawn;
	BindObservedWeapon(IsValid(InPawn) ? InPawn->FindComponentByClass<UWeaponComponent>() : nullptr);
	BindObservedHealth(IsValid(InPawn) ? InPawn->FindComponentByClass<UHealthComponent>() : nullptr);
	RefreshSpecialPresentation();
	RefreshCooldowns();
}

void UDualFireCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshViewportLayout(MyGeometry);
	// 쿨다운은 델리게이트가 아니라 이 HUD의 단일 Tick에서만 폴링한다.
	RefreshCooldowns();
}

void UDualFireCombatHUDWidget::HandleHealthChanged(const int32 CurrentHealth, const int32 MaxHealth)
{
	SetSegments(HealthSegmentContainer, CurrentHealth, MaxHealth, HealthColor, HealthSegmentTexture);
}

void UDualFireCombatHUDWidget::HandleShieldChanged(const int32 CurrentShield, const int32 MaxShield)
{
	SetSegments(ShieldSegmentContainer, CurrentShield, MaxShield, ShieldColor, ShieldSegmentTexture);
}

void UDualFireCombatHUDWidget::HandleLifeChanged(const int32 CurrentLife)
{
	if (IsValid(LivesText))
	{
		LivesText->SetText(FText::Format(
			NSLOCTEXT("DualFireCombatHUD", "RemainingLives", "×{0}"),
			FText::AsNumber(CurrentLife)));
	}
}

void UDualFireCombatHUDWidget::HandleResolvedLoadoutApplied()
{
	RefreshSpecialPresentation();
}

void UDualFireCombatHUDWidget::BindObservedHealth(UHealthComponent* InHealthComponent)
{
	ObservedHealthComponent = InHealthComponent;
	if (!IsValid(InHealthComponent))
	{
		HandleHealthChanged(0, 0);
		HandleShieldChanged(0, 0);
		HandleLifeChanged(0);
		return;
	}

	InHealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::HandleHealthChanged);
	InHealthComponent->OnShieldChanged.AddDynamic(this, &ThisClass::HandleShieldChanged);
	InHealthComponent->OnLifeChanged.AddDynamic(this, &ThisClass::HandleLifeChanged);
	HandleHealthChanged(InHealthComponent->CurrentHealth, InHealthComponent->MaxHealth);
	HandleShieldChanged(InHealthComponent->CurrentShield, InHealthComponent->MaxShield);
	HandleLifeChanged(InHealthComponent->GetCurrentLife());
}

void UDualFireCombatHUDWidget::UnbindObservedHealth()
{
	if (UHealthComponent* HealthComponent = ObservedHealthComponent.Get())
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleHealthChanged);
		HealthComponent->OnShieldChanged.RemoveDynamic(this, &ThisClass::HandleShieldChanged);
		HealthComponent->OnLifeChanged.RemoveDynamic(this, &ThisClass::HandleLifeChanged);
	}
	ObservedHealthComponent.Reset();
}

void UDualFireCombatHUDWidget::BindObservedWeapon(UWeaponComponent* InWeaponComponent)
{
	ObservedWeaponComponent = InWeaponComponent;
	if (IsValid(InWeaponComponent))
	{
		InWeaponComponent->OnResolvedLoadoutApplied.AddUObject(this, &ThisClass::HandleResolvedLoadoutApplied);
	}
}

void UDualFireCombatHUDWidget::UnbindObservedWeapon()
{
	if (UWeaponComponent* WeaponComponent = ObservedWeaponComponent.Get())
	{
		WeaponComponent->OnResolvedLoadoutApplied.RemoveAll(this);
	}
	ObservedWeaponComponent.Reset();
}

void UDualFireCombatHUDWidget::RefreshSpecialPresentation()
{
	SetSpecialPresentation(1);
	SetSpecialPresentation(2);
}

void UDualFireCombatHUDWidget::RefreshCooldowns()
{
	const UWeaponComponent* WeaponComponent = ObservedWeaponComponent.Get();
	const float Special1Cooldown = IsValid(WeaponComponent)
		? WeaponComponent->GetCooldownRemainingPercent(ELoadoutSlot::SpecialWeapon1) : 0.0f;
	const float Special2Cooldown = IsValid(WeaponComponent)
		? WeaponComponent->GetCooldownRemainingPercent(ELoadoutSlot::SpecialWeapon2) : 0.0f;
	if (IsValid(Special1CooldownBar))
	{
		Special1CooldownBar->SetPercent(Special1Cooldown);
	}
	if (IsValid(Special2CooldownBar))
	{
		Special2CooldownBar->SetPercent(Special2Cooldown);
	}
}

void UDualFireCombatHUDWidget::RefreshViewportLayout(const FGeometry& MyGeometry)
{
	if (!IsValid(PlayableFieldHost))
	{
		return;
	}

	const FVector2D LocalSize = MyGeometry.GetLocalSize();
	const FIntPoint ViewportSize(
		FMath::Max(0, FMath::RoundToInt(LocalSize.X)),
		FMath::Max(0, FMath::RoundToInt(LocalSize.Y)));
	if (ViewportSize == CachedViewportSize || ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		return;
	}

	CachedViewportSize = ViewportSize;
	const FIntRect PlayfieldRect = DualFireViewportLayout::MakeCenteredPlayfieldScreenRect(ViewportSize);
	if (UCanvasPanelSlot* HostSlot = Cast<UCanvasPanelSlot>(PlayableFieldHost->Slot))
	{
		HostSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		HostSlot->SetAlignment(FVector2D::ZeroVector);
		HostSlot->SetPosition(FVector2D(
			static_cast<float>(PlayfieldRect.Min.X),
			static_cast<float>(PlayfieldRect.Min.Y)));
		HostSlot->SetSize(FVector2D(
			static_cast<float>(PlayfieldRect.Width()),
			static_cast<float>(PlayfieldRect.Height())));
	}
}

void UDualFireCombatHUDWidget::SetSegments(
	UHorizontalBox* Container,
	const int32 Current,
	const int32 Maximum,
	const FLinearColor& FilledColor,
	UTexture2D* SegmentTexture)
{
	if (!IsValid(Container) || !WidgetTree)
	{
		return;
	}

	Container->ClearChildren();
	for (int32 Index = 0; Index < FMath::Max(0, Maximum); ++Index)
	{
		UImage* Segment = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (IsValid(SegmentTexture))
		{
			Segment->SetBrushFromTexture(SegmentTexture, true);
		}
		Segment->SetDesiredSizeOverride(SegmentDesiredSize);
		Segment->SetColorAndOpacity(Index < Current ? FilledColor : EmptySegmentColor);
		if (UHorizontalBoxSlot* SegmentSlot = Container->AddChildToHorizontalBox(Segment))
		{
			SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SegmentSlot->SetPadding(FMargin(1.0f, 0.0f));
		}
	}
}

void UDualFireCombatHUDWidget::SetSpecialPresentation(const int32 SpecialIndex)
{
	const ELoadoutSlot WeaponSlot = SpecialIndex == 1 ? ELoadoutSlot::SpecialWeapon1 : ELoadoutSlot::SpecialWeapon2;
	FWeaponRow WeaponData;
	const bool bHasWeapon = ObservedWeaponComponent.IsValid() && ObservedWeaponComponent->GetResolvedSlotData(WeaponSlot, WeaponData);
	UImage* Icon = SpecialIndex == 1 ? Special1Icon : Special2Icon;
	UCommonTextBlock* AttributeText = SpecialIndex == 1 ? Special1AttributeText : Special2AttributeText;
	if (IsValid(Icon))
	{
		Icon->SetBrushFromSoftTexture(WeaponData.Icon, true);
		Icon->SetVisibility(bHasWeapon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (IsValid(AttributeText))
	{
		const FText Label = GetAttributeBadgeLabel(WeaponData.AttributeArray);
		AttributeText->SetText(Label);
		AttributeText->SetColorAndOpacity(FSlateColor(GetAttributeBadgeColor(WeaponData.AttributeArray)));
		AttributeText->SetVisibility(bHasWeapon && !Label.IsEmpty()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}
