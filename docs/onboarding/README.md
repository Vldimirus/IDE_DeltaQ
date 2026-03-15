# DeltaQ Onboarding

This folder is the shortest documentation entry point for a new DeltaQ user on Linux.

## Start Here

1. [First Run With DeltaQ](first_run.md)
   The fastest `module -> graph -> generated C code -> build -> run` walkthrough.

2. [Example Catalog](../../resources/examples/README.md)
   The recommended order for the checked-in showcase projects, plus what each example proves.

3. [Linux-First Release Checklist](../release/linux_first_release_checklist.md)
   The manual acceptance path for Linux tarball and AppImage artifacts when you want to validate a packaged release rather than a source checkout.

4. [Linux Project Export Checklist](../release/linux_project_export_checklist.md)
   The handoff path for Linux application artifacts produced from a DeltaQ project via `Build -> Export Linux Bundle`.

## Suggested Order

Use the docs in this order:

1. run the minimal console walkthrough;
2. open the example catalog and choose the next stronger scenario;
3. use the Linux artifact checklist if you are reviewing or handing off a packaged build.
4. use the project export checklist when you want to hand off an application built inside DeltaQ rather than the IDE itself.

If you want to create a fresh desktop project instead of opening a checked-in example, start with `Desktop Text Editor` in the New Project wizard. Use `Desktop UI Baseline` only when you deliberately want the thinner advanced desktop baseline.

## Related Docs

- [Library Docs Hub](../library/README.md)
- [Release And CI](../release/README.md)
