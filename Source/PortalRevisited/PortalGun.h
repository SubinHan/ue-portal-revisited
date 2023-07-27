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
	void LinkPortal();

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
	FQuat CalculatePortalRotation(const FVector& ImpactNormal, const APortal& TargetPortal) const;
	void MovePortal(const FVector& ImpactPoint, const FVector& ImpactNormal, const APortal& TargetPortal);
	void DestroyAllPlanesSpawnedBefore();
	void SpawnPlanesAroundPortal();
	FVector CalculateOffset(
		const FVector& PortalForward, 
		const FVector PortalRight, 
		const FVector PortalUp, 
		const FVector U, 
		const double Delta) const;

	PortalCenterAndNormal CalculateCorrectPortalCenter(const FHitResult& HitResult, const APortal& TargetPortal) const;
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
	void StopGrabbing();
	void StartGrabbing(AActor* NewGrabbedActor);

	UFUNCTION(BlueprintCallable, Category="PortalGun")
	void Interact();

	virtual void PostInitProperties() override;
	bool CanGrab(AActor* Actor);
	UPrimitiveComponent* GetPrimitiveComponent(TObjectPtr<AActor>);
	void GrabObject();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** The Character holding this weapon*/
	TObjectPtr<APortalRevisitedCharacter> Character;
	
	TObjectPtr<UStaticMesh> PlaneMesh;

	TArray<TObjectPtr<AStaticMeshActor>> BluePortalPlanes;
	TArray<TObjectPtr<AStaticMeshActor>> OrangePortalPlanes;

	bool bIsGrabbing;
	TObjectPtr<AActor> GrabbedActor;
};
