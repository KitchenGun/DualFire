// Copyright DualFire. All Rights Reserved.

#include "UI/DualFireHangarTypes.h"

void UDualFireHangarItemObject::Initialize(
	const FDualFireHangarItemViewData& InItem,
	const bool bInEquipped)
{
	Item = InItem;
	bEquipped = bInEquipped;
}
