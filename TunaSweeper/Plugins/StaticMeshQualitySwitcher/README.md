# Static Mesh Quality Switcher

Editor-only Unreal Engine plugin that switches placed `UStaticMeshComponent` instances between strictly paired original and low-quality meshes.

## Setup

1. Enable the plugin and restart the editor.
2. Create a **Static Mesh Quality Profile** from **Miscellaneous > Data Asset**.
3. Add one original mesh and one dedicated low mesh to every pair.
4. Open **TunaSweeper > Static Mesh Quality Switcher**.
5. Select the profile, choose the scope, validate, and apply Original or Low.

The profile rejects null entries and any mesh asset reused in another slot, including cross-role reuse. These rules also participate in Unreal's Content Browser asset validation. Application is aborted before modifying the level if validation or asset loading fails. Successful changes participate in editor Undo/Redo and leave affected level packages dirty for explicit saving.

The Loaded Level scope only processes actors currently loaded in the editor. World Partition cells that are not loaded must be loaded before applying the switch.
