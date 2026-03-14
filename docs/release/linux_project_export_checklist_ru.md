# Checklist для Linux Project Export

Этот checklist является user-facing путём приёмки Linux artifact-а, который DeltaQ экспортировала через `Build -> Export Linux Bundle`.

## 1. Что сейчас входит в scope

`Linux Project Export v1` сейчас означает:

- только Linux export;
- типы проектов DeltaQ: `console` и `desktop`;
- build-coupled export path: перед упаковкой DeltaQ пересобирает проект и не отдаёт silently stale binary;
- две формы output из одного и того же validated payload:
  - `dist/<ProjectName>/`
  - `dist/<ProjectName>.tar.gz`

Текущая граница `v1` уже, чем универсальное packaging-обещание:

- Windows/macOS export в этот flow не входят;
- AppImage export для пользовательских проектов в этот flow не входит;
- universal cross-distro compatibility пока не заявляется;
- imported pack-и, которым нужны дополнительные runtime `.so` или data files вне собранного executable, всё ещё требуют явной ручной проверки.

## 2. Экспорт из DeltaQ

Откройте проект в DeltaQ и используйте:

`Build -> Export Linux Bundle`

Ожидаемый результат внутри project tree:

- `dist/<ProjectName>/`
- `dist/<ProjectName>.tar.gz`

Ожидаемый layout bundle:

- launcher `<ProjectName>`
- `bin/<ProjectName>.bin`
- `lib/`
- `assets/fonts/`
- `EXPORT_INFO.txt`

## 3. Проверка exported payload

Из source checkout DeltaQ проверьте обе формы:

```bash
./scripts/verify_project_export_bundle.sh path/to/dist/<ProjectName>
./scripts/verify_project_export_archive.sh path/to/dist/<ProjectName>.tar.gz
```

Эти проверки валидируют:

- layout bundle;
- наличие launcher-а и executable;
- desktop font baseline;
- layout после распаковки archive;
- loader-level resolution для non-allowlisted runtime libraries.

## 4. Handoff-проверка

Для самого понятного handoff proof используйте archive:

```bash
tar xf <ProjectName>.tar.gz
cd <ProjectName>
./<ProjectName>
```

Для desktop-проектов в headless-среде можно использовать:

```bash
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ./<ProjectName>
```

## 5. Что считается pass

Exported Linux project находится в хорошем handoff-состоянии, если:

- DeltaQ создала и `dist/<ProjectName>/`, и `dist/<ProjectName>.tar.gz`;
- bundle и archive verification scripts проходят успешно;
- распакованный archive запускается вне исходного project tree;
- desktop export берёт bundled runtime libraries из `bundle/lib`, а не молча из host system.
