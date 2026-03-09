# DeltaQ Example Catalog

This directory is the concrete product surface of DeltaQ: every example is a checked-in source tree, not a synthetic demo assembled only inside the IDE.

## Recommended Order

1. [minimal_console_flow](minimal_console_flow/)
   Start here. The smallest proof of `module -> graph -> generated C code -> build -> run`.

2. [desktop_ui_flow](desktop_ui_flow/)
   The strongest desktop/UI showcase: graph, `.dqui`, generated SDL2 runtime files, and live event handlers.

3. [reusable_composition_console](reusable_composition_console/)
   The reuse story: composite submodules, separate generated units, and repeated use from one root graph.

4. [imported_pack_sensor_console](imported_pack_sensor_console/)
   The first imported-pack reference path from external library to curated pack and working graph runtime.

5. [imported_pack_checksum_console](imported_pack_checksum_console/)
   The second imported-pack reference path from a different domain, showing that imported packs are the main ecosystem growth channel rather than more `core` expansion.

## Fixture SDK Trees

These directories back the imported-pack showcase projects and are useful when you want to inspect the vendor side of the story, not when you want the shortest DeltaQ walkthrough:

- [imported_pack_sensor_sdk](imported_pack_sensor_sdk/)
- [imported_pack_checksum_sdk](imported_pack_checksum_sdk/)

## Which Example Answers Which Question

- "What is DeltaQ in 5 minutes?" -> [minimal_console_flow](minimal_console_flow/)
- "How does the UI/designer/runtime path look?" -> [desktop_ui_flow](desktop_ui_flow/)
- "Why do submodules and reuse matter?" -> [reusable_composition_console](reusable_composition_console/)
- "How do external libraries become reusable packs?" -> [imported_pack_sensor_console](imported_pack_sensor_console/) and [imported_pack_checksum_console](imported_pack_checksum_console/)

## Related Docs

- [Onboarding Index](../../docs/onboarding/README.md)
- [First Run Walkthrough](../../docs/onboarding/first_run.md)
- [Linux-First Release Checklist](../../docs/release/linux_first_release_checklist.md)
