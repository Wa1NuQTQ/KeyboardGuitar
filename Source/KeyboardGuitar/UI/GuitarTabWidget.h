#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Instrument/GuitarTypes.h"
#include "GuitarTabWidget.generated.h"

class UGuitarPracticeSessionComponent;

// 绘制下方横向六线谱，仅读取会话，不参与判定
UCLASS()
class KEYBOARDGUITAR_API UGuitarTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(UGuitarPracticeSessionComponent* InSession);
	void SetChordType(EGuitarChordType Type) { ChordType = Type; }
	void SetPlayedChord(FName ChordId) { PlayedChord = ChordId; }

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGuitarPracticeSessionComponent> Session;
	EGuitarChordType ChordType = EGuitarChordType::Major;
	FName PlayedChord;
};
