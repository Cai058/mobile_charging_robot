# Windows 新电脑完整配置说明

最后验证：2026-07-16  
稳定分支：`codex/cubemx-rebuild`  
封版标签：`v3.0.0-cubemx`  
封版提交：`9d9497c`  
目标芯片：`STM32F427IIHx`  
调试器：`ATK ATK-HS-V3-CMSIS-DAP`

## 1. 文档目标

本文档用于在一台完全没有嵌入式开发环境的 Windows 新电脑上，复制当前已经通过真实机器人验证的开发链路。

完成后应得到唯一工作流：

```text
Ctrl+Shift+B
→ CMake Preset 调用 Ninja
→ GNU Arm GCC 生成 Debug ELF/HEX/BIN/MAP

F5
→ 自动再执行 Debug Build
→ Cortex-Debug 启动 xPack OpenOCD
→ OpenOCD 通过 CMSIS-DAP / SWD 连接 STM32F427IIHx
→ GNU GDB 下载同一个 Debug ELF 到 Flash
→ 加载同一个 ELF 的调试符号
→ reset/halt 后运行到 main

继续运行目标
→ ARM LiveWatch 连接 OpenOCD TCL 6666
→ CPU 不停机查看标量、结构体和结构体成员
```

这条链路不使用 Keil、EIDE、ARMCC AXF、pyOCD GDB Server 或 ST-Link。`pyOCD` 只可作为可选的调试器识别工具。

新电脑建议严格按以下顺序执行：

1. 获取 `v3.0.0-cubemx` 或 `codex/cubemx-rebuild`。
2. 安装已验证版本的 CMake、Ninja、GNU Arm Toolchain 和 xPack OpenOCD。
3. 安装四个 VSCode 扩展，禁用 STM32 VS Code Extension。
4. 运行第 5 节的路径预检查。
5. 执行 Debug/Release 首次干净构建。
6. 连接 CMSIS-DAP 和目标板，按 F5 完成烧录与调试。
7. 在同一 OpenOCD 会话中打开 ARM LiveWatch。
8. 按第 14 节逐项验收。

## 2. 当前已验证的软件版本

| 组件 | 当前版本 | 用途 |
| --- | --- | --- |
| Windows | 64 位 Windows，PowerShell 5.1 | 开发主机 |
| Git for Windows | 2.49.0 | 获取与管理代码 |
| VSCode | 1.129.0 x64 | 编辑、构建和调试入口 |
| CMake | 4.3.4 | 配置和驱动构建 |
| Ninja | 1.13.2 | CMake 生成器/实际构建执行器 |
| Arm GNU Toolchain | 14.2.Rel1，GCC 14.2.1 | 编译 C/ASM、链接、objcopy、size |
| GNU GDB | 15.2.90，随 Arm GNU 14.2.Rel1 安装 | Cortex-Debug 和 LiveWatch 符号解析 |
| xPack OpenOCD | 0.12.0-7 | CMSIS-DAP/SWD GDB Server 和 TCL Server |
| STM32CubeMX | 6.18.0-RC3 | 仅在修改 IOC/重新生成外设初始化时需要 |
| STM32Cube FW_F4 | V1.26.2 | CubeMX 生成代码所用 HAL/CMSIS 固件包 |
| pyOCD | 0.44.1，可选 | 检查 CMSIS-DAP 是否能被 Windows 识别 |

VSCode 扩展的已验证版本：

| 扩展 | ID | 版本 |
| --- | --- | --- |
| CMake Tools | `ms-vscode.cmake-tools` | 1.23.52 |
| C/C++ | `ms-vscode.cpptools` | 1.33.4 |
| Cortex-Debug | `marus25.cortex-debug` | 1.12.1 |
| ARM LiveWatch | `misaka21.arm-livewatch` | 0.1.8 |

`stmicroelectronics.stm32-vscode-extension` 不属于日常构建链路。当前电脑虽安装了 3.9.0，但已在该工作区禁用。

## 3. 获取正确的代码版本

仓库：

```text
https://github.com/Cai058/mobile_charging_robot.git
```

先克隆仓库：

```powershell
git clone https://github.com/Cai058/mobile_charging_robot.git
cd mobile_charging_robot
```

要严格复现已验证封版，使用：

```powershell
git switch --detach v3.0.0-cubemx
git log -1 --oneline --decorate
```

应看到提交 `9d9497c` 和标签 `v3.0.0-cubemx`。

