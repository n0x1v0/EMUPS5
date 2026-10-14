# EMPUPS5

[![Build and Release](https://img.shields.io/github/actions/workflow/status/n0x1v0/EMUPS5/build.yml?label=build)](https://github.com/n0x1v0/EMUPS5/actions/workflows/build.yml)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64%20%7C%20Linux%20x64%20%7C%20macOS%20x86__64-0078D4.svg)](#requisitos)
[![License](https://img.shields.io/badge/license-GPL--2.0-blue.svg)](LICENSE)

EMUPS5 es una capa de traducción de PlayStation 5 libre y de código abierto, escrita en C++,
para Windows y Linux (macOS experimental). Está basada en
[KytyPS5](https://github.com/KytyPS5/KytyPS5), que a su vez parte de
[Kyty](https://github.com/InoriRus/Kyty).

Este repositorio es la build **`fix/robustez`**: KytyPS5 con una tanda de arreglos de
compatibilidad, estabilidad y seguridad (ver [Cambios](#cambios-de-esta-build)).

> [!IMPORTANT]
> EMPUPS5 no está afiliado a Sony Interactive Entertainment ni a PlayStation. No se distribuyen
> juegos ni software de sistema con copyright. Usa únicamente ficheros de juego obtenidos
> legalmente.

---

## Cambios de esta build

Sobre la base KytyPS5 `df5b9b9`:

| Área | Cambio |
|---|---|
| **Kernel / seguridad** | `fileSystem.cpp`: se rechaza `..` en rutas del guest (cierra un escape del sandbox). `KernelOpen`, `read/write/pread/pwrite` devuelven `EINVAL`/`EISDIR`/`EFBIG` en vez de abortar el proceso. |
| **Memoria** | `memory.cpp`: memoria flexible por defecto 1 GiB → **4 GiB**. |
| **Loader** | `symbolDatabase`: índices por NID/nombre → búsqueda **O(1)** (antes O(n) en el arranque). `runtimeLinker`: mutex en la contabilidad de imports sin resolver. |
| **GPU / PM4** | `pm4Dispatch`/`pm4Handlers`: nuevo handler `CpOpUnimplemented`; los 9 opcodes PM4 antes sin manejar ya **no abortan** (`CONTEXT_CONTROL`, `MEM_SEMAPHORE`, `RELEASE_MEM`, etc.). |
| **Vídeo** | `videoOut.cpp`: `BufferAttributeGroup::ImageInfo` pasa a `std::optional`; validación de índices/punteros nulos. |
| **Librerías de sistema** | `libSystemService`, `libAppContent`, `libUserService`, `libSaveData`: devuelven códigos de error en vez de `EXIT` ante argumentos inválidos del guest. |

Pendientes conocidos: `exit()` de libc (requiere un mecanismo de parada coordinada) y teselación
multi-modo (`spirvEmitterTessellation.cpp`).

---

## Requisitos

- Windows 10 (1803 o superior) o una distribución Linux actual; macOS experimental (x86-64 /
  Rosetta 2).
- CPU x86 de 64 bits.
- GPU con **Vulkan 1.3** y drivers actuales. En macOS, Vulkan lo aporta MoltenVK.

---

## Instalación en Windows

### Opción A — Descargar un binario ya compilado (sin instalar nada)

1. Entra en la pestaña **Actions**: https://github.com/n0x1v0/EMUPS5/actions
2. Abre la ejecución más reciente de **Build and Release KytyPS5** (o **Windows build (artifact)**).
3. Descarga el artifact **`KytyPS5-Windows-x64`** (contiene el emulador ya instalado).
4. Descomprime y ejecuta `launcher.exe` (interfaz gráfica) o `kyty_emulator.exe` (línea de
   comandos).

> En repos personales/fork no hay releases automáticas; los binarios salen como *artifacts* de
> Actions. En Windows 10/11, el SO ya trae el runtime de Vulkan actualizado con los drivers de
> GPU.

### Opción B — Compilar desde el código fuente

Requisitos:

- Git
- CMake 3.22.1 o superior
- Ninja
- Visual Studio 2022 (o Build Tools 2022) con las cargas **Desktop development with C++** y el
  componente **C++ Clang tools for Windows**
- Qt 6 para MSVC 2022 64-bit (módulos Concurrent, Network y Widgets)
- `glslangValidator` en el `PATH` ([glslang releases](https://github.com/KhronosGroup/glslang/releases))
- Python 3 en el `PATH`

> El compilador de Microsoft (`cl.exe`) **no** es compatible; se usa `clang-cl`.

Clona y compila desde un **x64 Native Tools Command Prompt for Visual Studio 2022**:

```powershell
git clone https://github.com/n0x1v0/EMUPS5.git
cd EMPUPS5
git submodule update --init --recursive

# Ajusta la ruta de Qt a tu versión instalada.
cmake -S . -B _Build/windows -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"

cmake --build _Build/windows --target launcher
cmake --install _Build/windows --prefix _Build/windows/install
```

El ejecutable y sus dependencias quedan en `_Build\windows\install`.
Para arrancar:

```powershell
.\_Build\windows\install\launcher.exe
# o directamente un juego:
.\_Build\windows\install\kyty_emulator.exe --game "D:\Games\Ejemplo"
```

---

## Instalación en Linux

### Dependencias (Debian/Ubuntu)

```bash
sudo apt-get install --no-install-recommends \
  clang lld ninja-build cmake git glslang-tools python3 pkg-config \
  libgl1-mesa-dev libx11-dev libxcursor-dev libxext-dev libxfixes-dev \
  libxi-dev libxrandr-dev libxss-dev libxtst-dev libxkbcommon-dev \
  libasound2-dev libpulse-dev libudev-dev libdbus-1-dev libwayland-dev wayland-protocols
```

Se necesita **Qt 6** (Concurrent, Network, Widgets) para el launcher: usa el paquete de tu
distribución (`qt6-base-dev`) o una instalación oficial de Qt.

### Compilar e instalar

```bash
git clone https://github.com/n0x1v0/EMUPS5.git
cd EMPUPS5
git submodule update --init --recursive

cmake -S . -B _Build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_PREFIX_PATH="$Qt6_DIR"

cmake --build _Build/linux --target launcher --parallel
cmake --install _Build/linux --prefix _Build/linux/install
```

El `install` copia las librerías y plugins de Qt junto a los binarios, así que
`_Build/linux/install` se ejecuta sin depender del Qt del sistema. FFmpeg se enlaza de forma
estática desde el core empaquetado.

Ejecutar:

```bash
./_Build/linux/install/launcher
# o directamente:
./_Build/linux/install/kyty_emulator --game "/games/Ejemplo"
```

### Sin Qt (solo emulador)

Para compilar `kyty_emulator` y los tests sin el launcher:

```bash
cmake -S . -B _Build/linux-no-qt -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DKYTY_BUILD_LAUNCHER=OFF
cmake --build _Build/linux-no-qt --target kyty_emulator kyty_tests --parallel
```

Otras distribuciones: en Fedora usa `dnf` con los paquetes equivalentes; en NixOS hay un
`shell.nix` con todo (Clang, CMake, Ninja, Qt 6, Vulkan y SDL3).

---

## Uso

Al abrir el **launcher** por primera vez, añade carpetas de juegos en los ajustes globales. El
launcher busca de forma recursiva directorios con `eboot.bin` y dumps ZArchive (`.zar`). Selecciona
un juego y ejecútalo.

También puedes lanzar el emulador directamente con un directorio, un ELF o un `.zar`:

```bash
kyty_emulator --game "/games/Ejemplo"        # Linux
kyty_emulator.exe --game "D:\Games\Ejemplo"  # Windows
```

Ejecuta `kyty_emulator --help` para ver las opciones de gráficos, logging, validación, perfilado
y depuración.

---

## Tests

```powershell
cmake --build _Build/windows --target kyty_tests
ctest --test-dir _Build/windows --output-on-failure
```

```bash
cmake --build _Build/linux --target kyty_tests
ctest --test-dir _Build/linux --output-on-failure
```

---

## Licencia y atribuciones

EMUPS5 se distribuye bajo la **GNU General Public License v2** (`GPL-2.0-only`), igual que
KytyPS5. Está basado en el Kyty original (licencia MIT; aviso conservado en
`LICENSES/Kyty-MIT.txt`). Los componentes de terceros mantienen sus licencias.

- [KytyPS5](https://github.com/KytyPS5/KytyPS5) — base de este proyecto.
- [InoriRus/Kyty](https://github.com/InoriRus/Kyty) — proyecto original.
- [shadps4-emu/shadPS4](https://github.com/shadps4-emu/shadPS4) — referencia de memoria/GPU/AVPlayer.
