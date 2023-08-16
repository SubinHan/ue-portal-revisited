// Fill out your copyright notice in the Description page of Project Settings.

#include "PortalClipLocation.h"

#include "DebugHelper.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "PortalRevisited/Portal.h"

template<class T>
using Asset = ConstructorHelpers::FObjectFinder<T>;

const FString PARAMETER_NAME_LEFTUP("LeftUpClipLocation");
const FString PARAMETER_NAME_LEFTDOWN("LeftDownClipLocation");
const FString PARAMETER_NAME_RIGHTUP("RightUpClipLocation");
const FString PARAMETER_NAME_RIGHTDOWN("RightDownClipLocation");

const FString PARAMETER_NAME_FRONT("Front");
const FString PARAMETER_NAME_BACK("Back");

UPortalClipLocation::UPortalClipLocation()
{
	Asset<UMaterialParameterCollection> MatParamCollectionAsset(
		TEXT("/Script/Engine.MaterialParameterCollection'/Game/MPC_Portal.MPC_Portal'"));
	
	if (MatParamCollectionAsset.Object)
	{
		MatParamCollection = MatParamCollectionAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Error, TEXT("MPC_Portal cannot be found."));
	}
}

UPortalClipLocation::~UPortalClipLocation()
{
}

void NormalizeToZeroOne(UE::Math::TVector4<double>& Vector)
{
	Vector.X += 1.0;
	Vector.Y += 1.0;

	Vector.X /= 2.0;
	Vector.Y /= 2.0;
}

void OneMinus(double& Value)
{
	Value = 1.0 - Value;
}

void CalculateClipSpaceLocation(const FMatrix& ViewProjectionMatrix, APortal* PortalToDraw, UE::Math::TVector4<double>& ClipLeftUp, UE::Math::TVector4<double>& ClipLeftDown, UE::Math::TVector4<double>& ClipRightUp, UE::Math::TVector4<double>& ClipRightDown)
{
	const auto PortalCenter = 
		PortalToDraw->GetPortalPlaneLocation();
	const auto PortalRight =
		PortalToDraw->GetPortalRightVector();
	const auto PortalUp =
		PortalToDraw->GetPortalUpVector();

	// TODO: Refactor hard coded portal size
	constexpr double PORTAL_HEIGHT_HALF = 150.0;
	constexpr double PORTAL_WIDTH_HALF = 100.0;

	const auto WorldLeftUp =
		PortalCenter + 
		PortalUp * PORTAL_HEIGHT_HALF - 
		PortalRight * PORTAL_WIDTH_HALF;

	const auto WorldLeftDown =
		PortalCenter -
		PortalUp * PORTAL_HEIGHT_HALF - 
		PortalRight * PORTAL_WIDTH_HALF;

	const auto WorldRightUp =
		PortalCenter + 
		PortalUp * PORTAL_HEIGHT_HALF +
		PortalRight * PORTAL_WIDTH_HALF;
	
	const auto WorldRightDown =
		PortalCenter -
		PortalUp * PORTAL_HEIGHT_HALF +
		PortalRight * PORTAL_WIDTH_HALF;

	ClipLeftUp = ViewProjectionMatrix.TransformPosition(WorldLeftUp);

	ClipLeftDown = ViewProjectionMatrix.TransformPosition(WorldLeftDown);

	ClipRightUp = ViewProjectionMatrix.TransformPosition(WorldRightUp);

	ClipRightDown = ViewProjectionMatrix.TransformPosition(WorldRightDown);

	ClipLeftUp /= ClipLeftUp.W;
	ClipLeftDown /= ClipLeftDown.W;
	ClipRightUp /= ClipRightUp.W;
	ClipRightDown /= ClipRightDown.W;

	NormalizeToZeroOne(ClipLeftUp);
	NormalizeToZeroOne(ClipLeftDown);
	NormalizeToZeroOne(ClipRightUp);
	NormalizeToZeroOne(ClipRightDown);

	OneMinus(ClipLeftUp.Y);
	OneMinus(ClipLeftDown.Y);
	OneMinus(ClipRightUp.Y);
	OneMinus(ClipRightDown.Y);

	ClipLeftUp.X = FMath::Clamp(ClipLeftUp.X, 0.0f, 1.0f);
	ClipLeftDown.X = FMath::Clamp(ClipLeftDown.X, 0.0f, 1.0f);
	ClipRightUp.X = FMath::Clamp(ClipRightUp.X, 0.0f, 1.0f);
	ClipRightDown.X = FMath::Clamp(ClipRightDown.X, 0.0f, 1.0f);
	
	ClipLeftUp.Y = FMath::Clamp(ClipLeftUp.Y, 0.0f, 1.0f);
	ClipLeftDown.Y = FMath::Clamp(ClipLeftDown.Y, 0.0f, 1.0f);
	ClipRightUp.Y = FMath::Clamp(ClipRightUp.Y, 0.0f, 1.0f);
	ClipRightDown.Y = FMath::Clamp(ClipRightDown.Y, 0.0f, 1.0f);
}

