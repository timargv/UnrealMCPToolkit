<!-- Language: **English** · [Русский](README.ru.md) -->
**English** · [Русский](README.ru.md)

# Unreal MCP Toolkit

![Unreal Engine 5.8](https://img.shields.io/badge/Unreal%20Engine-5.8-0e1128?logo=unrealengine&logoColor=white)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Win64-blue)
![Status](https://img.shields.io/badge/status-experimental-orange)
![Version](https://img.shields.io/badge/version-1.1-informational)
![Requires](https://img.shields.io/badge/requires-Model%20Context%20Protocol%20%2B%20Python-lightgrey)

> Control the **Unreal Engine 5.8** editor from any **MCP client / agent** — universal scripting tools (`execute_python`, `execute_console_command`) plus editor conveniences (`take_screenshot`, `save_all`, `list_assets`, `get_output_log`) layered on Epic's built-in **Model Context Protocol** server.

[Why](#why-this-plugin-vs-the-built-in-ue-mcp) · [Requirements](#requirements) · [Install](#install) · [Quick start](#quick-start) · [Tools](#tools) · [Troubleshooting](#troubleshooting) · [Security](#-security)

---

## Why this plugin? (vs the built-in UE MCP)

UE 5.8 ships Epic's **Model Context Protocol** plugin — but that is only the *server framework*. Out of the box it exposes a single "skills" toolset and **cannot create assets, place actors, run PCG, or script the editor**. To make it do anything real, you have to write C++ tools yourself.

**Unreal MCP Toolkit is that work — done once, and universally.** Instead of dozens of narrow tools, it adds two that cover everything, plus a handful of safe editor conveniences:

| Tool | Gives you |
|------|-----------|
| `execute_python` | The **entire `unreal` Python API** — create/modify any asset, PCG graph, actor, build, save, automate. |
| `execute_console_command` | **Any** editor/engine console command (cvars, `stat`, exec commands). |
| `take_screenshot` | A **context-safe** capture of the active viewport (returns a file path by default, never a giant base64 blob). |
| `save_all` · `list_assets` · `get_output_log` | Everyday editor conveniences without writing Python each time. |

**vs Python Remote Execution** (the VS Code / Rider approach): same power, but exposed as **native MCP tools** over the one MCP endpoint — no separate UDP/TCP client, no extra "Enable Remote Execution" setting, structured request/response.

> It doesn't solve specific problems for you — it hands any MCP client the keys to the editor.

## Compatibility

| Unreal Engine | Status |
|---------------|--------|
| 5.8           | ✅ Supported |

## Requirements

- **Unreal Engine 5.8** (Launcher or source build).
- Epic's **Model Context Protocol** plugin — experimental, off by default (*Edit → Plugins → Model Context Protocol*).
- **Python Editor Script Plugin** — required for `execute_python`.
- The **MCP server running**: *Project Settings → Plugins → Model Context Protocol → Auto Start Server*, or the console command `ModelContextProtocol.StartServer`. Default endpoint: `http://127.0.0.1:8000/mcp`.

Both engine plugins are declared as dependencies in the `.uplugin`, so they are enabled automatically with this plugin.

## Install

Clone into your project's `Plugins/` folder, then rebuild:

```bash
git clone https://github.com/timargv/UnrealMCPToolkit.git YourProject/Plugins/UnrealMCPToolkit
```

Or add it as a submodule:

```bash
git submodule add https://github.com/timargv/UnrealMCPToolkit.git Plugins/UnrealMCPToolkit
```

Then: regenerate project files → build the editor target → launch → confirm **Unreal MCP Toolkit** is enabled in *Edit → Plugins*. Tools register on editor start (and re-register on `ModelContextProtocol.RefreshTools`).

## Quick start

1. Start the MCP server in the editor (see [Requirements](#requirements)).
2. Point your MCP client at the server endpoint. Example project-scoped config (format depends on your client; this is the common `.mcp.json` shape):

   ```json
   {
     "mcpServers": {
       "unreal": { "type": "http", "url": "http://127.0.0.1:8000/mcp" }
     }
   }
   ```
3. Call a tool. First smoke test:

   ```jsonc
   // tool: execute_python
   { "python": "import unreal; unreal.log('hello from MCP'); print(unreal.SystemLibrary.get_engine_version())" }
   ```

## Tools

### `execute_python`

Run an arbitrary Python script in the editor (multi-statement supported). Returns the evaluated result plus captured log output; Python exceptions come back as an error result.

| Parameter | Type   | Required | Description              |
|-----------|--------|----------|--------------------------|
| `python`  | string | yes      | Python source to execute |

```json
{ "python": "import unreal\nactors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)\n[actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(i*200,0,0)) for i in range(20)]" }
```

> Note: prefer `unreal.get_editor_subsystem(unreal.EditorActorSubsystem)` over the deprecated `EditorLevelLibrary` for spawning/editing actors in UE 5.x.

### `execute_console_command`

Run a single editor/engine console command and capture its text output.

| Parameter | Type   | Required | Description               |
|-----------|--------|----------|---------------------------|
| `command` | string | yes      | Console command to execute |

```json
{ "command": "stat unit" }
```

### `take_screenshot`

Capture the **active editor viewport**, downscaled (aspect preserved) and encoded as JPEG/PNG under `<Project>/Saved/Screenshots/MCP/`.

**Context-safe by design.** By default it returns a small **text** result — the absolute file path, pixel dimensions and file size in KB — and **never** emits a base64 blob. Pass `"inline": true` to also embed the image, but it is embedded only when the encoded file is below a hard ~256 KB cap; otherwise you get the path plus a note to open the file. This keeps a full-resolution screenshot from flooding the model's context window. The capture is synchronous, so the file is complete on disk by the time the result returns.

| Parameter | Type    | Required | Default | Description                                                        |
|-----------|---------|----------|---------|--------------------------------------------------------------------|
| `width`   | integer | no       | `1280`  | Max long-edge width in px (aspect preserved, never upscaled), clamped 256..3840 |
| `format`  | string  | no       | `"jpg"` | `"jpg"` (smaller) or `"png"` (lossless)                            |
| `inline`  | boolean | no       | `false` | Also embed the image inline if it is below the size cap            |

```json
{ "width": 768, "format": "jpg" }
```

### `save_all`

Save all dirty map and content packages without a dialog (File > Save All).

_No parameters._

```json
{}
```

### `list_assets`

List asset object paths from the Asset Registry, optionally filtered by class. Output is capped to keep the result small.

| Parameter | Type   | Required | Default  | Description                                                                 |
|-----------|--------|----------|----------|-----------------------------------------------------------------------------|
| `path`    | string | no       | `/Game`  | Content path searched recursively                                           |
| `class`   | string | no       | _(none)_ | Class filter: short name (`StaticMesh`) or full path (`/Script/Engine.StaticMesh`); subclasses included |

```json
{ "path": "/Game/Meshes", "class": "StaticMesh" }
```

### `get_output_log`

Return the most recent editor log lines (captured by an in-process ring buffer installed at startup).

| Parameter | Type    | Required | Default | Description                                  |
|-----------|---------|----------|---------|----------------------------------------------|
| `lines`   | integer | no       | `100`   | Number of recent lines to return, clamped 1..2000 |

```json
{ "lines": 200 }
```

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Tools not listed by the MCP client | Ensure the MCP **server is started** and the client points to `…/mcp`. Run `ModelContextProtocol.RefreshTools` in the editor console. |
| `execute_python` fails / unavailable | Enable the **Python Editor Script Plugin** and restart the editor. |
| Client can't connect | Confirm the endpoint/port (default `127.0.0.1:8000`) matches *Project Settings → Model Context Protocol*. |
| Tools missing after enabling the plugin | Rebuild the editor target; confirm **Unreal MCP Toolkit** is enabled in *Edit → Plugins*; restart. |

## 🔒 Security

**This plugin executes arbitrary Python and console commands with full editor privileges.** Anything connected to the MCP server can read/modify your project and run code on your machine.

- Use only on a **trusted, local development machine**.
- It is an `Editor`-type module (excluded from shipping builds) — keep it out of any distributed editor build too.
- Treat the MCP endpoint as privileged; don't expose it to untrusted networks/clients.

## License

[MIT](LICENSE) © 2026 Timur Ragimkhanov
