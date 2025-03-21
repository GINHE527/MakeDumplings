// Copyright Epic Games, Inc. All Rights Reserved.

#include "MakeDumpling.h"
#include "MakeDumplingStyle.h"
#include "MakeDumplingCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName MakeDumplingTabName("MakeDumpling");

#define LOCTEXT_NAMESPACE "FMakeDumplingModule"

void FMakeDumplingModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FMakeDumplingStyle::Initialize();
	FMakeDumplingStyle::ReloadTextures();

	FMakeDumplingCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FMakeDumplingCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FMakeDumplingModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMakeDumplingModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MakeDumplingTabName, FOnSpawnTab::CreateRaw(this, &FMakeDumplingModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FMakeDumplingTabTitle", "MakeDumpling"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FMakeDumplingModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FMakeDumplingStyle::Shutdown();

	FMakeDumplingCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MakeDumplingTabName);
}

TSharedRef<SDockTab> FMakeDumplingModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FMakeDumplingModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("MakeDumpling.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FMakeDumplingModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(MakeDumplingTabName);
}

void FMakeDumplingModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FMakeDumplingCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FMakeDumplingCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMakeDumplingModule, MakeDumpling)