// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireHangarWidgets.h"

#include "CommonListView.h"
#include "CommonTextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "DualFire.h"
#include "Engine/DataTable.h"
#include "Input/CommonUIInputTypes.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "Loadout/LoadoutManagerSubsystem.h"
#include "GameInstance/DualFireMissionFlowSubsystem.h"
#include "UI/DualFireMenuButton.h"
#include "UI/DualFireMenuButtonStyle.h"

namespace
{
const TArray<EDualFireHangarCategory>& GetCategoryOrder()
{
	static const TArray<EDualFireHangarCategory> Categories = {
		EDualFireHangarCategory::Aircraft,
		EDualFireHangarCategory::PrimaryWeapon,
		EDualFireHangarCategory::SpecialWeapon1,
		EDualFireHangarCategory::SpecialWeapon2,
		EDualFireHangarCategory::SuperWeapon,
		EDualFireHangarCategory::Shield,
	};
	return Categories;
}

FName GetCategoryID(const EDualFireHangarCategory Category)
{
	switch (Category)
	{
	case EDualFireHangarCategory::Aircraft: return TEXT("Aircraft");
	case EDualFireHangarCategory::PrimaryWeapon: return TEXT("PrimaryWeapon");
	case EDualFireHangarCategory::SpecialWeapon1: return TEXT("SpecialWeapon1");
	case EDualFireHangarCategory::SpecialWeapon2: return TEXT("SpecialWeapon2");
	case EDualFireHangarCategory::SuperWeapon: return TEXT("SuperWeapon");
	case EDualFireHangarCategory::Shield: return TEXT("Shield");
	default: return NAME_None;
	}
}

FText GetCategoryLabel(const EDualFireHangarCategory Category)
{
	switch (Category)
	{
	case EDualFireHangarCategory::Aircraft: return NSLOCTEXT("DualFireHangar", "Aircraft", "AIRCRAFT");
	case EDualFireHangarCategory::PrimaryWeapon: return NSLOCTEXT("DualFireHangar", "Primary", "PRIMARY");
	case EDualFireHangarCategory::SpecialWeapon1: return NSLOCTEXT("DualFireHangar", "Special1", "SPECIAL 1");
	case EDualFireHangarCategory::SpecialWeapon2: return NSLOCTEXT("DualFireHangar", "Special2", "SPECIAL 2");
	case EDualFireHangarCategory::SuperWeapon: return NSLOCTEXT("DualFireHangar", "SuperWeapon", "SUPER WEAPON");
	case EDualFireHangarCategory::Shield: return NSLOCTEXT("DualFireHangar", "Shield", "SHIELD");
	default: return FText::GetEmpty();
	}
}

bool TryGetCategory(const FName CategoryID, EDualFireHangarCategory& OutCategory)
{
	for (const EDualFireHangarCategory Category : GetCategoryOrder())
	{
		if (GetCategoryID(Category) == CategoryID)
		{
			OutCategory = Category;
			return true;
		}
	}
	return false;
}

template <typename RowType>
const RowType* FindTypedRow(UDataTable* Table, const FName RowName, const TCHAR* Context)
{
	return IsValid(Table) ? Table->FindRow<RowType>(RowName, Context, false) : nullptr;
}

FText ResolveDisplayName(const FText& DisplayName, const FName ItemID)
{
	return DisplayName.IsEmpty() ? FText::FromName(ItemID) : DisplayName;
}
}

void UDualFireHangarTabListWidget::HandleTabCreation_Implementation(
	const FName TabNameID,
	UCommonButtonBase* TabButton)
{
	Super::HandleTabCreation_Implementation(TabNameID, TabButton);
	if (ensure(IsValid(TabButtonContainer) && IsValid(TabButton)))
	{
		if (UHorizontalBoxSlot* TabSlot = TabButtonContainer->AddChildToHorizontalBox(TabButton))
		{
			TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TabSlot->SetPadding(FMargin(2.0f, 0.0f));
		}
	}
}

