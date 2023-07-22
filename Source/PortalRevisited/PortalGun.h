// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <optional>

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "PortalGun.generated.h"

class APortalRevisitedCharacter;
class UInputMappingContext;
class UInputAction;
class APortal;
class AStaticMeshActor;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Portal), meta=(BlueprintSpawnableComponent))
class PORTALREVISITED_API UPortalGun : public USkeletalMeshComponent
{
	GENERATED_BODY()

	using PortalCenterAndNormal = std::optional<std::pair<FVector, FQuat>>;
	using PortalOffset = std::optional<FVector>;
public:
	UPortalGun();

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<USoundBase> FireSound;
	
	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UAnimMontage> FireAnimation;
	
	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Portal")
	TObjectPtr<APortal> BluePortal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Portal")
	TObjectPtr<APortal> OrangePortal;


	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction;

	UFUNCTION(BlueprintCallable, Category="PortalGun")
	void AttachPortalGun(APortalRevisitedCharacter* TargetCharacter);
	FVector GetPortalUpVector() const;
	FVector GetPortalRightVector() const;
	FVector GetPortalForwardVector() const;
	FVector GetPortalUpVector(const FQuat& PortalRotation) const;
	FVector GetPortalRightVector(const FQuat& PortalRotation) const;
	FVector GetPortalForwardVector(const FQuat& PortalRotation) const;
	FQuat CalculatePortalRotation(const FVector& ImpactNormal) const;
	void MovePortal(const FVector& ImpactPoint, const FVector& ImpactNormal);
	void DestroyAllPlanesSpawnedBefore();
	void SpawnPlanesAroundPortal();
	FVector CalculateOffset(
		const FVector& PortalForward, 
		const FVector PortalRight, 
		const FVector PortalUp, 
		const FVector U, 
		const double Delta) const;

	PortalCenterAndNormal CalculateCorrectPortalCenter(const FHitResult& HitResult) const;
	PortalOffset MovePortalUAxisAligned(
		const FVector& BoundCenter, 
		const FVector& BoundExtent, 
		const FVector& PortalForward, 
		const FVector& PortalRight,
		const FVector& PortalUp,
		const FVector& PortalPoint,
		const FVector& U) const;
	UFUNCTION(BlueprintCallable, Category="PortalGun")
	void FireBlue();

	UFUNCTION(BlueprintCallable, Category="PortalGun")
	void FireOrange();


private:
	/** The Character holding this weapon*/
	TObjectPtr<APortalRevisitedCharacter> Character;
	
	TObjectPtr<UStaticMesh> PlaneMesh;

	TArray<TObjectPtr<AStaticMeshActor>> BluePortalPlanes;
	TArray<TObjectPtr<AStaticMeshActor>> OrangePortalPlanes;
};
