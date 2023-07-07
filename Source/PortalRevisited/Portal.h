// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <stdexcept>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

UCLASS()
class PORTALREVISITED_API APortal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/**
	 * Transform the given vector by using given Portals' coordinates.
	 * [dest portal basis] * [src portal basis]^-1 * [target vector] = [result]
	 */
	static FVector TransformVectorToDestSpace(
		const FVector& Target,
		const FVector& SrcPortalFront,
		const FVector& SrcPortalRight,
		const FVector& SrcPortalUp,
		const FVector& DestPortalFront,
		const FVector& DestPortalRight,
		const FVector& DestPortalUp
	);

	/**
	 * Transform the given point by using given Portals' coordinates.
	 *
	 */
	static FVector TransformPointToDestSpace(
		const FVector& Target, 
		const FVector& SrcPortalPos,
		const FVector& SrcPortalFront,
		const FVector& SrcPortalRight,
		const FVector& SrcPortalUp,
		const FVector& DestPortalPos,
		const FVector& DestPortalFront,
		const FVector& DestPortalRight,
		const FVector& DestPortalUp
	);

	/**
	 * Transform the given quaternion by using given Portals' quaternion.
	 * 
	 */
	static FQuat TransformQuatToDestSpace(
		const FQuat& Target,
		const FQuat& SrcPortalQuat,
		const FQuat& DestPortalQuat,
		const FVector DestPortalUp
	);

	static bool IsPointInFrontOfPortal(
		const FVector& Point,
		const FVector& PortalPos,
		const FVector& PortalNormal
	);
};