void UDualFireHangarTabListWidget::HandleTabRemoval_Implementation(
	const FName TabNameID,
	UCommonButtonBase* TabButton)
{
	Super::HandleTabRemoval_Implementation(TabNameID, TabButton);
	if (IsValid(TabButtonContainer) && IsValid(TabButton))
	{
		TabButtonContainer->RemoveChild(TabButton);
	}
}

void UDualFireHangarLoadoutSlotWidget::SetSlotLabel(const FText& InLabel)
{
	if (IsValid(SlotLabel))
	{
		SlotLabel->SetText(InLabel);
	}
}

void UDualFireHangarLoadoutSlotWidget::SetItem(
	const FDualFireHangarItemViewData& Item,
	const bool bIsPreview)
{
	if (IsValid(ItemIcon))
	{
		ItemIcon->SetBrushFromSoftTexture(Item.Icon, false);
	}
	if (IsValid(ItemName))
	{
		ItemName->SetText(Item.DisplayName);
	}
	if (IsValid(PreviewIndicator))
	{
		PreviewIndicator->SetVisibility(bIsPreview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UDualFireHangarLoadoutPreviewWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ensure(IsValid(AircraftImage) && IsValid(AircraftName) &&
		IsValid(PrimarySlot) && IsValid(Special1Slot) && IsValid(Special2Slot) &&
		IsValid(SuperSlot) && IsValid(ShieldSlot)))
	{
		return;
	}

	PrimarySlot->SetSlotLabel(NSLOCTEXT("DualFireHangar", "PrimarySlot", "PRIMARY"));
	Special1Slot->SetSlotLabel(NSLOCTEXT("DualFireHangar", "Special1Slot", "SPECIAL 1"));
	Special2Slot->SetSlotLabel(NSLOCTEXT("DualFireHangar", "Special2Slot", "SPECIAL 2"));
	SuperSlot->SetSlotLabel(NSLOCTEXT("DualFireHangar", "SuperSlot", "SUPER"));
	ShieldSlot->SetSlotLabel(NSLOCTEXT("DualFireHangar", "ShieldSlot", "SHIELD"));
}

void UDualFireHangarLoadoutPreviewWidget::SetLoadoutItems(
	const TArray<FDualFireHangarItemViewData>& Items)
{
	CommittedItems.Reset();
	for (const FDualFireHangarItemViewData& Item : Items)
	{
		if (Item.IsValid())
		{
			CommittedItems.Add(Item.Category, Item);
		}
	}
	RestoreCommittedItems();
}

void UDualFireHangarLoadoutPreviewWidget::PreviewItem(const FDualFireHangarItemViewData& Item)
{
	RestoreCommittedItems();
	if (Item.IsValid())
	{
		ApplyItem(Item, true);
	}
}

void UDualFireHangarLoadoutPreviewWidget::ApplyItem(
	const FDualFireHangarItemViewData& Item,
	const bool bIsPreview)
{
	switch (Item.Category)
	{
	case EDualFireHangarCategory::Aircraft:
		if (IsValid(AircraftImage))
		{
			// 원본 텍스처 크기를 브러시에 반영해 ScaleBox가 전투기 이미지 비율을 보존하게 한다.
			AircraftImage->SetBrushFromSoftTexture(Item.Icon, true);
		}
		if (IsValid(AircraftName))
		{
			AircraftName->SetText(Item.DisplayName);
		}
		break;
	case EDualFireHangarCategory::PrimaryWeapon:
		PrimarySlot->SetItem(Item, bIsPreview);
		break;
	case EDualFireHangarCategory::SpecialWeapon1:
		Special1Slot->SetItem(Item, bIsPreview);
		break;
	case EDualFireHangarCategory::SpecialWeapon2:
		Special2Slot->SetItem(Item, bIsPreview);
		break;
	case EDualFireHangarCategory::SuperWeapon:
		SuperSlot->SetItem(Item, bIsPreview);
		break;
	case EDualFireHangarCategory::Shield:
		ShieldSlot->SetItem(Item, bIsPreview);
		break;
	default:
		break;
	}
}

void UDualFireHangarLoadoutPreviewWidget::RestoreCommittedItems()
{
	for (const TPair<EDualFireHangarCategory, FDualFireHangarItemViewData>& Pair : CommittedItems)
	{
		ApplyItem(Pair.Value, false);
	}
}

UDualFireHangarItemEntryWidget::UDualFireHangarItemEntryWidget()
{
	Style = UDualFireMenuButtonStyle::StaticClass();
	SetIsSelectable(true);
	SetShouldSelectUponReceivingFocus(true);
	SetIsInteractableWhenSelected(true);
}

void UDualFireHangarItemEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	ItemObject = Cast<UDualFireHangarItemObject>(ListItemObject);
	RefreshEntry();
}

