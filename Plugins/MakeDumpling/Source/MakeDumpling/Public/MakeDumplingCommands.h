// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "MakeDumplingStyle.h"

class FMakeDumplingCommands : public TCommands<FMakeDumplingCommands>
{
public:

	FMakeDumplingCommands()
		: TCommands<FMakeDumplingCommands>(TEXT("MakeDumpling"), NSLOCTEXT("Contexts", "MakeDumpling", "MakeDumpling Plugin"), NAME_None, FMakeDumplingStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};