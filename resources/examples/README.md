# DeltaQ Example Catalog

This directory is the concrete product surface of DeltaQ: every example is a checked-in source tree, not a synthetic demo assembled only inside the IDE.

## Recommended Order

1. [minimal_console_flow](minimal_console_flow/)
   Start here. The smallest proof of `module -> graph -> generated C code -> build -> run`.

2. [settings_file_console](settings_file_console/)
   The first file-backed useful baseline: `filesystem + config_json` in a real `build -> run` flow.

3. [sqlite_settings_console](sqlite_settings_console/)
   The first SQLite forcing-function: curated database pack in a real `build -> run` flow with schema creation, insert/update, and scalar select.

4. [sqlite_notes_desktop](sqlite_notes_desktop/)
   The desktop-side SQLite forcing-function: curated database pack reused inside a generated SDL2 desktop flow with persisted note loading and headless autoclose.

5. [process_timer_console](process_timer_console/)
   The second useful baseline: `process + timers` in a real `build -> run` flow with stdout, exit code, and time budget checks.

6. [transport_probe_console](transport_probe_console/)
   The third useful baseline: `tcp_udp + timers` as a loopback datagram probe in a real `build -> run` flow.

7. [serial_probe_console](serial_probe_console/)
   The fourth useful baseline: `serial + timers` as a self-contained PTY loopback probe in a real `build -> run` flow.

8. [desktop_ui_flow](desktop_ui_flow/)
   The strongest desktop/UI showcase: graph, `.dqui`, generated SDL2 runtime files, and live event handlers.

9. [reusable_composition_console](reusable_composition_console/)
   The reuse story: composite submodules, separate generated units, and repeated use from one root graph.

10. [imported_pack_sensor_console](imported_pack_sensor_console/)
   The first imported-pack reference path from external library to curated pack and working graph runtime.

11. [imported_pack_checksum_console](imported_pack_checksum_console/)
   The second imported-pack reference path from a different domain, showing that imported packs are the main ecosystem growth channel rather than more `core` expansion.

## Fixture SDK Trees

These directories back the imported-pack showcase projects and are useful when you want to inspect the vendor side of the story, not when you want the shortest DeltaQ walkthrough:

- [imported_pack_sensor_sdk](imported_pack_sensor_sdk/)
- [imported_pack_checksum_sdk](imported_pack_checksum_sdk/)

## Which Example Answers Which Question

- "What is DeltaQ in 5 minutes?" -> [minimal_console_flow](minimal_console_flow/)
- "How do file-backed settings look in DeltaQ?" -> [settings_file_console](settings_file_console/)
- "How does SQLite look in DeltaQ without growing `core`?" -> [sqlite_settings_console](sqlite_settings_console/)
- "How does SQLite look inside a desktop runtime path?" -> [sqlite_notes_desktop](sqlite_notes_desktop/)
- "How do subprocesses and time budgets look in DeltaQ?" -> [process_timer_console](process_timer_console/)
- "How do local transport probes look in DeltaQ?" -> [transport_probe_console](transport_probe_console/)
- "How do serial probes look in DeltaQ without real hardware?" -> [serial_probe_console](serial_probe_console/)
- "How does the UI/designer/runtime path look?" -> [desktop_ui_flow](desktop_ui_flow/)
- "Why do submodules and reuse matter?" -> [reusable_composition_console](reusable_composition_console/)
- "How do external libraries become reusable packs?" -> [imported_pack_sensor_console](imported_pack_sensor_console/) and [imported_pack_checksum_console](imported_pack_checksum_console/)

## Related Docs

- [Onboarding Index](../../docs/onboarding/README.md)
- [First Run Walkthrough](../../docs/onboarding/first_run.md)
- [Linux-First Release Checklist](../../docs/release/linux_first_release_checklist.md)
