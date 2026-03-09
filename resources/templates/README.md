Project templates are file-based.

Each user-facing template lives in its own directory and must contain:

- `template.json` with metadata used by the New Project wizard
- `__PROJECT_NAME__.dqproj` or another file path that uses `__PROJECT_NAME__`
- ready-to-copy source files, graphs, UI layouts, assets, and any other checked-in content

`template.json` fields:

- `id`: stable template identifier
- `name`: display name in the wizard
- `description`: short description shown in the wizard
- `project_type`: `console` or `desktop`
- `sort_order`: numeric order in the wizard
- `summary_files`: optional list of important files for the summary page

Rules:

- All template text must live in files inside the template directory.
- `__PROJECT_NAME__` is replaced in file names and file contents during project creation.
- Directories without `template.json` are ignored by the wizard and may be used for tests or internal fixtures.
