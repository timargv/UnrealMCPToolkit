# Changelog

All notable changes to **Unreal MCP Toolkit** are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1] - 2026-06-25

### Added
- **`take_screenshot`** tool — captures the active editor viewport, downscaled
  (aspect preserved) and encoded as JPEG/PNG under
  `Saved/Screenshots/MCP/`. **Context-safe by design:** returns a small text
  result (path, dimensions, size in KB) by default; only embeds an inline image
  when explicitly requested *and* the encoded file is below a hard size cap.
  Parameters: `width` (default 1280, clamped 256..3840), `format` (`jpg`/`png`),
  `inline` (default `false`).
- **`save_all`** tool — saves all dirty map and content packages without a
  dialog (File > Save All). No parameters.
- **`list_assets`** tool — lists asset object paths from the Asset Registry.
  Parameters: `path` (default `/Game`), `class` (short name or full path,
  subclasses included). Output capped to keep the result small.
- **`get_output_log`** tool — returns the most recent editor log lines.
  Parameter: `lines` (default 100, clamped 1..2000). Backed by a capped log
  ring buffer installed on `GLog` at module startup.

### Changed
- Module startup now defers tool registration until Epic's
  `ModelContextProtocol` module has loaded (via `FModuleManager::OnModulesChanged`),
  so tools register reliably regardless of plugin load order. The existing
  `OnRefreshTools` re-registration is preserved.

### Dependencies
- Added `AssetRegistry`, `ImageCore` and `ImageWrapper` private module
  dependencies.

## [1.0] - 2026

### Added
- Initial release.
- **`execute_python`** tool — runs an arbitrary Python script in the editor and
  returns the result plus captured log output.
- **`execute_console_command`** tool — runs a single editor/engine console
  command and returns its captured text output.
- Idempotent registration with re-registration on
  `ModelContextProtocol.RefreshTools`.
