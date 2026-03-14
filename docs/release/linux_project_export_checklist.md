# Linux Project Export Checklist

This checklist is the user-facing acceptance path for a Linux application artifact exported from DeltaQ via `Build -> Export Linux Bundle`.

## 1. Know The Current Scope

`Linux Project Export v1` currently means:

- Linux-only export;
- `console` and `desktop` DeltaQ project types;
- a build-coupled export path: DeltaQ rebuilds before packaging and does not silently hand off a stale binary;
- two output forms from the same validated payload:
  - `dist/<ProjectName>/`
  - `dist/<ProjectName>.tar.gz`

The current `v1` boundary is still narrower than a universal packaging claim:

- Windows/macOS export is not part of this flow;
- AppImage export for user projects is not part of this flow;
- universal cross-distro compatibility is not claimed yet;
- imported packs that need extra runtime `.so` files or data outside the built executable still require explicit manual validation.

## 2. Export From DeltaQ

Open the project in DeltaQ, then use:

`Build -> Export Linux Bundle`

Expected result inside the project tree:

- `dist/<ProjectName>/`
- `dist/<ProjectName>.tar.gz`

Expected bundle layout:

- `<ProjectName>` launcher
- `bin/<ProjectName>.bin`
- `lib/`
- `assets/fonts/`
- `EXPORT_INFO.txt`

## 3. Verify The Exported Payload

From a source checkout of DeltaQ, validate both forms:

```bash
./scripts/verify_project_export_bundle.sh path/to/dist/<ProjectName>
./scripts/verify_project_export_archive.sh path/to/dist/<ProjectName>.tar.gz
```

These checks validate:

- bundle layout;
- launcher and executable presence;
- desktop font baseline;
- archive extraction layout;
- loader-level resolution for non-allowlisted runtime libraries.

## 4. Handoff Test

For the clearest handoff proof, use the archive:

```bash
tar xf <ProjectName>.tar.gz
cd <ProjectName>
./<ProjectName>
```

For desktop projects in a headless validation environment, you can use:

```bash
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./<ProjectName>
```

## 5. What "Pass" Means

The exported Linux project is in good handoff shape if:

- DeltaQ produced both `dist/<ProjectName>/` and `dist/<ProjectName>.tar.gz`;
- bundle and archive verification scripts pass;
- the extracted archive starts outside the original project tree;
- desktop export resolves bundled runtime libraries from `bundle/lib` rather than silently from the host.
