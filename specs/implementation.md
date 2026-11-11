# Hints Framework: behavior and implementation

Describes the exported implementation **0.2.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Provides namespaced contextual controls in the native bottom item legend. Up to 32 providers publish up to eight controls, tied to current-frame game-thread/world/focus admission. Consumers clear only their stable key; stale unpublished hints expire. The separate presentation service admits up to 32 resident late callbacks so managed/native providers can publish at the correct presentation stage without replacing the ordinary hints ABI.

## State and lifecycle

Provider keys, callback handles and active hints are process/session state, not saves. Busy dispatch refuses registry mutation/reentry; unsubscribe must succeed before consumer callbacks/context can be released. World/focus/menu and legend gates control actual display. Managed wrappers bind after native readiness and keep delegates rooted while subscribed.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/hints-framework/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Provides `hints-framework.hints` | 1 | 24 bytes |
| Provides `hints-framework.presentation` | 1 | 24 bytes |

## Implementation sources

- [mod.c](../source/mods/hints-framework/mod.c)
- [registry.h](../source/mods/hints-framework/registry.h)
- [presentation_registry.h](../source/mods/hints-framework/presentation_registry.h)
- [presentation_adapter.h](../source/mods/hints-framework/presentation_adapter.h)

## Verification boundary

The framework does not create a permanent HUD or authorize arbitrary Unity calls from managed Update. Controller layout, loaded-game callback ordering and each consumer require matching runtime qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