要继续在阶段 3 分支上开发，使用：

```powershell
git switch codex/cubemx-rebuild
```

不要切换到 `master` 后按本文档配置。`master` 保留的是 Keil/ARMCC 基线，文件结构和本文档不同。

仓库中路径较长，建议在新电脑先启用 Git 长路径支持：

```powershell
git config --global core.longpaths true
```

## 4. 安装基础工具

### 4.1 Git、VSCode、CMake 和 Ninja

可使用 WinGet 安装：

```powershell
winget install --id Git.Git -e
winget install --id Microsoft.VisualStudioCode -e
winget install --id Kitware.CMake -e --version 4.3.4
winget install --id Ninja-build.Ninja -e --version 1.13.2
```

安装后重新打开 PowerShell，检查：

```powershell
git --version
code --version
& 'C:/Program Files/CMake/bin/cmake.exe' --version
& "$env:LOCALAPPDATA/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe/ninja.exe" --version
```

当前 `CMakePresets.json` 按 WinGet 的安装目录直接指定 Ninja：

```text
%LOCALAPPDATA%/Microsoft/WinGet/Packages/
Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe/ninja.exe
```

因此推荐用上述 WinGet 方式安装 Ninja，可以不修改仓库配置。

### 4.2 Arm GNU Toolchain 14.2.Rel1

从 Arm 官方下载 Windows 版、`arm-none-eabi` 裸机目标的 14.2.Rel1 安装程序。应选择的包类型是：

```text
mingw-w64-i686-arm-none-eabi
```

不要选 `aarch64-none-elf`、Linux 版或带 Linux sysroot 的工具链。

按安装器默认目录安装，使路径与当前电脑一致：

```text
C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin
```

检查：

```powershell
& 'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-gcc.exe' --version
& 'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-gdb.exe' --version
& 'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-objdump.exe' --version
```

GCC 第一行应包含：

```text
Arm GNU Toolchain 14.2.Rel1
```

### 4.3 xPack OpenOCD 0.12.0-7

下载 xPack OpenOCD `0.12.0-7` Windows x64 压缩包，将完整目录解压到：

```text
E:/download/xpack-openocd-0.12.0-7
```

必须同时存在：

```text
E:/download/xpack-openocd-0.12.0-7/bin/openocd.exe
E:/download/xpack-openocd-0.12.0-7/openocd/scripts/interface/cmsis-dap.cfg
E:/download/xpack-openocd-0.12.0-7/openocd/scripts/target/stm32f4x.cfg
```

检查：

```powershell
& 'E:/download/xpack-openocd-0.12.0-7/bin/openocd.exe' --version
```

当前可执行文件报告的完整版本行为：

```text
xPack Open On-Chip Debugger 0.12.0+dev-02228-ge5888bda3-dirty
```

新电脑没有 E 盘时，可以放到其他位置，但必须同步修改 `.vscode/launch.json` 中的 `serverpath` 和 `searchDir`。

### 4.4 VSCode 扩展

在 VSCode 关闭工作区后，可通过命令行安装已验证版本：

```powershell
code --install-extension ms-vscode.cmake-tools@1.23.52 --force
code --install-extension ms-vscode.cpptools@1.33.4 --force
code --install-extension marus25.cortex-debug@1.12.1 --force
code --install-extension misaka21.arm-livewatch@0.1.8 --force
```

检查：

```powershell
code --list-extensions --show-versions | Select-String -Pattern `
  'ms-vscode.cmake-tools|ms-vscode.cpptools|marus25.cortex-debug|misaka21.arm-livewatch'
