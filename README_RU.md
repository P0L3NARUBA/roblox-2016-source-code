![My *handmade* Roblox Logo](https://github.com/user-attachments/assets/ced623cd-6692-4759-8e46-e9453f5454fc)

<p align="center">
<img alt="GitHub Repo Size" src="https://img.shields.io/github/repo-size/P0L3NARUBA/roblox-2016-source-code">
<img alt="GitHub Release" src="https://img.shields.io/github/v/release/P0L3NARUBA/roblox-2016-source-code?color=violet">
<img alt="GitHub Last Commit" src="https://img.shields.io/github/last-commit/P0L3NARUBA/roblox-2016-source-code/master">
</p>

# Roblox 2016 Source Code

Этот исходный код взят из **[robloxsrc.zip](https://mega.nz/file/mrxkSRRK#n5YmV1iPUPZCfiI6IDWkT3eDq9k3-yA7rl_hURked8Y)**, который некоторое время назад был распространен, но его стало трудно найти.
После долгих усилий этот репозиторий был перенесен на GitHub со многими улучшениями!

**Чтобы выполнить сборку на основе исходного кода, обратитесь к [BUILDING.md](/BUILDING.md)**<br>
- Обязательно внимательно прочитайте его, чтобы избежать каких-либо проблем.

**Возникли проблемы? Вы можете получить помощь через [наш сервер Discord](https://www.discord.gg/rVrYHdrbsp) или в разделе [Проблемы](https://github.com/P0L3NARUBA/roblox-2016-source-code/issues).**

**Хотите немедленно приступить к игре? Ознакомьтесь с [Releases](https://github.com/P0L3NARUBA/roblox-2016-source-code/releases)**<br>
**ПРИМЕЧАНИЕ:** Для запуска игры вам может понадобиться **[Rocknet](https://github.com/P0L3NARUBA/Rocknet/tree/main)**.

**Каждый вклад продвигает проект вперед — мы всегда открыты для новых помощников!**

Этот **[pixel-lighting](https://github.com/P0L3NARUBA/roblox-2016-source-code/tree/pixel-lighting)** ветка не поддерживается мной и может быть устаревшей по сравнению с текущей веткой.
# содержание
1. [🪨 Особенности / Additions](#-features--additions)
2. [📚 Используемые Библиотеки](#-libraries-used)
3. [🔨 Используемые инструменты](#-tools-used)
4. [🎯 Текущие цели](#-current-goals)
5. [⚠️ Текущие проблемы](#%EF%B8%8F-current-problems)

---

## 🪨 Особенности / дополнения
- Было добавлено множество новых функций, и мы продолжаем совершенствоваться!
- Исправлены ошибки, связанные с компиляцией, чтобы все проекты работали должным образом.
- Весь исходный код был очищен для большей ясности и простоты использования.
- Обновлена заставка и даты защиты авторских прав.
-Переработал несколько библиотек C# и исполняемых файлов, используя **[ILSpy](https://github.com/icsharpcode/ILSpy/releases)**, чтобы сделать исходный код доступным.
- Представляю вашему вниманию **[Rocknet](https://github.com/P0L3NARUBA/Rocknet/tree/main)** — сервер, созданный специально для этого источника.

## 📚 Используемые Библиотеки
- [Boost](/Contribs/boost_1_56_0) = 1.56.0
- [cpp-netlib](/Contribs/cpp-netlib-0.11.0-final) = 0.11.0-final
- [DSBaseClasses](/Contribs/DSBaseClasses) = *неопознанная версия*
- [OpenSSL](/Contribs/openssl) = 1.0.0c
- [Qt](/BUILDING_CONTRIBS.md) = 4.8.5
- [Roblox SDK](/Contribs/SDK) = *неопознанная версия*
- [SDL2](/Contribs/SDL2) = 2.0.4
- [VMProtectWin](/Contribs/VMProtectWin_2.13) = 2.13
- [w3c-libwww](/Contribs/w3c-libwww-5.4.0) = 5.4.0
- [curl](/Contribs/windows/x86/curl/curl-7.43.0) = 7.43.0
- [zlib](/Contribs/windows/x86/zlib/zlib-1.2.8) = 1.2.8
- [glsl-optimizer](/Rendering/ShaderCompiler/glsl-optimizer) = *неопознанная версия*
- [hlsl2glslfork](/Rendering/ShaderCompiler/hlsl2glslfork) = *неопознанная версия*
- [mojoshader](/Rendering/ShaderCompiler/mojoshader) = *неопознанная версия*
- [gSOAP](/RCCService/gSOAP/gsoap-2.7) = 2.7.10
- [RakNet](/Network/raknet) = 5 

## 🔨 Используемые Инструменты
- [HxD](https://mh-nexus.de/en/downloads.php?product=HxD20)
- [ILSpy](https://github.com/icsharpcode/ILSpy/releases)
- [rbxsigner](/Tools/rbxsigner) = *неопознанная версия*

## 🎯 Current Goals
- Бэкпорт/Реализация **[Hitius](https://mega.nz/file/DnxUTAgI#52pYMEJyRFMMXVMAU71GboVWYxaTCv25eWB4QHFma6M)**, **[Graphictoria](https://mega.nz/file/e2RU0YbT#tGVrpYqR4fv6z7a4QQcdqT0nbmgdssGm3wGFd9jCiHA)** и **[Economy Simulator](https://mega.nz/file/76AyxJzC#fuKcKHTK6YI5S8zLyelsB7PIt0fVVTsWu9KTrgvXk2E)** Особенности
  - [x] Color3uint8  
     - [x] Color3.fromRGB()  
  - [ ] R15 character support  
- [x] :Connect() and :Wait()
- [x] Новые Шрифты
- [ ] Добавить поддержку кириллицы и нелатинских языков
  - [ ] Совместимость с UTF / Unicode  
  - [ ] Улучшен фильтр ненормативной лексики и нецензурных выражений
- [ ] Добавление или перенос новой версии Lua
- [x] Поддержка новых форматов мешей
- [ ] Поддержка для более новых версий плейсов
- [ ] Добавление тёмной темы для Роблокс Студио  
- [ ] Исправление проблемы с записью в игре 
- [x] Переместите несвязанные файлы из папки **content\fonts** 
- [ ] Запретить загрузчикам перезаписывать исходные файлы и реестры Roblox
  - Когда эта проблема будет решена, в будущих версиях будут требоваться только загрузчики; обновления Rocknet будет достаточно для обновления
- [ ] Создайте все проекты в исходном коде, используя последнюю версию Visual Studio  
  - [ ] Обеспечьте поддержку последних стандартов C/C++ (C++17 или более поздней версии).  
  - [ ] включить 64-разрядную поддержку во всех применимых проектах

## ⚠️ Текущие проблемы
- Undo/Redo не совсем точно обрабатывает свойства `Color3`; они часто возвращаются к ближайшему значению `BrickColor`.
  - Это может привести к несоответствиям, особенно с "цветами тела`.

---