void UDualFireHangarItemEntryWidget::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	if (IsValid(ItemName))
	{
		if (const TSubclassOf<UCommonTextStyle> CurrentTextStyle = GetCurrentTextStyleClass())
		{
			ItemName->SetStyle(CurrentTextStyle);
		}
	}
}

void UDualFireHangarItemEntryWidget::RefreshEntry()
{
	if (!IsValid(ItemObject))
	{
		return;
	}

	if (IsValid(ItemIcon))
	{
		ItemIcon->SetBrushFromSoftTexture(ItemObject->Item.Icon, false);
	}
	if (IsValid(ItemName))
	{
		ItemName->SetText(ItemObject->Item.DisplayName);
	}
	if (IsValid(EquippedIndicator))
	{
		EquippedIndicator->SetVisibility(
			ItemObject->bEquipped ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	NativeOnCurrentTextStyleChanged();
}

UDualFireHangarWidget::UDualFireHangarWidget()
{
	SetFocusedButtonConfirmEnabled(false);
	bIsBackActionDisplayedInActionBar = true;

	DefaultDraftLoadout.AircraftID = TEXT("F22");
	DefaultDraftLoadout.PrimaryWeaponID = TEXT("TEST_AG");
	DefaultDraftLoadout.SpecialWeapon1ID = TEXT("TEST_AA");
	DefaultDraftLoadout.SpecialWeapon2ID = TEXT("TEST_AM");
	DefaultDraftLoadout.SuperWeaponID = TEXT("TEST_SUPER");
	DefaultDraftLoadout.ShieldID = TEXT("TEST_SHIELD");
	MissingItemIcon = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath(TEXT("/Game/UI/Textures/Hangar/T_UI_Hangar_ItemPlaceholder.T_UI_Hangar_ItemPlaceholder")));
}

void UDualFireHangarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ensure(IsValid(LoadoutPreview) && IsValid(ItemList) && IsValid(CategoryTabs) &&
		IsValid(CategoryTitle) && IsValid(StatusText)))
	{
		return;
	}

	ItemList->SetSelectionMode(ESelectionMode::Single);
	ItemList->OnItemClicked().AddUObject(this, &ThisClass::HandleItemClicked);
	ItemList->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleItemSelectionChanged);
	CategoryTabs->OnTabSelected.AddDynamic(this, &ThisClass::HandleTabSelected);
	CategoryTabs->SetListeningForInput(false);

	RegisterInputActions();
}

void UDualFireHangarWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	bSortieRequested = false;
	SetStatus(FText::GetEmpty());
	RegisterCategoryTabs();

	ULoadoutManagerSubsystem* Manager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULoadoutManagerSubsystem>()
		: nullptr;
	if (!IsValid(Manager))
	{
		SetStatus(NSLOCTEXT("DualFireHangar", "ManagerMissing", "LOADOUT DATA IS UNAVAILABLE."));
		return;
	}

	FText ValidationError;
	FName InvalidField;
	const FLoadout& ActiveLoadout = Manager->GetActiveLoadout();
	DraftLoadout = Manager->ValidateLoadout(ActiveLoadout, ValidationError, InvalidField)
		? ActiveLoadout
		: DefaultDraftLoadout;

	RefreshLoadoutPreview();
	CategoryTabs->SelectTabByID(GetCategoryID(EDualFireHangarCategory::Aircraft), true);
}