```

打开仓库根目录后，VSCode 也会根据 `.vscode/extensions.json` 推荐这四个扩展。

### 4.5 禁用会覆盖 CMake Preset 的扩展

如果已安装 `STM32 VS Code Extension`，必须对当前工作区禁用：

1. 先用 VSCode 打开仓库根目录。
2. 按 `Ctrl+Shift+X` 打开扩展面板。
3. 搜索 `@id:stmicroelectronics.stm32-vscode-extension`。
4. 点击扩展页面中的齿轮或“Disable”旁的下拉菜单。
5. 选择 `Disable (Workspace)`。
6. 执行 `Developer: Reload Window`。

如果新电脑不需要该扩展，直接不安装或全局卸载更简单。

该扩展启用时可能向 VSCode 注入 `cube-cmake`、`cmake.configureArgs` 或 STM32 clangd 设置，导致 CMake Tools 显示 `Override settings applied`，并覆盖本项目的 Preset。

EIDE 扩展 `cl.eide` 也不属于最终工作流，可以不安装或在本工作区禁用。

## 5. 当前配置使用的精确路径

新电脑如果按下表安装，仓库内的 VSCode/CMake 文件不需要修改。

| 工具 | 当前精确路径 | 引用它的文件 |
| --- | --- | --- |
| CMake | `C:/Program Files/CMake/bin/cmake.exe` | `.vscode/settings.json`、`.vscode/tasks.json` |
| Ninja | `$LOCALAPPDATA/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe/ninja.exe` | `CMakePresets.json` |
| GNU Arm bin | `C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin` | `cmake/arm-none-eabi-gcc.cmake`、`.vscode/settings.json`、`.vscode/launch.json` |
| OpenOCD EXE | `E:/download/xpack-openocd-0.12.0-7/bin/openocd.exe` | `.vscode/launch.json` |
| OpenOCD scripts | `E:/download/xpack-openocd-0.12.0-7/openocd/scripts` | `.vscode/launch.json` |
| CubeMX | `E:/download/STM32CubeMX/STM32CubeMX.exe` | 仅 CubeMX 重新生成流程 |
| CubeMX Java | `E:/download/STM32CubeMX/jre/bin/java.exe` | `cubemx/README.md` 中的无头生成命令 |
| F4 固件包 | `C:/Users/Lexi/STM32Cube/Repository/STM32Cube_FW_F4_V1.26.2` | `cubemx/generate-stage1.txt` |

可在仓库根目录运行以下预检查：

```powershell
$paths = @(
  'C:/Program Files/CMake/bin/cmake.exe',
  "$env:LOCALAPPDATA/Microsoft/WinGet/Packages/Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe/ninja.exe",
  'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-gcc.exe',
  'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-gdb.exe',
  'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-objdump.exe',
  'E:/download/xpack-openocd-0.12.0-7/bin/openocd.exe',
  'E:/download/xpack-openocd-0.12.0-7/openocd/scripts/interface/cmsis-dap.cfg',
  'E:/download/xpack-openocd-0.12.0-7/openocd/scripts/target/stm32f4x.cfg'
)

$paths | ForEach-Object {
  [pscustomobject]@{
    Exists = Test-Path -LiteralPath $_
    Path = $_
  }
}
```

所有 `Exists` 都应为 `True`。

## 6. 仓库中必须保留的配置文件

新电脑不应使用扩展向导重新生成这些文件。应直接使用仓库中已经验证的版本：

```text
CMakeLists.txt
CMakePresets.json
cmake/arm-none-eabi-gcc.cmake
linker/STM32F427IIHx_FLASH.ld
startup/startup_stm32f427xx.s
platform/syscalls.c
.vscode/settings.json
.vscode/tasks.json
.vscode/launch.json
.vscode/extensions.json
.vscode/openocd-atk-stm32f427.cfg
```

### 6.1 CMake Preset

`CMakePresets.json` 定义两套配置：

| Preset | 构建类型 | 输出目录 |
| --- | --- | --- |
| `debug` | `Debug`，`-Og -g3` | `build/debug` |
| `release` | `Release`，`-O2 -g0` | `build/release` |

两者均使用：

```text
Generator: Ninja
Toolchain: cmake/arm-none-eabi-gcc.cmake
Target: mobile_charging_robot
CMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### 6.2 GNU Arm 工具链文件

`cmake/arm-none-eabi-gcc.cmake` 会按以下顺序寻找工具链：

1. 环境变量 `ARM_GCC_PATH`。
2. `C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin`。
3. `C:/Program Files/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin`。

如果工具链必须安装在其他位置，可设置：

```powershell
[Environment]::SetEnvironmentVariable(
  'ARM_GCC_PATH',
  'D:/Tools/Arm GNU Toolchain/14.2 rel1/bin',
  'User'
)
```

但这只会解决 CMake 编译器寻找。还必须修改：

- `.vscode/launch.json` 的 `gdbPath` 和 `objdumpPath`。
- `.vscode/settings.json` 的 `livewatch.gdbPath`。

### 6.3 MCU 和链接参数

`CMakeLists.txt` 的当前参数不应在新电脑上改动：

```text
-mcpu=cortex-m4
-mthumb
-mfpu=fpv4-sp-d16
-mfloat-abi=hard
```

预处理定义：

