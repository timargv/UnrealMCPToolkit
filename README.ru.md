[English](README.md) · **Русский**

# Unreal MCP Toolkit

![Unreal Engine 5.8](https://img.shields.io/badge/Unreal%20Engine-5.8-0e1128?logo=unrealengine&logoColor=white)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Win64-blue)
![Status](https://img.shields.io/badge/status-experimental-orange)
![Version](https://img.shields.io/badge/version-1.0-informational)
![Requires](https://img.shields.io/badge/requires-Model%20Context%20Protocol%20%2B%20Python-lightgrey)

> Управляй редактором **Unreal Engine 5.8** из любого **MCP-клиента / AI-агента** — два универсальных инструмента (`execute_python`, `execute_console_command`) поверх встроенного в UE сервера **Model Context Protocol**.

[Зачем](#зачем-этот-плагин-вместо-встроенного-ue-mcp) · [Требования](#требования) · [Установка](#установка) · [Быстрый старт](#быстрый-старт) · [Инструменты](#инструменты) · [Решение проблем](#решение-проблем) · [Безопасность](#-безопасность)

---

## Зачем этот плагин? (вместо встроенного UE-MCP)

В UE 5.8 есть плагин Epic **Model Context Protocol** — но это лишь *каркас сервера*. «Из коробки» он отдаёт один тулсет управления «скиллами» и **не умеет создавать ассеты, ставить акторы, запускать PCG или скриптить редактор**. Чтобы он делал что-то реальное, нужно самому писать C++-инструменты.

**Unreal MCP Toolkit — это и есть та работа, сделанная один раз и универсально.** Вместо десятков узких инструментов он добавляет два, покрывающих всё:

| Инструмент | Что даёт |
|------------|----------|
| `execute_python` | **Весь Python-API `unreal`** — создавать/править любой ассет, PCG-граф, актор, билдить, сохранять, автоматизировать. |
| `execute_console_command` | **Любую** консольную команду редактора/движка (cvars, `stat`, exec-команды). |

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
{ "python": "import unreal\n[unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(i*200,0,0)) for i in range(20)]" }
```

### `execute_console_command`

Выполняет одну консольную команду редактора/движка и перехватывает её текстовый вывод.

| Параметр  | Тип    | Обяз. | Описание                    |
|-----------|--------|-------|-----------------------------|
| `command` | string | да    | Консольная команда           |

```json
{ "command": "stat unit" }
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
