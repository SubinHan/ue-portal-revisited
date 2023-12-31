// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WallDissolver.generated.h"

/**
 * 
 */
UCLASS()
class PORTALREVISITED_API UWallDissolver : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UWallDissolver();

	void SetDissolverName(FString Name);
	FString GetDissolverName();
	void UpdateParameters(const FVector RootLocation);

private:
	TObjectPtr<UMaterialParameterCollection> MatParamCollection;
	FString DissolverName;
};