```text
USE_HAL_DRIVER
STM32F427xx
```

链接脚本：

```text
linker/STM32F427IIHx_FLASH.ld
```

当前仍保留 `-fcommon`，用于兼容旧 ARMCC 工程的重复全局变量行为。这是阶段 4 待清理项，新电脑配置时不要提前删除。

### 6.4 VSCode Build 任务

`.vscode/tasks.json` 中的默认 Build 任务是：

```text
CMake: Build Debug ELF
```

执行顺序：

```text
CMake: Configure Debug
→ cmake --preset debug
→ CMake: Build Debug ELF
→ cmake --build --preset debug
```

这使 `Ctrl+Shift+B` 始终构建 `build/debug/mobile_charging_robot.elf`。

Release 任务已定义，但不是默认任务：

```text
CMake: Build Release ELF
```

### 6.5 Cortex-Debug `launch.json`

当前只保留一个调试配置：

```text
STM32F427 - OpenOCD (CMake GCC ELF)
```

当前 `.vscode/launch.json` 的完整配置是：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "STM32F427 - OpenOCD (CMake GCC ELF)",
      "type": "cortex-debug",
      "request": "launch",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/build/debug/mobile_charging_robot.elf",
      "preLaunchTask": "CMake: Build Debug ELF",
      "servertype": "openocd",
      "serverpath": "E:/download/xpack-openocd-0.12.0-7/bin/openocd.exe",
      "searchDir": [
        "E:/download/xpack-openocd-0.12.0-7/openocd/scripts"
      ],
      "configFiles": [
        "${workspaceFolder}/.vscode/openocd-atk-stm32f427.cfg"
      ],
      "gdbPath": "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-gdb.exe",
      "objdumpPath": "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-objdump.exe",
      "runToEntryPoint": "main",
      "showDevDebugOutput": "none"
    }
  ]
}
```

`request: launch` 非常重要。它表示 GDB 会把 ELF 中的 Flash 程序段下载到芯片，而不是只附加到已有程序。

Build、Flash 和调试符号必须使用同一个：

```text
build/debug/mobile_charging_robot.elf
```

不要将 `executable` 改成 Keil/ARMCC 的 `.axf`，也不要先烧录 HEX 后再使用另一个 ELF 加载符号。

### 6.6 OpenOCD 配置

`.vscode/openocd-atk-stm32f427.cfg` 的完整内容是：

```tcl
source [find interface/cmsis-dap.cfg]
cmsis-dap backend hid
transport select swd
source [find target/stm32f4x.cfg]
adapter speed 1000
tcl port 6666
```

这些参数表示：

- 调试器是 CMSIS-DAP，不是 ST-Link。
- Windows 通过 HID 后端访问调试器。
- 传输接口是 SWD。
- 芯片族使用 OpenOCD 的 `stm32f4x.cfg`。
- SWD 速度固定为 1000 kHz，即 1 MHz。
- TCL 端口固定为 6666，专供 ARM LiveWatch 使用。

OpenOCD 默认还会使用：

```text
GDB: 3333
Telnet: 4444
TCL/LiveWatch: 6666
```

当前配置没有绑定 CMSIS-DAP 序列号，默认新电脑同时只连接一个 CMSIS-DAP 调试器。

### 6.7 ARM LiveWatch

`.vscode/settings.json` 已固定：

```json
{
  "livewatch.elfPath": "${workspaceFolder}/build/debug/mobile_charging_robot.elf",
  "livewatch.gdbPath": "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-gdb.exe",
  "livewatch.openocdHost": "127.0.0.1",
  "livewatch.openocdPort": 6666,
  "livewatch.ramRegions": [
    "0x20000000-0x20030000",
    "0x10000000-0x10010000"
  ],
  "livewatch.pollInterval": 200
}
```

两段 RAM 范围分别是：

- STM32F427 主 SRAM：`0x20000000-0x20030000`。
- CCM RAM：`0x10000000-0x10010000`。

ARM LiveWatch 必须单独指定 `livewatch.gdbPath`。它不会自动复用 Cortex-Debug 的 `gdbPath`。

### 6.8 CMake Tools 和 C/C++ 工作区设置

`.vscode/settings.json` 还固定：

```json
{
  "cmake.useCMakePresets": "always",
  "cmake.configureOnOpen": false,
  "cmake.cmakePath": "C:/Program Files/CMake/bin/cmake.exe",
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/debug/compile_commands.json"
}
```

含义：

- CMake Tools 必须使用仓库的 Preset。
- 打开工作区时不自动 Configure，避免扩展在路径未就绪时写入缓存。
- `Ctrl+Shift+B` 或手动 Configure 后，C/C++ 使用 Debug 的 `compile_commands.json` 完成宏、头文件和编译选项解析。

如果 CMake Tools 要求 `Select a Kit`，不要选本机 MSVC、MinGW 或 STM32 扩展创建的 Kit。本项目的编译器由 `CMakePresets.json` 和 `cmake/arm-none-eabi-gcc.cmake` 确定，应继续使用 Preset 流程。

## 7. 首次构建

用 VSCode 打开的必须是仓库根目录，即同时包含以下文件的目录：

```text
CMakeLists.txt
CMakePresets.json
.vscode/
User/
BSP/
Controller/
```

不要只打开 `User`、`cubemx`或 `build/debug` 子目录。

首次建议在 VSCode 终端执行干净 Debug 配置和构建：

```powershell
& 'C:/Program Files/CMake/bin/cmake.exe' --preset debug --fresh
& 'C:/Program Files/CMake/bin/cmake.exe' --build --preset debug
```

再构建 Release：

```powershell
& 'C:/Program Files/CMake/bin/cmake.exe' --preset release --fresh
& 'C:/Program Files/CMake/bin/cmake.exe' --build --preset release
```

Debug 应生成：

```text
build/debug/mobile_charging_robot.elf
build/debug/mobile_charging_robot.hex
build/debug/mobile_charging_robot.bin
build/debug/mobile_charging_robot.map
build/debug/compile_commands.json
```

Release 应生成同名文件到 `build/release`。

当前 `v3.0.0-cubemx` Debug 固件的已验证大小：

```text
text  = 51,396 bytes
data  =    552 bytes
bss   = 14,848 bytes
```

检查命令：

```powershell
& 'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-size.exe' `
  'build/debug/mobile_charging_robot.elf'
```

