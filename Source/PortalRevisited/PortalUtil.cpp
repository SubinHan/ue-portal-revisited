#include "PortalUtil.h"

std::optional<UPrimitiveComponent*> GetPrimitiveComponent(TObjectPtr<AActor> Actor)
{
	auto result = static_cast<UPrimitiveComponent*>(
		Actor->GetComponentByClass(
			UPrimitiveComponent::StaticClass()));

	if (result)
		return result;

	return std::nullopt;
}