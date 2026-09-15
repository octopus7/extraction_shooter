# STOVE PC SDK integration

## Local inputs

- SDK: `store/stove/StovePCSDK_Studio_Cpp_3.4.2/` (extract the supplied ZIP here).
- Settings: `store/stove/credentials.env`; keep this file and SDK distribution ignored by Git.
- `STOVE_GAME_ID` and `STOVE_APPLICATION_KEY` are required by PC SDK 3.4.2.
- `STOVE_PRODUCT_NO` and `STOVE_APPLICATION_SECRET` remain local administrative information. They are not embedded or staged by this integration.
- Use credentials issued for the selected product: demo and full targets do not automatically select different credentials files.

## Build and runtime

Only Win64 game targets `TunaSweeperStove` and `TunaSweeperStoveDemo` compile the SDK integration. Editor, Steam and NoStore builds do not initialize or link STOVE.

UBT validates the local settings and generates a private header under `TunaSweeper/Intermediate/Stove/<Target>/`. The required game ID/application key are compiled into the client; no credential values are supplied through compiler flags or tracked config. The `.env` file and Application Secret are not packaged.

At module startup, the game checks launcher startup, initializes BaseSDK, initializes OwnershipSDK and checks the exact game ID and acquired ownership. Both BASIC (3) and DEMO (4) are accepted; launcher developer mode was observed returning BASIC for the demo game ID. DLC and unknown types are rejected. Build flavor does not determine the SDK ownership type. Failure or a 90-second timeout exits before world loading. A launcher-requested restart exits the original process. Successful startup pumps SDK callbacks on the game thread and uninitializes OwnershipSDK before BaseSDK on exit. The SDK environment is `LIVE`.

Empty ownership lists are reported separately from rejected entries. Rejected-entry diagnostics show only ID match status, product type and ownership state; they do not expose game IDs or credentials. For Studio games, the matching local `dev_game_exhibit_list` entry uses `is_studio: true`; restart the launcher after editing its policy. A policy entry is not proof of production ownership.

This adds launch/ownership integration. STOVE platform achievements, billing and other optional modules are not connected; existing local achievement persistence remains unchanged.

## Packaging

- Demo: `TunaSweeper/BatchScripts/PackageTunaSweeperStoveDemoWin64.bat Shipping`
- Full: `TunaSweeper/BatchScripts/PackageTunaSweeperStoveFullWin64.bat Shipping`
- The existing editor Build Target menu also selects these targets.
- Output: `TunaSweeper/Builds/Stove/<Demo|Full>/Windows/`.
- `bMakeBinaryConfig=True` is required in the project packaging settings. UAT bakes the selected CustomConfig into `Config/BinaryConfig.ini`, ensuring installed/precompiled engine builds use the selected store settings at runtime. This controls the title version suffix, Steam wishlist visibility, online subsystem and achievement namespace together. Changing configuration requires regenerating/staging the PAK; do not reuse it with `-skippak`.
- Upload the whole Windows directory, not only its bootstrap executable. The actual game binary is under `TunaSweeper/Binaries/Win64/`, alongside BaseSDK, OwnershipSDK and LogSDK DLLs.

## Verification

- `Tools/StoveTests/RunTests.bat` exercises ownership acceptance/rejection without contacting STOVE.
- Build/cook/package must succeed for the selected target, and the existing `BuildFlavorData.ps1 -Mode VerifyDemo` validates demo data separation.
- Compare the three packaged SDK DLLs with the supplied originals and confirm that no credentials file is staged.
- A successful package does not establish successful authentication. Final launcher verification requires a STOVE account entitled to the selected product and its corresponding launcher build registration. Check successful launch, non-owned/refunded access, and normal exit using the STOVE test environment.
