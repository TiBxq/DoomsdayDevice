# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DoomsdayDevice is an Unreal Engine 5.8 first-person narrative game. It started from the UE First Person template and borrows systems from the [FlowGame sample](https://github.com/MothCocoon/FlowGame) — many gameplay files carry that copyright header. The engine is installed at `D:\unreal\UE_5.8`.

## Commands

There is one runtime C++ module, `DoomsdayDevice`. No automation tests exist in the project (`Automation_DoomsdayDevice.sln` is just the generated UAT tooling solution).

```powershell
# Build the editor target (Development Editor | Win64)
& "D:\unreal\UE_5.8\Engine\Build\BatchFiles\Build.bat" DoomsdayDeviceEditor Win64 Development -Project="D:\projects\DoomsdayDevice\DoomsdayDevice.uproject" -WaitMutex

# Launch the editor
& "D:\unreal\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "D:\projects\DoomsdayDevice\DoomsdayDevice.uproject"

# Regenerate VS project files (after adding/removing source files).
# GenerateProjectFiles.bat does not exist in this engine install, and UnrealBuildTool.exe
# needs the engine-bundled .NET 10 runtime (system dotnet is too old):
cmd /c "call ""D:\unreal\UE_5.8\Engine\Build\BatchFiles\GetDotnetPath.bat"" && ""D:\unreal\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"" -ProjectFiles -Project=""D:\projects\DoomsdayDevice\DoomsdayDevice.uproject"" -Game"
```

Build.bat requires the editor closed. While the editor is running, C++ changes — including brand-new UCLASSes — can instead be compiled in with Live Coding: run the `LiveCoding.Compile` console command in the editor and watch the log for "Live coding succeeded". The patch lasts only for that editor session; run Build.bat later for durable binaries.

The engine's experimental `ModelContextProtocol`, `Terminal`, and `AllToolsets` plugins are enabled, so a running editor can be driven over MCP. `.mcp.json` points at the in-editor server (`http://127.0.0.1:8000/mcp`, only up while the editor runs). It exposes three meta-tools — `list_toolsets`, `describe_toolset`, `call_tool` — wrapping toolsets such as `SceneTools` (find/spawn/save actors), `ObjectTools` (get/set properties; params are `instance` + `properties`), `EditorAppToolset` (PIE control, viewport capture), and `LogsToolset`. Quirks: `find_actors` requires `name`/`tag`/`collision_channels` even when unused (pass `""`, `""`, `[]`); on one-file-per-actor levels `save_actor` fails for newly spawned actors — use `AssetTools.save_assets` with an empty list instead; and `ObjectTools.set_properties` false-succeeds on deprecated properties (e.g. `UInputMappingContext::Mappings` — UE 5.8 reads only `DefaultKeyMappings.Mappings`), so verify struct-array writes visually via the asset editor, not by reading them back.

## Architecture

The game is script-driven through the **Flow plugin** (a source checkout at `Plugins/FlowGraph-5.x`, module name `Flow`). Level scripting lives in Flow graph assets (`Content/_Doomsaday/FlowAssets/FA_Main`), not in Blueprint level scripts. Most gameplay code exists to expose world systems to Flow graphs.

**Gameplay tags are the universal addressing scheme.** Actors are identified to Flow graphs, narrative facts are keyed, items are identified, and screens/lights are addressed — all by tags registered in `Config/DefaultGameplayTags.ini` (the `Flow.*` namespace: `Flow.Facts.*`, `Flow.Interaction.*`, `Flow.Items.*`, `Flow.Lights.*`, `Flow.Screens.*`). Register new tags there.

### Player stack (root of `Source/DoomsdayDevice/`)