当前已上机验证的产物哈希是：

```text
ELF SHA-256: BEC090AE1FAF60974187594A8554179860D51C091E0380DBF1D45F1C35EC60BE
BIN SHA-256: BE2A1DBA4A81338BC7ED406D8EA201789E6C42290D85C4640E4788074FBFA9A6
```

Debug ELF 内会嵌入源码和构建目录路径。如果新电脑的仓库路径不同，ELF 哈希可能不同；这不一定表示 Flash 程序字节不同。应结合 BIN 哈希、`size` 结果和关键符号进行判断。

关键符号检查：

```powershell
& 'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-nm.exe' `
  --defined-only 'build/debug/mobile_charging_robot.elf' |
  Select-String -Pattern '\b(main|Reset_Handler|m_ctrl|m_server_ctrl)$'
```

应能找到 `Reset_Handler`、`main`、`m_ctrl` 和 `m_server_ctrl`。

## 8. VSCode 中的日常编译

只编译 Debug：

1. 在 VSCode 打开仓库根目录。
2. 按 `Ctrl+Shift+B`。
3. 默认任务应是 `CMake: Build Debug ELF`。
4. 等待终端显示构建成功和固件大小。
5. 检查 `build/debug/mobile_charging_robot.elf` 的修改时间。

命令行等价命令：

```powershell
cmake --preset debug
cmake --build --preset debug
```

不要再到已删除的 `MDK-ARM/build/m_robot` 目录寻找 ELF。当前唯一 Debug ELF 位于：

```text
build/debug/mobile_charging_robot.elf
```

## 9. 连接 CMSIS-DAP 调试器

1. 给机器人控制器上电。
2. 确认 CMSIS-DAP 与目标板的 SWDIO、SWCLK、GND 和参考电压已正确连接。
3. 将 `ATK-HS-V3-CMSIS-DAP` 插入新电脑。
4. 同时只保留一个 CMSIS-DAP 调试器。

当前使用 `cmsis-dap backend hid`，Windows 通常使用自带 HID 驱动即可。不要在没有明确需求时用 Zadig 把 CMSIS-DAP HID 接口替换成其他驱动。

可选的 pyOCD 检查：

```powershell
python -m pip install pyocd==0.44.1
python -m pyocd list --probes
```

连接时应类似显示：

```text
ATK ATK-HS-V3-CMSIS-DAP   ATK 20190528
```

`Target` 在 `list --probes` 中显示 `n/a` 不代表调试器有问题；该命令只列出探针，未指定目标芯片。

在不启动 VSCode 调试会话的情况下，可用以下命令单独验证 OpenOCD 和 SWD 连接：

```powershell
& 'E:/download/xpack-openocd-0.12.0-7/bin/openocd.exe' `
  -s 'E:/download/xpack-openocd-0.12.0-7/openocd/scripts' `
  -f '.vscode/openocd-atk-stm32f427.cfg' `
  -c 'init; reset halt; reset run; shutdown'
```

