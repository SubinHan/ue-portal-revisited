// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PortalClipLocation.generated.h"

class APortal;

/**
 * 
 */
UCLASS()
class PORTALREVISITED_API UPortalClipLocation : public USceneComponent
{
	GENERATED_BODY()


public:
	UPortalClipLocation();
	~UPortalClipLocation();
	void UpdateBackPortalClipLocation(const FMatrix& Matrix, APortal* PortalToDraw);
	void UpdateFrontPortalClipLocation(const FMatrix& ViewProjectionMatrix, APortal* PortalToDraw);
	static bool CannotSeePortal(const FMatrix& ViewProjectionMatrix, APortal* PortalToDraw);

private:
	TObjectPtr<UMaterialParameterCollection> MatParamCollection;
};
