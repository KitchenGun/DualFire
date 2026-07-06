// Copyright DualFire. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UassetDiffLibrary.generated.h"

/**
 * uasset 리비전 비교(Content/Python/uasset_diff.py) 지원용 에디터 유틸리티.
 *
 * 에디터 내장 리비전 컨트롤 diff(DiffUtils::LoadPackageForDiff)와 동일하게,
 * Content 밖 임의 디스크 경로의 .uasset을 LOAD_ForDiff로 메모리에만 로드한다.
 * 에셋 레지스트리 등록·Content 복사가 없어 프로젝트를 오염시키지 않는다.
 *
 * 에디터 전용 동작 — 비에디터 빌드에서는 nullptr을 반환한다.
 */
UCLASS()
class DUALFIRE_API UUassetDiffLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 디스크 경로의 uasset을 비교 전용(LOAD_ForDiff | LOAD_DisableCompileOnLoad)으로 로드.
	 * DiffUtils::LoadPackageForDiff와 동일 구현 — 마운트되지 않은 로컬 경로는 /Temp/
	 * 패키지명으로 매핑되어 라이브 에셋과 충돌하지 않고, OriginalPackageName을 주면
	 * 패키지 내부 자기참조를 임시 패키지로 리매핑해 라이브 에셋 오염을 막는다.
	 * 반환 패키지는 참조가 끊기면 GC로 정리된다.
	 *
	 * @param DiskPath            이전 리비전 .uasset 파일의 절대경로 (git show 추출본 등)
	 * @param OriginalPackageName 원본 에셋의 패키지 경로 (예: "/Game/Blueprint/BP_Test"). 빈 값 허용
	 */
	UFUNCTION(BlueprintCallable, Category="UassetDiff")
	static UPackage* LoadPackageForDiff(const FString& DiskPath, const FString& OriginalPackageName);

	/** 패키지 안의 주 에셋(UBlueprint 등)을 반환. 없으면 nullptr */
	UFUNCTION(BlueprintCallable, Category="UassetDiff")
	static UObject* FindAssetInPackage(UPackage* Package);
};
