// Copyright Epic Games, Inc. All Rights Reserved.

#include "MakeDumplingCommands.h"

#define LOCTEXT_NAMESPACE "FMakeDumplingModule"

void FMakeDumplingCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "MakeDumpling", "Bring up MakeDumpling window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