UWidget* UDualFireHangarWidget::NativeGetDesiredFocusTarget() const
{
	return ItemList;
}

bool UDualFireHangarWidget::NativeOnHandleBackAction()
{
	DeactivateWidget();
	return true;
}

void UDualFireHangarWidget::RegisterCategoryTabs()
{
	if (!IsValid(CategoryButtonClass))
	{
		UE_LOG(LogDualFire, Error, TEXT("[Hangar] CategoryButtonClass is not configured."));
		return;
	}

	CategoryTabs->RemoveAllTabs();
	int32 Index = 0;
	for (const EDualFireHangarCategory Category : GetCategoryOrder())
	{
		const FName CategoryID = GetCategoryID(Category);
		if (!CategoryTabs->RegisterTab(CategoryID, CategoryButtonClass, nullptr, Index++))
		{
			UE_LOG(LogDualFire, Warning, TEXT("[Hangar] Failed to register category tab: %s"), *CategoryID.ToString());
			continue;
		}

		if (UDualFireMenuButton* Button = Cast<UDualFireMenuButton>(CategoryTabs->GetTabButtonBaseByID(CategoryID)))
		{
			// 카테고리는 Hover/포커스가 아니라 클릭 또는 이전/다음 액션으로만 변경한다.
			Button->SetSelectUponFocusEnabled(false);
			if (IsValid(CategoryButtonStyleClass))
			{
				Button->SetStyle(CategoryButtonStyleClass);
			}
			Button->SetLabelText(GetCategoryLabel(Category));
		}
	}
}

void UDualFireHangarWidget::RegisterInputActions()
{
	auto RegisterAction = [this](
		const UInputAction* Action,
		const bool bShowInActionBar,
		const FText& DisplayName,
		const FSimpleDelegate& Callback,
		const int32 ActionPriority)
	{
		if (!IsValid(Action))
		{
			UE_LOG(LogDualFire, Warning, TEXT("[Hangar] An input action is not configured on %s."), *GetName());
			return;
		}

		FBindUIActionArgs BindArgs(Action, bShowInActionBar, Callback);
		BindArgs.OverrideDisplayName = DisplayName;
		BindArgs.PriorityWithinCollection = ActionPriority;
		BindArgs.bConsumeInput = true;
		RegisterUIActionBinding(BindArgs);
	};

	RegisterAction(PreviousCategoryInputAction, true,
		NSLOCTEXT("DualFireHangar", "PreviousSlot", "PREV SLOT"),
		FSimpleDelegate::CreateUObject(this, &ThisClass::ChangeCategory, -1), 10);
	RegisterAction(NextCategoryInputAction, true,
		NSLOCTEXT("DualFireHangar", "NextSlot", "NEXT SLOT"),
		FSimpleDelegate::CreateUObject(this, &ThisClass::ChangeCategory, 1), 20);
	RegisterAction(ConfirmInputAction, true,
		NSLOCTEXT("DualFireHangar", "Equip", "EQUIP"),
		FSimpleDelegate::CreateUObject(this, &ThisClass::EquipFocusedItem), 30);
	RegisterAction(SortieInputAction, true,
		NSLOCTEXT("DualFireHangar", "Sortie", "SORTIE"),
		FSimpleDelegate::CreateUObject(this, &ThisClass::Sortie), 40);
}

