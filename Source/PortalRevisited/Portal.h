// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Portal.generated.h"

class UStaticMeshComponent;
class UCapsuleComponent;
class UArrowComponent;

DECLARE_LOG_CATEGORY_EXTERN(Portal, Error, Error);

UCLASS()
class PORTALREVISITED_API APortal : public AStaticMeshActor
{
	GENERATED_BODY()


public:
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
	TObjectPtr<UStaticMeshComponent> PortalInner;
	
	TObjectPtr<UTextureRenderTarget2D> PortalTexture;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USceneCaptureComponent2D> PortalCamera;
	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// Sets default values for this actor's properties
	APortal();

	void LinkPortal(TObjectPtr<APortal> NewTarget);
	void SetPortalTexture(TObjectPtr<UTextureRenderTarget2D> NewTexture);

private:
	TObjectPtr<APortal> LinkedPortal;
	uint8 PortalStencilValue;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	FVector GetPortalUpVector() const;
	FVector GetPortalRightVector() const;
	FVector GetPortalForwardVector() const;
	FVector GetPortalUpVector(const FQuat& PortalRotation) const;
	FVector GetPortalRightVector(const FQuat& PortalRotation) const;
	FVector GetPortalForwardVector(const FQuat& PortalRotation) const;
	FVector GetPortalPlaneLocation() const;
	uint8 GetPortalCustomStencilValue() const;
	
	void SetPortalCustomStencilValue(uint8 NewValue);

private:
	void InitMeshPortalHole();
	void InitPortalEnterMask();
	void InitPortalInner();
	void InitPortalCamera();

	void UpdateCaptureCamera();
	void UpdateCapture();

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
		const FVector& DestPortalUp
	);

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
