// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PortalGunHUD.generated.h"

/**
 * 
 */
UCLASS()
class PORTALREVISITED_API UPortalGunHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Update that the blue portal is exists or not. */
	void SetBluePortalStatus(bool bIsExists);

	/** Update that the orange portal is exists or not. */
	void SetOrangePortalStatus(bool bIsExists);


};
