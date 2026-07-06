// Copyright DualFire. All Rights Reserved.

#include "Core/UassetDiffLibrary.h"

#include "Engine/Level.h"
#include "Misc/PackageName.h"
#include "Misc/PackagePath.h"
#include "Misc/Paths.h"
#include "UObject/LinkerInstancingContext.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

UPackage* UUassetDiffLibrary::LoadPackageForDiff(const FString& DiskPath, const FString& OriginalPackageName)
{
#if WITH_EDITOR
	if (!FPaths::FileExists(DiskPath))
	{
		return nullptr;
	}

	// DiffUtils::LoadPackageForDiff(UnrealEd)와 동일한 로드 경로.
	// 마운트 밖 로컬 경로는 /Temp/<드라이브 상대 경로> 패키지명으로 매핑된다.
	const FPackagePath TempPackagePath = FPackagePath::FromLocalPath(DiskPath);

	// 마운트된 경로(예: /Game 하위)면 일반 로드로 충분
	if (!FPackageName::IsTempPackage(TempPackagePath.GetPackageName()))
	{
		return LoadPackage(nullptr, *TempPackagePath.GetPackageName(), LOAD_None);
	}

	// 패키지 내부 자기참조(원본 패키지명으로 저장돼 있음)를 임시 패키지로 리매핑 —
	// 없으면 자기참조가 라이브 원본 에셋으로 해석돼 비교가 오염된다
	FLinkerInstancingContext Context;
	if (!OriginalPackageName.IsEmpty())
	{
		Context.AddTag(ULevel::DontLoadExternalObjectsTag);
		Context.AddPackageMapping(FName(*OriginalPackageName), TempPackagePath.GetPackageFName());
	}

	return LoadPackage(nullptr, *TempPackagePath.GetPackageName(),
		LOAD_ForDiff | LOAD_DisableCompileOnLoad | LOAD_DisableEngineVersionChecks,
		/*InReaderOverride*/ nullptr, &Context);
#else
	return nullptr;
#endif
}

UObject* UUassetDiffLibrary::FindAssetInPackage(UPackage* Package)
{
	if (!Package)
	{
		return nullptr;
	}

	UObject* FoundAsset = nullptr;
	ForEachObjectWithPackage(Package, [&FoundAsset](UObject* Object)
	{
		if (Object->IsAsset())
		{
			FoundAsset = Object;
			return false; // 순회 중단
		}
		return true;
	}, EGetObjectsFlags::None);

	return FoundAsset;
}
