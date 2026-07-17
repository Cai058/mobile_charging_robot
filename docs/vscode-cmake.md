# VSCode / CMake / OpenOCD 标准开发流程

最后更新：2026-07-16

全新 Windows 电脑的工具安装、路径复制、VSCode 扩展、首次 Build/F5/LiveWatch 验收和排错步骤，见 `docs/windows-cmake-vscode-setup.md`。

## 目标

阶段 2 收口后只保留一条日常开发主链路：

```text
CMake Preset
→ GNU Arm GCC生成标准 ELF
→ Cortex-Debug以 launch 模式启动 OpenOCD
→ OpenOCD通过 CMSIS-DAP / SWD连接 STM32F427IIHx
→ GNU GDB将同一个 ELF下载到 Flash并加载调试符号
→ reset / halt，并运行到 main
→ ARM LiveWatch通过 OpenOCD TCL端口实时读取变量
```

日常开发不再使用 EIDE、ARMCC AXF、pyOCD Flash或“先烧录 HEX 再 attach”的分离流程。

该流程已于 2026-07-14 在真实机器人上完成整机验证。当前稳定版本保存在 `codex/cmake-vscode` 分支，并使用 `v2.0.0-cmake-vscode` 标签封版；远程 `master` 继续保留 Keil/ARMCC 稳定版本。

## 唯一工作区入口

使用 VSCode打开仓库根目录 `mobile_charging_robot(vscode)`。

当前 CubeMX/CMake 分支不再包含 `MDK-ARM`。完整 Keil 工程和 ARMCC 回退版本保留在远程 `master` 分支；日常工作只打开仓库根目录。

最终配置统一位于：

```text
.vscode/tasks.json
.vscode/launch.json
.vscode/settings.json
.vscode/extensions.json
.vscode/openocd-atk-stm32f427.cfg
CMakeLists.txt
CMakePresets.json
cmake/arm-none-eabi-gcc.cmake
```

## 必需软件和插件

软件：

- GNU Arm Embedded Toolchain 14.2.Rel1
- CMake 4.3.4
- Ninja 1.13.2
- xPack OpenOCD 0.12.0-7
- ATK-HS-V3-CMSIS-DAP

VSCode插件：

- CMake Tools：`ms-vscode.cmake-tools`
- C/C++：`ms-vscode.cpptools`
- Cortex-Debug：`marus25.cortex-debug`
- ARM LiveWatch：`misaka21.arm-livewatch`

EIDE：`cl.eide` 不属于最终主链路，可以卸载或在当前工作区禁用。

STMicroelectronics STM32 VSCode Extension也应在当前工作区禁用。它会自动回写 `cube-cmake`、`cmake.configureArgs` 和 STM32 clangd设置，使 CMake Tools显示 `Override settings applied` 并覆盖本项目的 Preset配置。阶段 3在独立 CubeMX工程中再按需启用。

## CMake构建

CMake使用 GCC专用文件：

```text
startup/startup_stm32f427xx.s
linker/STM32F427IIHx_FLASH.ld
platform/syscalls.c
```

目标参数：

```text
-mcpu=cortex-m4
-mthumb
-mfpu=fpv4-sp-d16
-mfloat-abi=hard
```

Debug关键参数：

```text
-Og
-g3
-ffunction-sections
-fdata-sections
-fcommon
```

`-fcommon` 暂时用于兼容旧 ARMCC工程，阶段 4清理重复全局变量后移除。

## Build

在 VSCode中按 `Ctrl+Shift+B`，默认任务为：

```text
CMake: Build Debug ELF
```

命令行等价操作：

```powershell
cmake --preset debug
cmake --build --preset debug
```

需要完全刷新缓存时：

```powershell
cmake --preset debug --fresh
cmake --build --preset debug
```

Debug输出：

```text
build/debug/mobile_charging_robot.elf
build/debug/mobile_charging_robot.hex
build/debug/mobile_charging_robot.bin
build/debug/mobile_charging_robot.map
```

Cortex-Debug和 ARM LiveWatch统一使用 `build/debug/mobile_charging_robot.elf`。

## OpenOCD

统一配置：

```text
interface/cmsis-dap.cfg
cmsis-dap backend hid
transport select swd
target/stm32f4x.cfg
adapter speed 1000
tcl port 6666
```

GDB端口由 Cortex-Debug管理；TCL固定为 `127.0.0.1:6666`，供 ARM LiveWatch使用。

## Cortex-Debug

`launch.json` 只保留：

```text
STM32F427 - OpenOCD (CMake GCC ELF)
```

要求：

- `request` 为 `launch`。
- `executable` 指向 CMake Debug ELF。
- `servertype` 为 `openocd`。
- F5前自动执行 `CMake: Build Debug ELF`。
- GDB直接下载同一个 ELF并加载同一 ELF符号。
- 下载后 reset/halt，并运行到 `main()`。

## ARM LiveWatch

固定配置：

```text
ELF: build/debug/mobile_charging_robot.elf
GDB: GNU Arm 14.2 arm-none-eabi-gdb.exe
OpenOCD: 127.0.0.1:6666
RAM: 0x20000000-0x20030000
CCMRAM: 0x10000000-0x10010000
```

LiveWatch应能实时显示标量，并展开 `m_ctrl`、`m_server_ctrl` 等结构体。

## 最终日常操作

### 只编译

1. 打开仓库根目录。
2. 按 `Ctrl+Shift+B`。
3. 确认 `build/debug/mobile_charging_robot.elf` 已更新。

### 编译、烧录和调试

1. 选择 `STM32F427 - OpenOCD (CMake GCC ELF)`。
2. 按 `F5`。
3. VSCode自动执行 CMake Debug Build。
4. Cortex-Debug启动 OpenOCD。
5. GNU GDB下载 ELF并加载符号。
6. 程序停在 `main()` 后进行断点、单步或继续运行。

### 实时查看变量

1. 保持 OpenOCD调试会话运行。
2. 让目标程序继续运行。
3. 打开 ARM LiveWatch。
4. 确认状态为 `Connected`。
5. 添加标量、结构体或结构体成员。

## 验收标准

- [x] CMake Debug和 Release使用 GNU Arm GCC 14.2构建成功。
- [x] Debug Build生成标准 `mobile_charging_robot.elf`。
- [x] ELF包含 Flash、初始化数据和 RAM的正确 LOAD映射。
- [x] GDB能解析 `Reset_Handler`、`main`、`m_ctrl` 和 `m_server_ctrl`。
- [x] Build、Debug和 LiveWatch统一使用同一个 CMake ELF。
- [x] 调试器重新连接后，验证 GDB直接下载 CMake ELF。
- [x] 验证复位、断点和单步调试。
- [x] 验证 ARM LiveWatch连接 `127.0.0.1:6666`。
- [x] 验证 ARM LiveWatch实时显示标量和结构体。
- [x] 完整流程不需要手工修改路径、端口或烧录文件。
- [x] 在真实机器人上完成 CAN、电机、串口、DMA、中断、传感器、执行器和充电相关模块检测。

阶段 2 已正式完成。后续日常开发继续使用本文件定义的唯一工作流；阶段 3 从该稳定基线创建独立 CubeMX 重建分支。

## CubeMX和 Keil

当前 `.ioc` 不是完整硬件配置，不能在现有源码上原地生成。阶段 3在独立分支或目录中重建完整 CubeMX CMake工程。

Keil/AC5工程和可工作的 Keil HEX由远程 `master` 及既有 `v1.4.2-robot` 标签保留，用于行为对照和固件回退；CMake分支不重复跟踪 Keil自动生成目录，也不再把 Keil作为日常构建入口。
