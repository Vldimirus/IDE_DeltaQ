# Linux-First Release Checklist

This checklist is the user-facing acceptance path for a DeltaQ Linux artifact before you hand it to another user or present it publicly.

Current framing for this checklist:

- treat the reviewed artifact as a **Linux release candidate**
- do not reinterpret it as a cross-platform release claim

## Pick The Artifact Style

- `.tar.gz`
  Use this when you want the most transparent manual review path: the bundle layout, examples, modules, and generated files are all visible as ordinary files.

- `.AppImage`
  Use this when you want the cleanest single-file handoff. For deep manual inspection of bundled examples and layout, the tarball remains the easier artifact to review.

## 1. Verify The Checksum

Keep the artifact next to `SHA256SUMS`, then run one of:

```bash
grep 'Linux-x86_64.tar.gz' SHA256SUMS | sha256sum -c
grep 'x86_64.AppImage' SHA256SUMS | sha256sum -c
```

## 2. Optional: Isolate Writable State

If you do not want to mix the check with your regular DeltaQ state, use:

```bash
export DELTAQ_HOME="$(mktemp -d)"
```

Expected writable locations after first launch:

- `$DELTAQ_HOME/config/settings.ini`
- `$DELTAQ_HOME/modules/core/pack.json`

Without `DELTAQ_HOME`, the same files are created under `~/.deltaq/`.

## 3. First Launch

### Tarball

```bash
tar xf DeltaQ-*-Linux-x86_64.tar.gz
cd DeltaQ-*-Linux-x86_64
./deltaq
```

### AppImage

```bash
chmod +x DeltaQ-*-x86_64.AppImage
./DeltaQ-*-x86_64.AppImage
```

On first launch, verify:

- the app starts without trying to write inside the bundled tree;
- writable state appears under `$DELTAQ_HOME/` or `~/.deltaq/`;
- the bundled `core` pack becomes available in the user-writable modules root.

## 4. Shortest Product Proof

For the clearest manual proof, use the tarball bundle and open:

- `examples/minimal_console_flow/minimal_console_flow.dqproj`

Then:

1. open `graphs/main.dqgraph`;
2. press `Build`;
3. press `Run`;
4. type `DeltaQ`.

Expected output:

```text
Hello from DeltaQ!
DeltaQ
```

After that, inspect:

- `src/main.c`

The point of this step is to confirm that the release artifact still expresses the same transparent path as the source checkout:

`module -> graph -> generated C code -> build -> run`

## 5. Stronger Optional Proof

If you want a broader showcase after the minimal console path:

- open `examples/desktop_ui_flow` for the desktop/UI path;
- open `examples/reusable_composition_console` for submodule reuse;
- open `examples/imported_pack_sensor_console` or `examples/imported_pack_checksum_console` for external-library-to-pack flows.

## 6. What "Pass" Means

The Linux artifact is in good user-facing shape if:

- checksum verification passes;
- first launch creates writable state outside the bundle;
- the bundled examples are present;
- `minimal_console_flow` still passes `open -> build -> run`;
- generated `src/main.c` remains easy to inspect and map back to the graph.
