// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <optional>

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Portal.generated.h"

#define PORTAL_COLLISION_PROFILE_NAME "Pawn_Hole"
#define STANDARD_COLLISION_PROFILE_NAME "Pawn"

class UStaticMeshComponent;
class UCapsuleComponent;
class UArrowComponent;
class UPortalGun;
class UWallDissolver;

DECLARE_LOG_CATEGORY_EXTERN(Portal, Log, All);

UCLASS()
class PORTALREVISITED_API APortal : public AStaticMeshActor
{
	GENERATED_BODY()


public:
	
	/** Sound to play in ambient of portal.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<USoundBase> AmbientSound;
	
	/** Sound to play if the actor pass the portal.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<USoundBase> EnterSound;

	/**
	 * 
	 */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	UStaticMeshComponent* MeshPortalHole;
	/**
	 * If a dynamic mesh has overlapped this mask, then collision channel
	 * will be changed to enter the portal ignoring static mesh behind.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UCapsuleComponent> PortalEnterMask;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> PortalPlane;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> PortalInner;
	
	TObjectPtr<UTextureRenderTarget2D> PortalTexture;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USceneCaptureComponent2D> PortalCamera;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UWallDissolver> WallDissolver;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAudioComponent> AmbientSoundComponent;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	void UnregisterOverlappingActor(TObjectPtr<AActor> Actor, UPrimitiveComponent* Component);

	UFUNCTION()
	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// Sets default values for this actor's properties
	APortal();

	void LinkPortals(TObjectPtr<APortal> NewTarget);
	void RegisterPortalGun(TObjectPtr<UPortalGun> NewPortalGun);
	void SetPortalTexture(TObjectPtr<UTextureRenderTarget2D> NewTexture);

private:
	TObjectPtr<UBlueprint> CharacterBlueprint;

	TObjectPtr<APortal> LinkedPortal;
	uint8 PortalStencilValue;

	bool bIsActivated;

	TArray<TObjectPtr<AActor>> OverlappingActors;
	TArray<TObjectPtr<AActor>> IgnoredActors;
	TMap<TObjectPtr<AActor>, TObjectPtr<AActor>> CloneMap;
	bool bStopRegistering;
	TObjectPtr<UPortalGun> PortalGun;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void RegisterOverlappingActor(TObjectPtr<AActor> Actor, TObjectPtr<UPrimitiveComponent> Component);
	void Activate();
	void Deactivate();
	void SetMeshesVisibility(bool bNewVisibility);

	FVector GetPortalUpVector() const;
	FVector GetPortalRightVector() const;
	FVector GetPortalForwardVector() const;
	FVector GetPortalUpVector(const FQuat& PortalRotation) const;
	FVector GetPortalRightVector(const FQuat& PortalRotation) const;
	FVector GetPortalForwardVector(const FQuat& PortalRotation) const;
	FVector GetPortalPlaneLocation() const;
	uint8 GetPortalCustomStencilValue() const;

	TObjectPtr<APortal> GetLink() const;
	
	void SetPortalCustomStencilValue(uint8 NewValue);
	void AddIgnoredActor(TObjectPtr<AActor> Actor);
	void RemoveIgnoredActor(TObjectPtr<AActor> Actor);

	std::optional<TObjectPtr<AActor>> GetOriginalIfClone(AActor* Actor);
	
	FVector TransformVectorToDestSpace(const FVector& Target);
	static FVector TransformVectorToDestSpace(
		const FVector& Target, 
		const APortal& SrcPortal, 
		const APortal& DestPortal);
	/**
	 * Transform the given vector by using given Portals' coordinates.
	 * [dest portal basis] * [src portal basis]^-1 * [target vector] = [result]
	 */
	static FVector TransformVectorToDestSpace(
		const FVector& Target,
		const FVector& SrcPortalForward,
		const FVector& SrcPortalRight,
		const FVector& SrcPortalUp,
		const FVector& DestPortalForward,
		const FVector& DestPortalRight,
		const FVector& DestPortalUp);
	FVector TransformPointToDestSpace(const FVector& Target);
	static FVector TransformPointToDestSpace(
		const FVector& Target, 
		const APortal& SrcPortal, 
		const APortal& DestPortal);
	/**
	 * Transform the given point by using given Portals' coordinates.
	 *
	 */
	static FVector TransformPointToDestSpace(
		const FVector& Target, 
		const FVector& SrcPortalPos,
		const FVector& SrcPortalForward,
		const FVector& SrcPortalRight,
		const FVector& SrcPortalUp,
		const FVector& DestPortalPos,
		const FVector& DestPortalForward,
		const FVector& DestPortalRight,
		const FVector& DestPortalUp);
	FQuat TransformQuatToDestSpace(const FQuat& Target);
	static FQuat TransformQuatToDestSpace(
		const FQuat& Target, 
		const APortal& SrcPortal, 
		const APortal& DestPortal);
	/**
	 * Transform the given quaternion by using given Portals' quaternion.
	 * 
	 */
	static FQuat TransformQuatToDestSpace(
		const FQuat& Target,
		const FQuat& SrcPortalQuat,
		const FQuat& DestPortalQuat,
		const FVector DestPortalUp);
	static bool IsPointInFrontOfPortal(
		const FVector& Point,
		const FVector& PortalPos,
		const FVector& PortalNormal);
	
	static std::optional<TObjectPtr<APortal>> CastPortal(AActor* Actor);

private:
	void InitMeshPortalHole();
	void InitPortalEnterMask();
	void InitPortalPlane();
	void InitPortalInner();
	void InitPortalCamera();
	void InitWallDissolver();
	void InitAmbientSoundComponent();
	
	void UpdateClones();
	void UpdateCaptureCamera();
	void UpdateCapture();
	void CheckAndTeleportOverlappingActors();
	void PlaySoundAtLocation(USoundBase* SoundToPlay, FVector Location);
	void TeleportActor(AActor& Actor);
	void RemoveClone(TObjectPtr<AActor> Actor);

};