The active player classes are `ADoomsdayDeviceCharacter` and `ADoomsdayDevicePlayerController` (used with the Blueprint game mode `Content/FirstPerson/Blueprints/BP_FirstPersonGameMode`, the project's global default):

- `ADoomsdayDeviceCharacter` — first-person `ACharacter` with camera/arms mesh, Enhanced Input movement, and the player's `UFlowComponent` (its identity tags are how Flow graphs recognize the player, e.g. in trigger nodes). Also owns the two "hands" systems, which are mutually exclusive: heavy-item carry (`StartCarry`/`IsCarrying`; carried actor attaches to `CarryAttachPoint`) and tool equip (see Tools below; `StartCarry` auto-unequips the tool, `ToggleToolSlot` refuses while carrying).
- `ADoomsdayDevicePlayerController` — manages input mapping contexts, tracks nearby `UInteractionComponent`s via their static `OnPlayerEnter`/`OnPlayerExit` delegates and activates the best one by line trace (opening/closing the `InteractionWidget` "Use" prompt), gates use through the tool requirement (see Tools below), handles the tool slot hotkeys, and turns dialogue input (continue + four choice actions) into `ContinueDialogueEvent`/`SelectDialogueChoiceEvent` delegates consumed by the dialogue system.

**Input keys 1–4 are deliberately double-mapped**: `IA_DialogueChoice1-4` (in `IMC_Default`) and `IA_ToolSlot1-4` (in `IMC_Tools`) share keys One–Four, with both contexts added at priority 0. All eight input actions must keep `bConsumeInput = false` — if either set consumes, the other silently never fires. Exclusivity lives in C++ instead: `OnToolSlotPressed` early-outs while the dialogue widget is open; choice selection is a no-op with no pending choices.

Note: the FlowGame-derived alternatives in `Player/` and `Gameplay/` (`ABasicPlayerController`, `APlayerPawn`, `AMoverPawn`, `UTaggedInputComponent`) are **not** the active player stack — don't extend them for player-facing changes.

### Domain folders (`Public/`/`Private/` mirrored)

- **Flow/** — the narrative backbone.
  - `Nodes/` — custom `UFlowNode` subclasses: dialogue (`FlowNode_DialogueLine`, `_Choice`, `_EndDialogue`), facts (`_AddFact`, `_CompareFact`), interactions (`_OnInteractionUsed`, `_SetInteractionState`), items (`_OnItemCollected` — waits for the player to collect an item; checks `UInventorySubsystem` retroactively on Start, then subscribes to its `OnItemCollected` delegate), triggers (`_OnTriggerEnter/Exit/Event`), spawning (`_SpawnByActorReference`, `_SpawnByGameplayTag`), and `_CustomCheckpoint`. World-observing nodes extend `UFlowNode_ComponentObserver` and find actors by the gameplay tags on their `UFlowComponent`.
  - `Triggers/` — `UFlowTriggerComponent` (a `UFlowComponent` bridging overlap events to the graph) plus ready-made trigger actors/volumes.
  - `FactsDBSubsystem` — GameInstance subsystem holding narrative state as `TMap<FGameplayTag, int32>`; queried/mutated by the fact nodes.
  - `FlowSaveSubsystem` — checkpoint save/load (marked work-in-progress).
- **Gameplay/** — `UInteractionComponent` (world interactable with `Distance` and enabled state; fires `OnUsed`), `USpawnComponent` (spawn point consumed by the spawn Flow nodes), and the item-pickup system:
  - `UInventorySubsystem` — GameInstance subsystem holding collected items as `TMap<FGameplayTag, int32>` counts (`CollectItem`/`HasItem`/`GetItemCount`, native `OnItemCollected` delegate). Mirrors `FactsDBSubsystem` but for `Flow.Items.*` tags.
  - `UPickupComponent` — `UInteractionComponent` subclass (starts enabled, unlike the base). On use: `CollectItem(ItemTag, Count)`, then `Disable()`, then destroys its owner. The `Disable()`-before-`Destroy()` order is load-bearing: `ADoomsdayDevicePlayerController::PlayerTick` sorts interaction candidates through unchecked weak pointers and will crash on a destroyed-but-still-listed component.
  - `APickupItem` — ready-made collectible actor (engine cube mesh + `UFlowComponent` + `UPickupComponent`), CDO-defaulted to `Flow.Items.Test.Cube`; override the tags per instance for real items.
  - **Tool gating**: `UInteractionComponent::RequiredToolTag` (a `Flow.Items.Tools.*` tag). `ADoomsdayDevicePlayerController::OnInteractionUsed` checks `IsToolRequirementMet(EquippedToolTag)` (hierarchical `MatchesTag`; empty requirement always passes) — on failure it broadcasts the component's `OnUseDenied` plus the controller's `OnInteractionUseDenied(RequiredToolTag)` instead of `OnUsed`. Gated interactables still highlight and show the "Use" prompt; **nothing subscribes to the deny events yet**, so there is no blocked-state UI (deferred until the interaction-indications rework).
- **Tools (equippable items on hotkeys 1–4)** — extends the pickup/inventory systems; no separate "tool inventory" exists:
  - Slots are static and designer-defined: `UPlayerSettings::ToolSlots` (`TArray<FToolSlotDefinition>`: `ToolTag`, `ToolActorClass`, `Icon`, `DisplayName`), configured in `Config/DefaultGame.ini`; slot index = hotkey − 1.
  - A slot is "unlocked" iff `UInventorySubsystem::HasItem(ToolTag)` — tools are ordinary items collected via `UPickupComponent` (set the pickup's `ItemTag` to the tool tag), so `FlowNode_OnItemCollected` works for tools unchanged and no unlock state is duplicated.
  - Equip state lives on `ADoomsdayDeviceCharacter`: `ToggleToolSlot(int32)` (same slot toggles off, other slot switches), `UnequipTool()`, `GetEquippedToolSlot()`/`GetEquippedToolTag()` (INDEX_NONE/empty tag = bare hands). `AToolActor`s (root `USceneComponent` + OnlyOwnerSee first-person static mesh; the mesh's relative transform IS the grip offset) are lazily spawned owner=character, attached to `FirstPersonMesh` socket `HandGrip_R`, and hidden/unhidden on switch — never destroyed until `EndPlay`.
  - HUD: `UToolSlotsWidget` C++ base (pull full state in `NativeConstruct`, deltas pushed via `UBasicUIManager::NotifyToolSlotUnlocked`/`NotifyEquippedToolChanged`; idempotent setters make pull+push safe). `Content/UI/Tools/WBP_ToolSlots` is a minimal test scaffold (numbered labels, yellow = selected) meant to be replaced using the `BP_*` events + `GetToolSlotDefinition`.
  - Flow nodes for tools don't exist yet; the deny/equip delegates are shaped so observers can bind like `FlowNode_OnInteractionUsed` binds `OnUsed`.
- **UI/** + **Dialogue/** — `UBasicUIManager` (LocalPlayer subsystem; opens/closes widgets by soft class, displays dialogue lines and choices fed by the dialogue Flow nodes), `UDialogSpeakerDataAsset` for speaker metadata. Widget classes come from `UPlayerSettings`, a `UDeveloperSettings` configured under `[/Script/DoomsdayDevice.PlayerSettings]` in `Config/DefaultGame.ini`.

### Content

Game content lives in `Content/_Doomsaday/` (note the intentional "Doomsaday" spelling — keep it). `Maps/Level_Main` is both the editor startup and game default map; the `Lvl_*` maps are its layered sublevels (Blockout, Environment, Audio, lighting, PostProcess, Stars). Dialogue assets are in `_Doomsaday/Story/`.

The `Variant_Horror`/`Variant_Shooter` source folders and their Content counterparts are First Person template variants, separate from the main game. `Content/FirstPerson/Lvl_FirstPerson_TestFlow` is a sandbox map for testing Flow-driven gameplay; it contains placed `APickupItem`s: `TestPickup_Cube` (plain pickup), `ToolPickup_Test` (`ItemTag = Flow.Items.Tools.Test`, unlocks tool slot 1), and `GatedPickup_Test` (`RequiredToolTag = Flow.Items.Tools.Test` — only collectible with the test tool equipped). Tool assets live in `Content/_Doomsaday/Tools/` (`BP_Tool_Test`) and `Content/Input/` (`IA_ToolSlot1-4`, `IMC_Tools`).