void UPortalClipLocation::UpdateBackPortalClipLocation(
	const FMatrix& ViewProjectionMatrix, 
	APortal* PortalToDraw)
{
	const auto MatParamCollectionInstance =
		GetWorld()->GetParameterCollectionInstance(MatParamCollection);

	UE::Math::TVector4<double> ClipLeftUp;
	UE::Math::TVector4<double> ClipLeftDown;
	UE::Math::TVector4<double> ClipRightUp;
	UE::Math::TVector4<double> ClipRightDown;
	CalculateClipSpaceLocation(
		ViewProjectionMatrix,
		PortalToDraw, 
		ClipLeftUp, 
		ClipLeftDown,
		ClipRightUp, 
		ClipRightDown);

	const auto LeftUpParameterName =
		PARAMETER_NAME_BACK + PARAMETER_NAME_LEFTUP;
	
	const auto LeftDownParameterName =
		PARAMETER_NAME_BACK + PARAMETER_NAME_LEFTDOWN;
	
	const auto RightUpParameterName =
		PARAMETER_NAME_BACK + PARAMETER_NAME_RIGHTUP;
	
	const auto RightDownParameterName =
		PARAMETER_NAME_BACK + PARAMETER_NAME_RIGHTDOWN;
	
	MatParamCollectionInstance->SetVectorParameterValue(
		FName(LeftUpParameterName),
		ClipLeftUp);

	MatParamCollectionInstance->SetVectorParameterValue(
		FName(LeftDownParameterName),
		ClipLeftDown);
	
	MatParamCollectionInstance->SetVectorParameterValue(
		FName(RightUpParameterName),
		ClipRightUp);

	MatParamCollectionInstance->SetVectorParameterValue(
		FName(RightDownParameterName),
		ClipRightDown);
}

void UPortalClipLocation::UpdateFrontPortalClipLocation(
	const FMatrix& ViewProjectionMatrix, 
	APortal* PortalToDraw)
{
	const auto MatParamCollectionInstance =
		GetWorld()->GetParameterCollectionInstance(MatParamCollection);

	UE::Math::TVector4<double> ClipLeftUp;
	UE::Math::TVector4<double> ClipLeftDown;
	UE::Math::TVector4<double> ClipRightUp;
	UE::Math::TVector4<double> ClipRightDown;
	CalculateClipSpaceLocation(
		ViewProjectionMatrix,
		PortalToDraw, 
		ClipLeftUp, 
		ClipLeftDown,
		ClipRightUp, 
		ClipRightDown);

	const auto LeftUpParameterName =
		PARAMETER_NAME_FRONT + PARAMETER_NAME_LEFTUP;
	
	const auto LeftDownParameterName =
		PARAMETER_NAME_FRONT + PARAMETER_NAME_LEFTDOWN;
	
	const auto RightUpParameterName =
		PARAMETER_NAME_FRONT + PARAMETER_NAME_RIGHTUP;
	
	const auto RightDownParameterName =
		PARAMETER_NAME_FRONT + PARAMETER_NAME_RIGHTDOWN;

	MatParamCollectionInstance->SetVectorParameterValue(
		FName(LeftUpParameterName),
		ClipLeftUp);

	MatParamCollectionInstance->SetVectorParameterValue(
		FName(LeftDownParameterName),
		ClipLeftDown);
	
	MatParamCollectionInstance->SetVectorParameterValue(
		FName(RightUpParameterName),
		ClipRightUp);

	MatParamCollectionInstance->SetVectorParameterValue(
		FName(RightDownParameterName),
		ClipRightDown);
}
