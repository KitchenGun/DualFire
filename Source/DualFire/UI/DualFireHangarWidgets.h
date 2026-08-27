// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "CommonButtonBase.h"
#include "CommonTabListWidgetBase.h"
#include "CommonUserWidget.h"
#include "Core/DualFireDataTypes.h"
#include "UI/DualFireHangarTypes.h"
#include "UI/DualFireMenuScreenWidget.h"
#include "DualFireHangarWidgets.generated.h"

class UCommonListView;
class UCommonTextBlock;
class UHorizontalBox;
class UImage;
class UInputAction;
class UWidget;

/** UCommonTabListWidgetBase를 WBP에서 배치할 수 있게 하는 구체 클래스다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireHangarTabListWidget : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UHorizontalBox> TabButtonContainer;
};

/** 선택된 장비 하나를 기체 프리뷰 아래에 표시하는 모듈형 슬롯이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireHangarLoadoutSlotWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Hangar")
	void SetSlotLabel(const FText& InLabel);

	UFUNCTION(BlueprintCallable, Category="Hangar")
	void SetItem(const FDualFireHangarItemViewData& Item, bool bIsPreview);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> SlotLabel;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> ItemName;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UWidget> PreviewIndicator;
};

/** 현재 DraftLoadout과 포커스 중인 후보를 함께 시각화한다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireHangarLoadoutPreviewWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Hangar")
	void SetLoadoutItems(const TArray<FDualFireHangarItemViewData>& Items);

	UFUNCTION(BlueprintCallable, Category="Hangar")
	void PreviewItem(const FDualFireHangarItemViewData& Item);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UImage> AircraftImage;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> AircraftName;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireHangarLoadoutSlotWidget> PrimarySlot;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireHangarLoadoutSlotWidget> Special1Slot;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireHangarLoadoutSlotWidget> Special2Slot;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireHangarLoadoutSlotWidget> SuperSlot;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireHangarLoadoutSlotWidget> ShieldSlot;

private:
	void ApplyItem(const FDualFireHangarItemViewData& Item, bool bIsPreview);
	void RestoreCommittedItems();

	TMap<EDualFireHangarCategory, FDualFireHangarItemViewData> CommittedItems;
};

/** 아이콘, 이름, 장착 상태를 표시하는 UCommonListView 행이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireHangarItemEntryWidget : public UCommonButtonBase, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UDualFireHangarItemEntryWidget();

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnCurrentTextStyleChanged() override;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> ItemName;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UWidget> EquippedIndicator;

private:
	void RefreshEntry();

	UPROPERTY(Transient)
	TObjectPtr<UDualFireHangarItemObject> ItemObject;
};

/** 시작 메뉴와 전투 레벨 사이에서 DraftLoadout을 편집하는 격납고 화면이다. */
UCLASS(BlueprintType, Blueprintable)
class DUALFIRE_API UDualFireHangarWidget : public UDualFireMenuScreenWidget
{
	GENERATED_BODY()

public:
	UDualFireHangarWidget();

	UFUNCTION(BlueprintCallable, Category="Hangar")
	void EquipFocusedItem();

	UFUNCTION(BlueprintCallable, Category="Hangar")
	void Sortie();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hangar|Data")
	FLoadoutRowHandles InitialLoadoutRows;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hangar|Data")
	TSoftObjectPtr<UTexture2D> MissingItemIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hangar|Input")
	TObjectPtr<UInputAction> PreviousCategoryInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hangar|Input")
	TObjectPtr<UInputAction> NextCategoryInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hangar|Input")
	TObjectPtr<UInputAction> SortieInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hangar")
	TSubclassOf<UCommonButtonBase> CategoryButtonClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hangar")
	TSubclassOf<UCommonButtonStyle> CategoryButtonStyleClass;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireHangarLoadoutPreviewWidget> LoadoutPreview;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonListView> ItemList;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UDualFireHangarTabListWidget> CategoryTabs;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> CategoryTitle;

	UPROPERTY(BlueprintReadOnly, Category="Hangar|Widgets", meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> StatusText;

private:
	UFUNCTION()
	void HandleTabSelected(FName TabID);

	void RegisterCategoryTabs();
	void RegisterInputActions();
	void ChangeCategory(int32 Direction);
	void RefreshItemList();
	void RefreshLoadoutPreview(const FDualFireHangarItemViewData* PreviewItem = nullptr);
	void BuildItemsForCategory(EDualFireHangarCategory Category, TArray<FDualFireHangarItemViewData>& OutItems) const;
	bool ResolveItem(EDualFireHangarCategory Category, FName ItemID, FDualFireHangarItemViewData& OutItem) const;
	void EquipItem(UDualFireHangarItemObject* ItemObject);
	void HandleItemClicked(UObject* Item);
	void HandleItemSelectionChanged(UObject* Item);
	void SetStatus(const FText& Message);
	void SelectInvalidField(FName InvalidField);

	FName GetDraftItemID(EDualFireHangarCategory Category) const;
	void SetDraftItemID(EDualFireHangarCategory Category, FName ItemID);

	FLoadout DraftLoadout;
	EDualFireHangarCategory ActiveCategory = EDualFireHangarCategory::Aircraft;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDualFireHangarItemObject>> VisibleItems;

	bool bSortieRequested = false;
};
