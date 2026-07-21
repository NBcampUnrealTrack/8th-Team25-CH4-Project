// FDItemInventoryWidget.h

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FDItemInventoryWidget.generated.h"

class UFDItemInventoryComponent;

UCLASS()
class FUNNYORDIE_API UFDItemInventoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "Item")
    void InitializeWithInventory(UFDItemInventoryComponent* InInventory);
    
};