该命令只连接、复位/停机验证、恢复运行并退出，不下载新固件。成功时应识别 Cortex-M4 和 STM32F42x/43x Flash。

## 10. F5 编译、烧录和调试

1. 按 `Ctrl+Shift+D` 打开 Run and Debug。
2. 在顶部选择 `STM32F427 - OpenOCD (CMake GCC ELF)`。
3. 按 `F5`。
4. VSCode 先自动执行 `CMake: Build Debug ELF`。
5. Cortex-Debug 启动 OpenOCD。
6. OpenOCD 通过 CMSIS-DAP/SWD 连接芯片。
7. GNU GDB 下载 `build/debug/mobile_charging_robot.elf` 到 Flash。
8. GDB 从同一 ELF 加载符号。
9. 目标 reset/halt，并运行到 `main()`。

成功后应能：

- 在 `main.c` 看到黄色停止指示。
- 下断点、单步、继续和重置。
- 在 Debug Console 中看到 GDB 已连接 OpenOCD。
- 在 Terminal 的 `gdb-server` 页看到 OpenOCD 识别 STM32F42x/43x Flash。

为使主程序正常运行，停在 `main()` 后再按一次 `F5` 或点击 Continue。

## 11. ARM LiveWatch 实时变量

ARM LiveWatch 依赖 Cortex-Debug 已经启动的 OpenOCD TCL Server。正确顺序是：

1. 先按 `F5` 启动 Cortex-Debug。
2. 让程序 Continue，保持 CPU 运行。
3. 不要终止 Cortex-Debug 会话。
4. 在 VSCode 左侧活动栏打开 `ARM LiveWatch`。
5. 确认它连接 `127.0.0.1:6666`。
6. 点击 `+` 或运行 `ARM LiveWatch: Add Watch Variable`。

已验证可以添加：

```text
m_systick
m_ctrl
m_ctrl.m_soc
m_ctrl.location_id
m_server_ctrl
m_server_ctrl.command
```

`m_ctrl` 和 `m_server_ctrl` 可以直接展开成结构体成员。

每次重新编译 ELF 后，如果符号或结构体显示不更新，运行：

```text
ARM LiveWatch: Re-resolve All Symbols
```

或点击 LiveWatch 中的刷新符号按钮。

## 12. CubeMX 配置与重新生成

### 12.1 普通开发不需要 CubeMX

只要是编译、烧录、调试和 LiveWatch，新电脑不需要安装 CubeMX。根工程直接编译已提交的 `User/Src` 和 `User/Inc` 文件。

### 12.2 必须重新生成时的版本

需要修改芯片引脚、时钟、DMA、NVIC 或外设初始化时，安装：

```text
STM32CubeMX 6.18.0-RC3
STM32Cube FW_F4 V1.26.2
```

当前 CubeMX 路径：

```text
E:/download/STM32CubeMX
```

当前固件包路径：

```text
C:/Users/Lexi/STM32Cube/Repository/STM32Cube_FW_F4_V1.26.2
```

新电脑用户名通常不是 `Lexi`，必须把 `cubemx/generate-stage1.txt` 中的：

```text
C:\Users\Lexi\STM32Cube\Repository\STM32Cube_FW_F4_V1.26.2
```

改成新电脑实际的固件包目录。

### 12.3 IOC 是唯一硬件配置源

唯一有效 IOC：

```text
cubemx/mobile_charging_robot.ioc
```

代码生成目标：

```text
cubemx/generated/mobile_charging_robot_cubemx
```

根目录的 `CMakeLists.txt` 不直接编译 `cubemx/generated/.../Src`。CubeMX 先生成到隔离目录，审查后再通过白名单同步到根工程的稳定路径。

`cubemx/generate-stage1.txt` 中还包含当前电脑仓库的绝对路径：

```text
D:\SUSTech\Research\project\Robot\robot 1st\mobile_charging_robot(vscode)
```

