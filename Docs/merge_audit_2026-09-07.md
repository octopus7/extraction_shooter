# Recent-work integration audit — 2026-09-07

Scope: work committed between 2026-09-06 17:35:39 and the audit on 2026-09-07, relative to main at `8bacb33c`.

## Integrated work

52 original work commits from 20 independent tips were merged with their ancestry preserved:

| Tip | Work |
| --- | --- |
| `583d4804` | Hinged loot containers |
| `85ad11e3` | Memo storage devices |
| `c2b70edf` | Modular roofless interior |
| `3c9173ec` | Four robot source models |
| `4cfbfe0a` | Water masks and optional painted sky |
| `6c2d79ac` | Exposed roots |
| `9937d9b2` | Round low bush |
| `bd85c874` | Dense short grass |
| `fa8a48ce` | Curved long grass |
| `80f4e4e0` | Sparse grass |
| `5fcf8581` | Spreading bush |
| `0245b50e` | Leaf litter |
| `1ad5661e` | Sparse bush |
| `535e12a9` | Lab and supply props |
| `8bb53750` | Interior expansion modules |
| `7874f2c1` | Weapon collection, facility rooms and mole control room |
| `6afc168b` | Extraction marker assembly |
| `56b88b67` | Authored noise emitter |
| `c8e68bcc` | Research localization |
| `4a0218c2` | Runtime placement anchors and difficulty/text data |

Research localization retains all 17 current nodes, including the four burn nodes and their effect values/type tags. The four burn nodes now also use Korean, English and Japanese UI CSV strings. Water uses the new mask implementation; previously removed editor startup generators stay removed. The completed noise importer and its entry point were removed after verification. Both sides of request/question log conflicts were retained.

## Verification

- UE 5.7 Development Editor build: succeeded, exit 0.
- Research, difficulty, raid placement, noise emitter and burn automation: 24 passed, 0 failed (two passed with existing duplicate UI string warnings), exit 0.
- `StylizedWater.MaskWater.AssetsAndTopology`: passed with actual RHI and material shader compilation, exit 0.
- Changed Unreal packages: 249 loaded, 0 missing/failed. The Python commandlet returned 1 because of the previously documented Niagara typed-element initialization ensure for `NE_PostProcess`; package loading itself completed successfully.
- `git diff --check`: passed after trimming two trailing blank lines.
- Removed noise importer and wrapper have no remaining operational references in the noise tool documentation.

Local reports are in `HYTemp/integration-tests`, `HYTemp/integration-water-tests`, and `HYTemp/integration-assets-result.json` (ignored, not part of published source).

## Preserved work outside the merge scope

The original main working directory had 5 modified tracked files and 135 untracked files. They were backed up under `HYTemp/merge-backup-20260907` before main was updated; pre-existing local changes are kept separate from the verified integration commits.

Three automatic editor snapshot commits (`2d5399c5`, `b80b6e5e`, `81de0d73`) and the pre-existing stash (`a5538fc0`, including its index parent) are retained as recovery data. Their unreviewed generated-asset rewrites are not treated as completed work to merge. Their actual work ancestors are integrated. Older unrelated unmerged branches are outside this 24-hour scope.