void UDualFireHangarWidget::HandleTabSelected(const FName TabID)
{
	EDualFireHangarCategory Category;
	if (!TryGetCategory(TabID, Category))
	{
		return;
	}

	ActiveCategory = Category;
	CategoryTitle->SetText(GetCategoryLabel(Category));
	RefreshItemList();
}

void UDualFireHangarWidget::ChangeCategory(const int32 Direction)
{
	const TArray<EDualFireHangarCategory>& Categories = GetCategoryOrder();
	const int32 CurrentIndex = Categories.IndexOfByKey(ActiveCategory);
	if (CurrentIndex == INDEX_NONE || Categories.IsEmpty())
	{
		return;
	}

	const int32 NewIndex = (CurrentIndex + Direction + Categories.Num()) % Categories.Num();
	CategoryTabs->SelectTabByID(GetCategoryID(Categories[NewIndex]));
}

void UDualFireHangarWidget::RefreshItemList()
{
	TArray<FDualFireHangarItemViewData> Items;
	BuildItemsForCategory(ActiveCategory, Items);

	TArray<TObjectPtr<UDualFireHangarItemObject>> VisibleItems;
	for (const FDualFireHangarItemViewData& Item : Items)
	{
		UDualFireHangarItemObject* ItemObject = NewObject<UDualFireHangarItemObject>(this);
		ItemObject->Initialize(Item, Item.ItemID == GetDraftItemID(ActiveCategory));
		VisibleItems.Add(ItemObject);
	}

	ItemList->SetListItems(VisibleItems);
	if (VisibleItems.IsEmpty())
	{
		SetStatus(NSLOCTEXT("DualFireHangar", "NoItems", "NO ITEMS ARE AVAILABLE FOR THIS SLOT."));
		return;
	}

	int32 SelectedIndex = VisibleItems.IndexOfByPredicate([](const UDualFireHangarItemObject* Item)
	{
		return IsValid(Item) && Item->bEquipped;
	});
	if (SelectedIndex == INDEX_NONE)
	{
		SelectedIndex = 0;
	}

	ItemList->SetSelectedIndex(SelectedIndex);
	ItemList->NavigateToIndex(SelectedIndex);
	SetStatus(FText::GetEmpty());
}

