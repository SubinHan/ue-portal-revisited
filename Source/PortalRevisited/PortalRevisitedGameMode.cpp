// Copyright Epic Games, Inc. All Rights Reserved.

#include "PortalRevisitedGameMode.h"
#include "PortalRevisitedCharacter.h"
#include "UObject/ConstructorHelpers.h"

APortalRevisitedGameMode::APortalRevisitedGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
