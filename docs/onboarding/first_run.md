# First Run With DeltaQ

This walkthrough gives the shortest path to understanding DeltaQ without diving into the full internal architecture first.

## Step 1. Open the reference project

Open:

- `resources/examples/minimal_console_flow/minimal_console_flow.dqproj`

This project is intentionally small: one graph, only core modules, no extra libraries, and no composite submodules.

## Step 2. Inspect the source graph

Open:

- `resources/examples/minimal_console_flow/graphs/main.dqgraph`

What it does:

1. `string_constant` provides the string `Hello from DeltaQ!`;
2. the first `println` prints the greeting;
3. `read_line` reads one line from stdin;
4. the second `println` prints the entered text back.

This is the minimal DeltaQ path:

`module -> graph -> generated C code -> build -> run`

## Step 3. Build and run the project

In the IDE:

1. press `Build`;
2. then press `Run`;
3. type, for example, `DeltaQ`.

Expected output:

```text
Hello from DeltaQ!
DeltaQ
```

## Step 4. Inspect the generated code

After `pre-build` and compilation, open:

- `resources/examples/minimal_console_flow/src/main.c`

This file is generated from `graphs/main.dqgraph`. The important point here is that:

- the graph does not hide runtime behavior in a proprietary format;
- the result is plain C code;
- the generated output can be traced back to the source graph.

## Where to go next

After this walkthrough, continue with the stronger showcase examples:

- `resources/examples/desktop_ui_flow` — shows the desktop/UI path with generated SDL2 runtime files and live handlers;
- `resources/examples/reusable_composition_console` — shows composite submodules and real reuse;
- `resources/examples/imported_pack_sensor_console` — shows the path `external library -> curated pack -> graph -> build -> run`.

For the full newcomer path and Linux artifact handoff:

- `docs/onboarding/README.md`
- `resources/examples/README.md`
- `docs/release/linux_first_release_checklist.md`
