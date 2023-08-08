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
	void LinkPortals();

	/** Sound to play each time we fire blue portal*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<USoundBase> BlueFireSound;

	/** Sound to play each time we fire orange portal*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<USoundBase> OrangeFireSound;
	
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

	TObjectPtr<UTextureRenderTarget2D> BluePortalTexture;
	TObjectPtr<UTextureRenderTarget2D> OrangePortalTexture;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> PortalGunMappingContext;

	/** Fire Blue Portal Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireBlueAction;
	
	/** Fire Orange Portal Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireOrangeAction;

	/** Grab Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> GrabAction;

	UFUNCTION(BlueprintCallable, Category="PortalGun")
	void AttachPortalGun(APortalRevisitedCharacter* TargetCharacter);
	void PlaySoundAtLocation(USoundBase* SoundToPlay, FVector Location);
	void PlayFiringAnimation();

	UFUNCTION(BlueprintCallable, Category="PortalGun")
	void FireBlue();

	UFUNCTION(BlueprintCallable, Category="PortalGun")
	void FireOrange();

	UFUNCTION(BlueprintCallable, Category="PortalGun")
	void Interact();

	void ResetPortal();

	void OnActorPassedPortal(
		TObjectPtr<APortal> PassedPortal,
		TObjectPtr<AActor> PassingActor);

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	void FirePortal(TObjectPtr<APortal> TargetPortal);
	/**
	 * @brief 
	 * @param The actor like walls 
	 * @return true if the portal can be placed on the actor.
	 */
	bool CanPlacePortal(UPhysicalMaterial* WallPhysicalMaterial);
	void SpawnPlanesAroundPortal(TObjectPtr<APortal> TargetPortal);
	
	TArray<TObjectPtr<AStaticMeshActor>>& GetCollisionPlanes(
		TObjectPtr<APortal> TargetPortal);

	void DestroyAllPlanesSpawnedBefore(
		TArray<TObjectPtr<AStaticMeshActor>>& TargetCollisionPlanes);

	PortalCenterAndNormal CalculateCorrectPortalCenter(
		const FHitResult& HitResult, 
		const APortal& TargetPortal) const;
	
	FQuat CalculatePortalRotation(
		const FVector& ImpactNormal,
		const APortal& TargetPortal) const;

	PortalOffset MovePortalUAxisAligned(
		const FVector& BoundCenter, 
		const FVector& BoundExtent, 
		const FVector& PortalForward, 
		const FVector& PortalRight,
		const FVector& PortalUp,
		const FVector& PortalPoint,
		const FVector& U) const;

	FVector CalculateOffset(
		const FVector& PortalForward, 
		const FVector PortalRight, 
		const FVector PortalUp, 
		const FVector U, 
		const double Delta) const;

	void StopGrabbing();
	void StartGrabbing(AActor* NewGrabbedActor);
	bool CanGrab(AActor* Actor);
	std::optional<TObjectPtr<AActor>> GetOriginalIfClone(AActor* Actor);
	std::optional<TObjectPtr<APortal>> GetPortalInFrontOfCharacter();

	void ForceGrabbedObject();

private:
	/** The Character holding this weapon*/
	TObjectPtr<APortalRevisitedCharacter> Character;
	
	TObjectPtr<UStaticMesh> PlaneMesh;

	TArray<TObjectPtr<AStaticMeshActor>> BluePortalPlanes;
	TArray<TObjectPtr<AStaticMeshActor>> OrangePortalPlanes;

	bool bIsGrabbing;
	bool bIsGrabbedObjectAcrossedPortal;
	TObjectPtr<AActor> GrabbedActor;
};
