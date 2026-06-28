[English](README.md) · **Русский**

# Unreal MCP Toolkit

![Unreal Engine 5.8](https://img.shields.io/badge/Unreal%20Engine-5.8-0e1128?logo=unrealengine&logoColor=white)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Win64-blue)
![Status](https://img.shields.io/badge/status-experimental-orange)
![Version](https://img.shields.io/badge/version-1.1-informational)
![Requires](https://img.shields.io/badge/requires-Model%20Context%20Protocol%20%2B%20Python-lightgrey)
[![Latest release](https://img.shields.io/github/v/release/timargv/UnrealMCPToolkit?label=download&logo=github)](https://github.com/timargv/UnrealMCPToolkit/releases/latest)

> Управляй редактором **Unreal Engine 5.8** из любого **MCP-клиента / агента** — универсальные инструменты скриптинга (`execute_python`, `execute_console_command`) плюс редакторские удобства (`take_screenshot`, `save_all`, `list_assets`, `get_output_log`) поверх встроенного в UE сервера **Model Context Protocol**.

[Зачем](#зачем-этот-плагин-вместо-встроенного-ue-mcp) · [Требования](#требования) · [Установка](#установка) · [Быстрый старт](#быстрый-старт) · [Инструменты](#инструменты) · [Решение проблем](#решение-проблем) · [Безопасность](#-безопасность)

---

## Зачем этот плагин? (вместо встроенного UE-MCP)

В UE 5.8 есть плагин Epic **Model Context Protocol** — но это лишь *каркас сервера*. «Из коробки» он отдаёт один тулсет управления «скиллами» и **не умеет создавать ассеты, ставить акторы, запускать PCG или скриптить редактор**. Чтобы он делал что-то реальное, нужно самому писать C++-инструменты.

**Unreal MCP Toolkit — это и есть та работа, сделанная один раз и универсально.** Вместо десятков узких инструментов он добавляет два, покрывающих всё, плюс несколько безопасных редакторских удобств:

| Инструмент | Что даёт |
|------------|----------|
| `execute_python` | **Весь Python-API `unreal`** — создавать/править любой ассет, PCG-граф, актор, билдить, сохранять, автоматизировать. |
| `execute_console_command` | **Любую** консольную команду редактора/движка (cvars, `stat`, exec-команды). |
| `take_screenshot` | **Контекст-безопасный** снимок активного вьюпорта (по умолчанию возвращает путь к файлу, а не гигантский base64). |
| `save_all` · `list_assets` · `get_output_log` | Повседневные редакторские удобства без написания Python каждый раз. |

**В сравнении с Python Remote Execution** (подход VS Code / Rider): та же мощь, но как **нативные MCP-инструменты** через единый MCP-эндпоинт — без отдельного UDP/TCP-клиента, без галки «Enable Remote Execution», со структурным запросом/ответом.

> Он не решает за тебя конкретные задачи — он даёт любому MCP-клиенту ключи от редактора.

## Совместимость

| Unreal Engine | Статус |
|---------------|--------|
| 5.8           | ✅ Поддерживается |

## Требования

- **Unreal Engine 5.8** (Launcher или source build).
- Плагин Epic **Model Context Protocol** — experimental, по умолчанию выключен (*Edit → Plugins → Model Context Protocol*).
- **Python Editor Script Plugin** — нужен для `execute_python`.
- **Запущенный MCP-сервер**: *Project Settings → Plugins → Model Context Protocol → Auto Start Server*, либо консольная команда `ModelContextProtocol.StartServer`. Эндпоинт по умолчанию: `http://127.0.0.1:8000/mcp`.

Оба движковых плагина прописаны зависимостями в `.uplugin`, поэтому включаются автоматически вместе с этим плагином.

## Установка

### Вариант A — Готовая сборка (без компиляции)

Скачай последний **[precompiled-релиз](https://github.com/timargv/UnrealMCPToolkit/releases/latest)** (UE 5.8 / Win64), распакуй папку `UnrealMCPToolkit` в каталог `Plugins/` своего проекта и запусти редактор — без сборки. Работает даже в Blueprint-only проектах. Движок должен быть **ровно UE 5.8** (бинарники привязаны к версии).

### Вариант B — Из исходников

Склонируй в папку `Plugins/` своего проекта и пересобери:

```bash
git clone https://github.com/timargv/UnrealMCPToolkit.git YourProject/Plugins/UnrealMCPToolkit
```

Или как submodule:

```bash
git submodule add https://github.com/timargv/UnrealMCPToolkit.git Plugins/UnrealMCPToolkit
```

Затем: перегенерируй project files → собери editor-таргет → запусти → проверь, что **Unreal MCP Toolkit** включён в *Edit → Plugins*. Инструменты регистрируются при старте редактора (и заново на `ModelContextProtocol.RefreshTools`).

## Быстрый старт

1. Запусти MCP-сервер в редакторе (см. [Требования](#требования)).
2. Укажи MCP-клиенту эндпоинт сервера. Пример проектного конфига (формат зависит от клиента; это типовой вид `.mcp.json`):

   ```json
   {
     "mcpServers": {
       "unreal": { "type": "http", "url": "http://127.0.0.1:8000/mcp" }
     }
   }
   ```
3. Вызови инструмент. Первый смоук-тест:

   ```jsonc
   // tool: execute_python
   { "python": "import unreal; unreal.log('hello from MCP'); print(unreal.SystemLibrary.get_engine_version())" }
   ```

## Инструменты

### `execute_python`

Выполняет произвольный Python-скрипт в редакторе (многострочный поддерживается). Возвращает результат + перехваченный лог; исключения Python приходят как error-результат.

| Параметр | Тип    | Обяз. | Описание                 |
|----------|--------|-------|--------------------------|
| `python` | string | да    | Python-код для выполнения |

```json
{ "python": "import unreal\nactors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)\n[actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(i*200,0,0)) for i in range(20)]" }
```

> Примечание: для спавна/правки акторов в UE 5.x используй `unreal.get_editor_subsystem(unreal.EditorActorSubsystem)`, а не устаревший `EditorLevelLibrary`.

### `execute_console_command`

Выполняет одну консольную команду редактора/движка и перехватывает её текстовый вывод.

| Параметр  | Тип    | Обяз. | Описание                    |
|-----------|--------|-------|-----------------------------|
| `command` | string | да    | Консольная команда           |

```json
{ "command": "stat unit" }
```

### `take_screenshot`

Снимает **активный вьюпорт редактора**, уменьшает (с сохранением пропорций) и кодирует в JPEG/PNG в `<Project>/Saved/Screenshots/MCP/`.

**Контекст-безопасен по дизайну.** По умолчанию возвращает компактный **текстовый** результат — абсолютный путь к файлу, размеры в пикселях и размер файла в КБ — и **никогда** не выдаёт base64-блоб. Передай `"inline": true`, чтобы дополнительно вшить картинку, но она вшивается только если файл меньше жёсткого порога ~256 КБ; иначе вернётся путь и пометка открыть файл. Так полноразмерный скриншот не переполняет контекст модели. Захват синхронный, поэтому к моменту возврата результата файл уже записан на диск.

| Параметр  | Тип     | Обяз. | По умолч. | Описание                                                              |
|-----------|---------|-------|-----------|----------------------------------------------------------------------|
| `width`   | integer | нет   | `1280`    | Макс. длинная сторона в px (пропорции сохраняются, без апскейла), зажат 256..3840 |
| `format`  | string  | нет   | `"jpg"`   | `"jpg"` (меньше) или `"png"` (без потерь)                            |
| `inline`  | boolean | нет   | `false`   | Дополнительно вшить картинку, если она меньше порога                  |

```json
{ "width": 768, "format": "jpg" }
```

### `save_all`

Сохраняет все «грязные» map- и content-пакеты без диалога (File > Save All).

_Без параметров._

```json
{}
```

### `list_assets`

Перечисляет object-пути ассетов из Asset Registry, опционально с фильтром по классу. Вывод ограничен, чтобы результат оставался компактным.

| Параметр | Тип    | Обяз. | По умолч. | Описание                                                                    |
|----------|--------|-------|-----------|-----------------------------------------------------------------------------|
| `path`   | string | нет   | `/Game`   | Content-путь, поиск рекурсивный                                              |
| `class`  | string | нет   | _(нет)_   | Фильтр по классу: короткое имя (`StaticMesh`) или полный путь (`/Script/Engine.StaticMesh`); подклассы включаются |

```json
{ "path": "/Game/Meshes", "class": "StaticMesh" }
```

### `get_output_log`

Возвращает последние строки лога редактора (из in-process кольцевого буфера, установленного при старте).

| Параметр | Тип     | Обяз. | По умолч. | Описание                                      |
|----------|---------|-------|-----------|-----------------------------------------------|
| `lines`  | integer | нет   | `100`     | Сколько последних строк вернуть, зажат 1..2000 |

```json
{ "lines": 200 }
```

## Решение проблем

| Симптом | Что делать |
|---------|------------|
| Инструменты не видны в MCP-клиенте | Убедись, что **сервер запущен**, а клиент указывает на `…/mcp`. Выполни `ModelContextProtocol.RefreshTools` в консоли редактора. |
| `execute_python` не работает | Включи **Python Editor Script Plugin** и перезапусти редактор. |
| Клиент не подключается | Проверь эндпоинт/порт (по умолчанию `127.0.0.1:8000`) в *Project Settings → Model Context Protocol*. |
| Инструментов нет после включения плагина | Пересобери editor-таргет; проверь, что **Unreal MCP Toolkit** включён в *Edit → Plugins*; перезапусти. |

## 🔒 Безопасность

**Плагин выполняет произвольный Python и консольные команды с полными правами редактора.** Всё, что подключено к MCP-серверу, может читать/менять проект и выполнять код на твоей машине.

- Используй только на **доверенной локальной dev-машине**.
- Это модуль типа `Editor` (в shipping-сборку не попадает) — но и в распространяемые editor-сборки его класть не стоит.
- Считай MCP-эндпоинт привилегированным; не выставляй его в недоверенные сети/клиентам.

## Лицензия

[MIT](LICENSE) © 2026 Timur Ragimkhanov