如果新电脑克隆位置不同，在执行无头生成前，必须把该文件中的 `config load`、`project path` 和 `config save` 路径替换为新仓库的绝对路径。

当前无头生成命令：

```powershell
& 'E:/download/STM32CubeMX/jre/bin/java.exe' `
  -jar 'E:/download/STM32CubeMX/STM32CubeMX.exe' `
  -q 'cubemx/generate-stage1.txt'
```

生成后只能用白名单脚本同步已审查的外设，例如 CAN：

```powershell
powershell -ExecutionPolicy Bypass -File cubemx/sync-generated.ps1 -Peripheral can
```

可用的白名单：

```text
can
gpio
usart1_rc
tim2
rfid_uart8
battery_usart6
server_uart7
```

不要直接把 CubeMX 生成目录的整个 `Src`、`Inc` 或 `Drivers` 覆盖到根工程。

## 13. 常见问题

### 13.1 CMake Tools 显示 `Override settings applied`

优先检查 `STM32 VS Code Extension` 是否仍在本工作区启用。禁用并 Reload Window。

再检查 VSCode 的 User Settings 和 Workspace Settings 中是否存在不属于本项目的：

```text
cmake.configureArgs
cmake.generator
cmake.toolchain
cube-cmake
```

本项目只应使用 `CMakePresets.json`。

### 13.2 CMake 找不到 `arm-none-eabi-gcc`

先检查：

```powershell
Test-Path 'C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin/arm-none-eabi-gcc.exe'
```

如果路径不同，修改 `cmake/arm-none-eabi-gcc.cmake` 或设置 `ARM_GCC_PATH`，然后强制刷新缓存：

```powershell
cmake --preset debug --fresh
cmake --build --preset debug
```

### 13.3 CMake 找不到 Ninja

检查 `CMakePresets.json` 中的 `CMAKE_MAKE_PROGRAM` 与实际 `ninja.exe` 位置。

可用：

```powershell
Get-Command ninja -ErrorAction SilentlyContinue
Get-ChildItem "$env:LOCALAPPDATA/Microsoft/WinGet/Packages" -Recurse -Filter ninja.exe
```

如果不是 WinGet 默认路径，只修改 `CMakePresets.json` 中 Debug 和 Release 的 `CMAKE_MAKE_PROGRAM`。

### 13.4 `Ctrl+Shift+B` 后 ELF 没有更新

确认查看的是：

```text
build/debug/mobile_charging_robot.elf
```

然后直接执行：

```powershell
cmake --preset debug --fresh
cmake --build --preset debug
Get-Item build/debug/mobile_charging_robot.elf | Select-Object FullName,LastWriteTime,Length
```

### 13.5 F5 提示找不到 OpenOCD

检查 `.vscode/launch.json` 的：

```text
serverpath
searchDir
```

`serverpath` 必须直接指向 `openocd.exe`，`searchDir` 必须指向包含 `interface` 和 `target` 子目录的 scripts 目录。

### 13.6 OpenOCD 找不到 CMSIS-DAP

检查：

- 目标板已上电。
- USB 线支持数据，不是只充电线。
- Windows 设备管理器能看到 ATK/CMSIS-DAP HID 设备。
- 没有 pyOCD、另一个 OpenOCD、Keil 或其他调试会话正在占用探针。
- 同时只插入一个 CMSIS-DAP。

可用：

```powershell
python -m pyocd list --probes
```

辅助判断 Windows 是否能看到探针。

### 13.7 OpenOCD 或 LiveWatch 端口被占用

检查：

```powershell
Get-NetTCPConnection -State Listen -LocalPort 3333,4444,6666 -ErrorAction SilentlyContinue |
  Select-Object LocalPort,OwningProcess
```

正常情况下，只有当 Cortex-Debug 会话运行时才应由该会话的 OpenOCD 监听这些端口。终止调试会话后，端口应释放。

### 13.8 F5 出现 `RW_IRAM1 outside of ELF segments`

这通常表示错误地使用了 ARMCC/Keil AXF。

正确文件必须是：

```text
build/debug/mobile_charging_robot.elf
```

不要使用：

```text
MDK-ARM/build/m_robot/m_robot.axf
```

当前分支已经删除 `MDK-ARM`，这也是为了避免新电脑选错产物。

### 13.9 F5 在 Flash 下载时失败

先在 Cortex-Debug 终止后确认没有第二个 GDB Server。再检查：

