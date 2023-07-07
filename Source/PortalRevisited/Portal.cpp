// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"

#include <stdexcept>

// Sets default values
APortal::APortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame




void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector APortal::TransformVectorToDestSpace(
	const FVector& Target, 
	const FVector& SrcPortalFront,
	const FVector& SrcPortalRight, 
	const FVector& SrcPortalUp,
	const FVector& DestPortalFront,
	const FVector& DestPortalRight, 
	const FVector& DestPortalUp)
{
	FVector Coordinate;
	Coordinate.X = FVector::DotProduct(Target, SrcPortalFront);
	Coordinate.Y = FVector::DotProduct(Target, SrcPortalRight);
	Coordinate.Z = FVector::DotProduct(Target, SrcPortalUp);
	
	return Coordinate.X * DestPortalFront +
		Coordinate.Y * DestPortalRight +
		Coordinate.Z * DestPortalUp;
}

FVector APortal::TransformPointToDestSpace(
	const FVector& Target,
	const FVector& SrcPortalPos,
	const FVector& SrcPortalFront,
	const FVector& SrcPortalRight, 
	const FVector& SrcPortalUp,
	const FVector& DestPortalPos, 
	const FVector& DestPortalFront, 
	const FVector& DestPortalRight,
	const FVector& DestPortalUp)
{
	const FVector SrcToTarget = Target - SrcPortalPos;

	const FVector DestToTarget = TransformVectorToDestSpace(
		SrcToTarget,
		SrcPortalFront,
		SrcPortalRight,
		SrcPortalUp,
		DestPortalFront,
		DestPortalRight,
		DestPortalUp
	);
	
	return DestPortalPos + DestToTarget;
}

FQuat APortal::TransformQuatToDestSpace(
	const FQuat& Target,
	const FQuat& SrcPortalQuat, 
	const FQuat& DestPortalQuat,
	const FVector DestPortalUp)
{
	const FQuat Diff = DestPortalQuat * SrcPortalQuat.Inverse();
	FQuat Result = Diff * Target;

	// Turn 180 degrees.
	// Multiply the up axis pure quaternion by sin(90) (real part is cos(90))
	FQuat Rotator = FQuat(DestPortalUp.X, DestPortalUp.Y, DestPortalUp.Z, 0);

	return Rotator * Result;
}

bool APortal::IsPointInFrontOfPortal(const FVector& Point, const FVector& PortalPos, const FVector& PortalNormal)
{
	const FVector PointToPortal = PortalPos - Point;
	return FVector::DotProduct(PointToPortal, PortalNormal) < 0.0;
}
