#include "GuitarPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Instrument/GuitarInstrumentComponent.h"
#include "UI/GuitarScreenWidget.h"

namespace
{
	// 根音动作数组依次对应 C、D、E、F、G、A、B。
	constexpr int32 RootPitchClasses[] = {0, 2, 4, 5, 7, 9, 11};
}

void AGuitarPlayerController::BindGuitarActions(UEnhancedInputComponent& EnhancedInputComponent)
{
	// 根音按下时选和弦，释放或取消时撤销该根音。
	for (int32 Index = 0; Index < ChordRootActions.Num(); ++Index)
	{
		EnhancedInputComponent.BindAction(ChordRootActions[Index], ETriggerEvent::Started,
			this, &AGuitarPlayerController::ChooseChordRoot, RootPitchClasses[Index]);
		EnhancedInputComponent.BindAction(ChordRootActions[Index], ETriggerEvent::Completed,
			this, &AGuitarPlayerController::ReleaseChordRoot, RootPitchClasses[Index]);
		EnhancedInputComponent.BindAction(ChordRootActions[Index], ETriggerEvent::Canceled,
			this, &AGuitarPlayerController::ReleaseChordRoot, RootPitchClasses[Index]);
	}

	for (int32 Index = 0; Index < ChordTypeActions.Num(); ++Index)
	{
		EnhancedInputComponent.BindAction(ChordTypeActions[Index], ETriggerEvent::Started,
			this, &AGuitarPlayerController::ChooseChordType, static_cast<EGuitarChordType>(Index));
	}

	for (int32 StringIndex = 0; StringIndex < PluckStringActions.Num(); ++StringIndex)
	{
		EnhancedInputComponent.BindAction(PluckStringActions[StringIndex], ETriggerEvent::Started,
			this, &AGuitarPlayerController::PluckString, StringIndex);
	}
}

void AGuitarPlayerController::ChooseChordType(EGuitarChordType Type)
{
	if (CanPlayGuitar())
	{
		PendingChordType = Type;
		if (GuitarScreen) GuitarScreen->SetChordType(Type);
	}
}

void AGuitarPlayerController::ChooseChordRoot(int32 RootPitchClass)
{
	APawn* ControlledPawn = GetPawn();
	if (!CanPlayGuitar() || !ControlledPawn)
	{
		return;
	}

	// 拨弦前的状态同步与 Started 回调可能发生在同一帧，只记录一次按下快照。
	if (HeldChords.ContainsByPredicate(
		[RootPitchClass](const FHeldChord& Held) { return Held.RootPitchClass == RootPitchClass; }))
	{
		return;
	}

	UGuitarInstrumentComponent* Instrument = ControlledPawn->FindComponentByClass<UGuitarInstrumentComponent>();
	const UEnhancedPlayerInput* EnhancedPlayerInput = Cast<UEnhancedPlayerInput>(PlayerInput);
	if (!Instrument || !EnhancedPlayerInput)
	{
		return;
	}

	// 类型与根音同帧按下时，先读取映射中的新按键，不依赖动作回调顺序。
	for (const FEnhancedActionKeyMapping& Mapping : GuitarMappingContext->GetMappings())
	{
		if (!WasInputKeyJustPressed(Mapping.Key)) continue;
		const int32 TypeIndex = ChordTypeActions.IndexOfByKey(Mapping.Action);
		if (TypeIndex != INDEX_NONE) ChooseChordType(static_cast<EGuitarChordType>(TypeIndex));
	}

	// 直接读取本帧已求值的修饰动作，避免自行维护按下/释放状态。
	const bool bSharp = EnhancedPlayerInput->GetActionValue(ChordSharpAction).Get<bool>();
	const bool bFlat = EnhancedPlayerInput->GetActionValue(ChordFlatAction).Get<bool>();
	if (bSharp && bFlat)
	{
		return;
	}

	int32 PitchClass = RootPitchClass;
	if (bSharp)
	{
		++PitchClass;
	}
	else if (bFlat)
	{
		--PitchClass;
	}
	const FName ChordId = Guitar::MakeChordId(PitchClass, PendingChordType);
	HeldChords.Add({ RootPitchClass, ChordId });
	Instrument->SelectChord(ChordId);
}

void AGuitarPlayerController::ReleaseChordRoot(int32 RootPitchClass)
{
	const int32 HeldIndex = HeldChords.IndexOfByPredicate(
		[RootPitchClass](const FHeldChord& Held) { return Held.RootPitchClass == RootPitchClass; });
	if (HeldIndex == INDEX_NONE)
	{
		return;
	}

	const bool bWasActive = HeldIndex == HeldChords.Num() - 1;
	HeldChords.RemoveAt(HeldIndex);
	if (!bWasActive)
	{
		return;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UGuitarInstrumentComponent* Instrument = ControlledPawn->FindComponentByClass<UGuitarInstrumentComponent>())
		{
			if (HeldChords.IsEmpty())
			{
				Instrument->ClearChord();
			}
			else
			{
				Instrument->SelectChord(HeldChords.Last().ChordId);
			}
		}
	}
}

void AGuitarPlayerController::ClearHeldChords()
{
	HeldChords.Reset();
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UGuitarInstrumentComponent* Instrument = ControlledPawn->FindComponentByClass<UGuitarInstrumentComponent>())
		{
			Instrument->ClearChord();
		}
	}
}

void AGuitarPlayerController::PluckString(int32 StringIndex)
{
	APawn* ControlledPawn = GetPawn();
	if (!CanPlayGuitar() || !ControlledPawn)
	{
		return;
	}

	// 根音与拨弦回调可能交叉执行；先同步动作本帧已求值的按下/释放状态。
	// Choose/Release 均可重复调用，不会重排已有按下快照或误清除其他根音。
	if (const UEnhancedPlayerInput* EnhancedPlayerInput = Cast<UEnhancedPlayerInput>(PlayerInput))
	{
		for (int32 Index = 0; Index < ChordRootActions.Num(); ++Index)
		{
			const FInputActionInstance* Action = EnhancedPlayerInput->FindActionInstanceData(ChordRootActions[Index]);
			if (!Action)
			{
				continue;
			}
			const ETriggerEvent Event = Action->GetTriggerEvent();
			if (Event == ETriggerEvent::Completed || Event == ETriggerEvent::Canceled)
			{
				ReleaseChordRoot(RootPitchClasses[Index]);
			}
			else if (Event == ETriggerEvent::Started || Event == ETriggerEvent::Ongoing
				|| Event == ETriggerEvent::Triggered)
			{
				ChooseChordRoot(RootPitchClasses[Index]);
			}
		}
	}

	if (UGuitarInstrumentComponent* Instrument = ControlledPawn->FindComponentByClass<UGuitarInstrumentComponent>())
	{
		Instrument->PluckString(StringIndex);
	}
}