- `launch.json` 使用 `request: launch`。
- `executable` 是 CMake/GCC ELF。
- OpenOCD 识别到 STM32F42x/43x Flash。
- SWD 速度保持当前已验证的 1 MHz。
- 目标板供电稳定。

如需详细 GDB 日志，可临时把 `launch.json` 的：

```json
"showDevDebugOutput": "none"
```

改为：

```json
"showDevDebugOutput": "raw"
```

问题排除后恢复为 `none`，避免日志过多。

### 13.10 ARM LiveWatch 无法连接

检查顺序：

1. Cortex-Debug/OpenOCD 会话必须正在运行。
2. `.vscode/openocd-atk-stm32f427.cfg` 包含 `tcl port 6666`。
3. `.vscode/settings.json` 的 `livewatch.openocdPort` 为 6666。
4. `livewatch.gdbPath` 指向 GNU Arm 14.2 的真实 `arm-none-eabi-gdb.exe`。
5. `livewatch.elfPath` 指向当前 Debug ELF。
6. 如果刚刚重新构建，刷新 LiveWatch 符号。

### 13.11 Cortex-Debug 只有暂停时才能看变量

这是标准 GDB Debug 视图的正常行为。VSCode 的 Variables/Watch 由 GDB 读取，CPU 运行时通常不会实时刷新。

要在 CPU 运行时查看变量，必须使用本项目配置的 ARM LiveWatch，而不是 Cortex-Debug 自带的 Watch 面板。

## 14. 最终验收清单

新电脑的 Agent 只有完成以下所有项，才可以认为配置完成。

### 工具和文件

- [ ] CMake、Ninja、GNU Arm GCC/GDB 和 OpenOCD 路径全部存在。
- [ ] VSCode 安装 CMake Tools、C/C++、Cortex-Debug 和 ARM LiveWatch。
- [ ] STM32 VS Code Extension 未安装，或已对当前工作区禁用。
- [ ] 打开的是仓库根目录。
- [ ] 当前代码来自 `codex/cubemx-rebuild` 或 `v3.0.0-cubemx`，不是 `master`。

### 构建

- [ ] `cmake --preset debug --fresh` 成功。
- [ ] `cmake --build --preset debug` 成功。
- [ ] `cmake --build --preset release` 成功。
- [ ] `build/debug` 生成 ELF、HEX、BIN、MAP 和 `compile_commands.json`。
- [ ] ELF 中包含 `Reset_Handler`、`main`、`m_ctrl` 和 `m_server_ctrl`。
- [ ] `Ctrl+Shift+B` 能更新 Debug ELF。

### 烧录和调试

- [ ] Windows 能识别 `ATK-HS-V3-CMSIS-DAP`。
- [ ] F5 自动执行 Debug Build。
- [ ] Cortex-Debug 能启动 OpenOCD。
- [ ] OpenOCD 能识别 STM32F427IIHx/STM32F42x/43x Flash。
- [ ] GDB 能下载同一个 CMake Debug ELF。
- [ ] 程序能停在 `main()`。
- [ ] 断点、Continue、单步和 Reset 正常。

### LiveWatch

- [ ] OpenOCD 正在监听 TCL 6666。
- [ ] ARM LiveWatch 显示 Connected。
- [ ] CPU 运行时 `m_systick` 持续变化。
- [ ] `m_ctrl` 可以展开并查看各成员。
- [ ] `m_server_ctrl` 可以展开并查看各成员。
- [ ] 终止 Cortex-Debug 后，3333、4444 和 6666 端口已释放。

## 15. 给新电脑 Agent 的执行原则

1. 先检查工具版本和绝对路径，再运行 CMake。
2. 优先按当前路径安装工具，尽量不修改已验证的仓库配置。
3. 如必须改路径，只改本文档第 5 节列出的路径字段。
4. 不要使用 CMake Tools、STM32 扩展或 CubeMX 向导覆盖根工程的 `CMakeLists.txt`、Preset 和 `launch.json`。
5. 不要把 Build、Flash 和符号拆成不同产物；三者必须统一使用 `build/debug/mobile_charging_robot.elf`。
6. 不要将 Keil AXF 放入 Cortex-Debug 的 `executable`。
7. 不要用 pyOCD 替换当前 OpenOCD 主链路。
8. 修改 CubeMX 配置时，必须隔离生成并通过白名单同步，不得整目录覆盖正式源码。
9. 配置完成后，必须按第 14 节逐项验收，不得只以“能编译”作为完成标准。
