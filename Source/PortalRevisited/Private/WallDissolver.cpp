// Fill out your copyright notice in the Description page of Project Settings.


#include "WallDissolver.h"

#include <stdexcept>

#include "DebugHelper.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"

template<class T>
using Asset = ConstructorHelpers::FObjectFinder<T>;

const FString PARAMETER_NAME_LOCATION("Location");
const FString PARAMETER_NAME_SCALE("Scale");
const FString PARAMETER_NAME_ROTATION("Rotation");

UWallDissolver::UWallDissolver()
{
	Asset<UMaterialParameterCollection> MatParamCollectionAsset(
		TEXT("MaterialParameterCollection'/Game/MPC_Dissolve_Area.MPC_Dissolve_Area'"));

	if (MatParamCollectionAsset.Object)
	{
		MatParamCollection = MatParamCollectionAsset.Object;
	}
}

void UWallDissolver::SetDissolverName(FString Name)
{
	DissolverName = Name;
}

FString UWallDissolver::GetDissolverName()
{
	return DissolverName;
}

/**
 * Update material parameter collection.
 * It updates location and scale parameter, named by
 * "Location" + DissolverName and "Scale" + DissolverName.
 */
void UWallDissolver::UpdateParameters(const FVector RootLocation)
{
	auto MatParamCollectionInstance =
		GWorld->GetParameterCollectionInstance(MatParamCollection);

	auto LocationParameterName =
		PARAMETER_NAME_LOCATION + DissolverName;

	MatParamCollectionInstance->SetVectorParameterValue(
		FName(LocationParameterName),
		RootLocation);
	
	auto ScaleParameterName =
		PARAMETER_NAME_SCALE + DissolverName;

	MatParamCollectionInstance->SetVectorParameterValue(
		FName(ScaleParameterName), 
		GetComponentScale());
	
	auto RotationParameterName =
		PARAMETER_NAME_ROTATION + DissolverName;

	MatParamCollectionInstance->SetVectorParameterValue(
		FName(RotationParameterName),
		GetComponentRotation().Euler());
}