void UDualFireHangarWidget::BuildItemsForCategory(
	const EDualFireHangarCategory Category,
	TArray<FDualFireHangarItemViewData>& OutItems) const
{
	OutItems.Reset();
	const ULoadoutManagerSubsystem* Manager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULoadoutManagerSubsystem>()
		: nullptr;
	if (!IsValid(Manager))
	{
		return;
	}

	auto FinalizeItem = [this, Category, &OutItems](
		const FName ItemID,
		const FText& DisplayName,
		const TSoftObjectPtr<UTexture2D>& Icon)
	{
		FDualFireHangarItemViewData Item;
		Item.Category = Category;
		Item.ItemID = ItemID;
		Item.DisplayName = ResolveDisplayName(DisplayName, ItemID);
		Item.Icon = Icon.IsNull() ? MissingItemIcon : Icon;
		OutItems.Add(Item);
	};

	if (Category == EDualFireHangarCategory::Aircraft && IsValid(Manager->AircraftDataTable))
	{
		for (const FName RowName : Manager->AircraftDataTable->GetRowNames())
		{
			if (const FAircraftRow* Row = FindTypedRow<FAircraftRow>(Manager->AircraftDataTable, RowName, TEXT("HangarAircraft")))
			{
				FinalizeItem(Row->AircraftID.IsNone() ? RowName : Row->AircraftID, Row->DisplayName, Row->Icon);
			}
		}
	}
	else if (IsValid(Manager->WeaponDataTable) && (Category == EDualFireHangarCategory::PrimaryWeapon ||
		Category == EDualFireHangarCategory::SpecialWeapon1 ||
		Category == EDualFireHangarCategory::SpecialWeapon2))
	{
		const EWeaponCategory RequiredCategory = Category == EDualFireHangarCategory::PrimaryWeapon
			? EWeaponCategory::Primary
			: EWeaponCategory::Special;
		for (const FName RowName : Manager->WeaponDataTable->GetRowNames())
		{
			if (const FWeaponRow* Row = FindTypedRow<FWeaponRow>(Manager->WeaponDataTable, RowName, TEXT("HangarWeapon"));
				Row && Row->Category == RequiredCategory)
			{
				FinalizeItem(Row->WeaponID.IsNone() ? RowName : Row->WeaponID, Row->DisplayName, Row->Icon);
			}
		}
	}
	else if (Category == EDualFireHangarCategory::SuperWeapon && IsValid(Manager->SuperWeaponDataTable))
	{
		for (const FName RowName : Manager->SuperWeaponDataTable->GetRowNames())
		{
			if (const FSuperWeaponRow* Row = FindTypedRow<FSuperWeaponRow>(Manager->SuperWeaponDataTable, RowName, TEXT("HangarSuperWeapon")))
			{
				FinalizeItem(Row->SuperWeaponID.IsNone() ? RowName : Row->SuperWeaponID, Row->DisplayName, Row->Icon);
			}
		}
	}
	else if (Category == EDualFireHangarCategory::Shield && IsValid(Manager->ShieldDataTable))
	{
		for (const FName RowName : Manager->ShieldDataTable->GetRowNames())
		{
			if (const FShieldRow* Row = FindTypedRow<FShieldRow>(Manager->ShieldDataTable, RowName, TEXT("HangarShield")))
			{
				FinalizeItem(Row->ShieldID.IsNone() ? RowName : Row->ShieldID, Row->DisplayName, Row->Icon);
			}
		}
	}

	OutItems.Sort([](const FDualFireHangarItemViewData& Left, const FDualFireHangarItemViewData& Right)
	{
		const int32 NameOrder = Left.DisplayName.ToString().Compare(Right.DisplayName.ToString(), ESearchCase::IgnoreCase);
		return NameOrder == 0 ? Left.ItemID.LexicalLess(Right.ItemID) : NameOrder < 0;
	});
}

bool UDualFireHangarWidget::ResolveItem(
	const EDualFireHangarCategory Category,
	const FName ItemID,
	FDualFireHangarItemViewData& OutItem) const
{
	TArray<FDualFireHangarItemViewData> Items;
	BuildItemsForCategory(Category, Items);
	if (const FDualFireHangarItemViewData* Found = Items.FindByPredicate([ItemID](const FDualFireHangarItemViewData& Item)
	{
		return Item.ItemID == ItemID;
	}))
	{
		OutItem = *Found;
		return true;
	}
	return false;
}

void UDualFireHangarWidget::RefreshLoadoutPreview(const FDualFireHangarItemViewData* PreviewItem)
{
	TArray<FDualFireHangarItemViewData> EquippedItems;
	for (const EDualFireHangarCategory Category : GetCategoryOrder())
	{
		FDualFireHangarItemViewData Item;
		if (ResolveItem(Category, GetDraftItemID(Category), Item))
		{
			EquippedItems.Add(Item);
		}
	}

	LoadoutPreview->SetLoadoutItems(EquippedItems);
	if (PreviewItem)
	{
		LoadoutPreview->PreviewItem(*PreviewItem);
	}
}

void UDualFireHangarWidget::EquipFocusedItem()
{
	EquipItem(ItemList->GetSelectedItem<UDualFireHangarItemObject>());
}

void UDualFireHangarWidget::EquipItem(UDualFireHangarItemObject* ItemObject)
{
	if (!IsValid(ItemObject))
	{
		return;
	}

	SetDraftItemID(ItemObject->Item.Category, ItemObject->Item.ItemID);
	SetStatus(FText::GetEmpty());
	RefreshLoadoutPreview();
	RefreshItemList();
}

