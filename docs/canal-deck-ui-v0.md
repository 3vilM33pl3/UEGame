# Deck UI v0 (M1)

`UCanalDeckMenuWidget` and `ACanalDeckUiActor` provide a Deck-first control menu for:

- `Generate`
- `Scenario`
- `Start Capture`
- `Stop Capture`

The menu is C++-driven and can run without authoring a UMG Blueprint first.

## Files

- `Source/UEGame/Public/CanalGen/CanalDeckMenuWidget.h`
- `Source/UEGame/Private/CanalGen/CanalDeckMenuWidget.cpp`
- `Source/UEGame/Public/CanalGen/CanalDeckUiActor.h`
- `Source/UEGame/Private/CanalGen/CanalDeckUiActor.cpp`

## Runtime Setup

1. Place `ACanalDeckUiActor` in the level.
2. Keep `bCreateOnBeginPlay=true`.
3. Assign `ContextActor` to an actor that has:
   - `UCanalSeedSessionComponent`
   - `UCanalScenarioRunnerComponent`
4. Press Play.

The widget can also auto-resolve these components from player/controller/world actors.

## Controls

- `Generate`: uses manual seed input when provided, otherwise generates a new seed.
- `Scenario`: runs scenario lifecycle with current seed.
- `Start Capture`: starts scenario capture flow.
- `Stop Capture`: stops active capture flow.

## Handheld Readability

- Button text defaults to large font (`ButtonFontSize=30`).
- Buttons use large minimum heights (`MinimumButtonHeight=82`).
- Layout targets 1280x800 handheld readability with centered panel and large touch/gamepad targets.

## No-Keyboard Input

- Buttons are focusable and compatible with gamepad navigation.
- `ACanalDeckUiActor` sets `GameAndUI` input mode on show, so interaction works without keyboard.