void UDualFireHangarWidget::HandleItemClicked(UObject* Item)
{
	EquipItem(Cast<UDualFireHangarItemObject>(Item));
}

void UDualFireHangarWidget::HandleItemSelectionChanged(UObject* Item)
{
	if (const UDualFireHangarItemObject* ItemObject = Cast<UDualFireHangarItemObject>(Item))
	{
		RefreshLoadoutPreview(&ItemObject->Item);
	}
}

void UDualFireHangarWidget::Sortie()
{
	if (bSortieRequested)
	{
		return;
	}

	ULoadoutManagerSubsystem* Manager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULoadoutManagerSubsystem>()
		: nullptr;
	if (!IsValid(Manager))
	{
		SetStatus(NSLOCTEXT("DualFireHangar", "ManagerMissingSortie", "LOADOUT DATA IS UNAVAILABLE."));
		return;
	}

	FText Error;
	FName InvalidField;
	UDualFireMissionFlowSubsystem* Flow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDualFireMissionFlowSubsystem>()
		: nullptr;
	if (!IsValid(Flow) || !Flow->TryFinalizeLaunch(DraftLoadout, Error, InvalidField))
	{
		SetStatus(Error);
		SelectInvalidField(InvalidField);
		return;
	}

	const FMissionLaunchContext Launch = Flow->GetLaunchContext();
	if (Launch.Preparation.MissionLevel.IsNull())
	{
		SetStatus(NSLOCTEXT("DualFireHangar", "MissionMissing", "MISSION LEVEL IS NOT CONFIGURED."));
		return;
	}

	bSortieRequested = true;
	UE_LOG(LogDualFire, Log, TEXT("[Hangar] Sortie confirmed. Aircraft=%s"), *DraftLoadout.AircraftID.ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Launch.Preparation.MissionLevel);
}

void UDualFireHangarWidget::SetStatus(const FText& Message)
{
	if (!IsValid(StatusText))
	{
		return;
	}
	StatusText->SetText(Message);
	StatusText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UDualFireHangarWidget::SelectInvalidField(const FName InvalidField)
{
	EDualFireHangarCategory Category;
	if (TryGetCategory(InvalidField, Category))
	{
		CategoryTabs->SelectTabByID(GetCategoryID(Category));
	}
}

FName UDualFireHangarWidget::GetDraftItemID(const EDualFireHangarCategory Category) const
{
	switch (Category)
	{
	case EDualFireHangarCategory::Aircraft: return DraftLoadout.AircraftID;
	case EDualFireHangarCategory::PrimaryWeapon: return DraftLoadout.PrimaryWeaponID;
	case EDualFireHangarCategory::SpecialWeapon1: return DraftLoadout.SpecialWeapon1ID;
	case EDualFireHangarCategory::SpecialWeapon2: return DraftLoadout.SpecialWeapon2ID;
	case EDualFireHangarCategory::SuperWeapon: return DraftLoadout.SuperWeaponID;
	case EDualFireHangarCategory::Shield: return DraftLoadout.ShieldID;
	default: return NAME_None;
	}
}

void UDualFireHangarWidget::SetDraftItemID(
	const EDualFireHangarCategory Category,
	const FName ItemID)
{
	switch (Category)
	{
	case EDualFireHangarCategory::Aircraft: DraftLoadout.AircraftID = ItemID; break;
	case EDualFireHangarCategory::PrimaryWeapon: DraftLoadout.PrimaryWeaponID = ItemID; break;
	case EDualFireHangarCategory::SpecialWeapon1: DraftLoadout.SpecialWeapon1ID = ItemID; break;
	case EDualFireHangarCategory::SpecialWeapon2: DraftLoadout.SpecialWeapon2ID = ItemID; break;
	case EDualFireHangarCategory::SuperWeapon: DraftLoadout.SuperWeaponID = ItemID; break;
	case EDualFireHangarCategory::Shield: DraftLoadout.ShieldID = ItemID; break;
	default: break;
	}
}
