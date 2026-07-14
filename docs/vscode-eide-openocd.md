# 已废弃：EIDE / OpenOCD 流程

本流程于 2026-07-14 废弃。项目最终统一使用 CMake + GNU Arm GCC + OpenOCD + Cortex-Debug + ARM LiveWatch。

请以 `docs/vscode-cmake.md` 为唯一有效流程文档。

以下内容仅保留为迁移历史，不再实施。

# 原 VSCode / EIDE / OpenOCD 标准开发流程

最后更新：2026-07-14

## 目标

阶段 2 收口后只保留一条日常开发主链路：

```text
EIDE Build（GNU Arm GCC）
→ 生成标准 ELF
→ Cortex-Debug 以 launch 模式启动 OpenOCD
→ OpenOCD 通过 CMSIS-DAP / SWD 连接 STM32F427IIHx
→ GNU GDB 将同一个 ELF 的程序段下载到 Flash
→ GNU GDB 从同一个 ELF 加载调试符号
→ reset / halt，并运行到 main
→ ARM LiveWatch 通过 OpenOCD TCL 端口实时读取变量
```

日常开发不再混用以下路径：

- 不使用 EIDE/ARMCC V5 的 `m_robot.axf` 作为 GNU GDB 的下载文件。
- 不使用 pyOCD 作为日常烧录或调试后端。
- 不使用独立 HEX 预烧录再 attach 的分离流程。
- 不使用 CMake Build 作为日常 EIDE 开发入口。

CMake/GCC 工程继续保留，作为 EIDE GCC 迁移的已验证配置参考、独立构建后备和后续 CubeMX 迁移基础。

## 唯一工作区入口

使用 VSCode 打开：

```text
MDK-ARM/m_robot.code-workspace
```

或直接打开 `MDK-ARM` 文件夹。

EIDE 项目文件位于：

```text
MDK-ARM/.eide/eide.yml
```

最终 VSCode 调试和 LiveWatch 配置统一位于：

```text
MDK-ARM/.vscode/launch.json
MDK-ARM/.vscode/settings.json
MDK-ARM/.vscode/openocd-atk-stm32f427.cfg
```

## 必需软件

- GNU Arm Embedded Toolchain 14.2.Rel1
- xPack OpenOCD 0.12.0-7
- STM32F4xx DFP 2.13.0
- ATK-HS-V3-CMSIS-DAP
- STM32F427IIHx 机器人控制器

## 必需 VSCode 插件

### 主链路必需

- Embedded IDE：`cl.eide`，当前验证版本 3.27.2
- Cortex-Debug：`marus25.cortex-debug`，当前验证版本 1.12.1
- ARM LiveWatch：`misaka21.arm-livewatch`，当前验证版本 0.1.8
- C/C++：`ms-vscode.cpptools`

### 保留但不进入日常主链路

- CMake Tools：`ms-vscode.cmake-tools`

CMake Tools 用于维护仓库根目录的独立 CMake/GCC 工程，不负责 EIDE 日常 Build。

## EIDE GCC 构建要求

EIDE 必须使用：

```text
GNU Arm Embedded Toolchain (GCC)
```

工具链根目录：

```text
C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1
```

### 目标架构

```text
CPU: Cortex-M4
Instruction set: Thumb
FPU: FPv4-SP-D16 / single precision
Float ABI: hard
```

对应关键 GCC 参数：

```text
-mcpu=cortex-m4
-mthumb
-mfpu=fpv4-sp-d16
-mfloat-abi=hard
```

### Debug 编译参数

```text
-Og
-g3
-ffunction-sections
-fdata-sections
-fcommon
```

`-fcommon` 暂时用于兼容旧 ARMCC 工程的全局变量定义方式，阶段 4 清理重复定义后移除。

### GCC 专用文件

EIDE GCC 构建必须使用：

```text
../startup/startup_stm32f427xx.s
../linker/STM32F427IIHx_FLASH.ld
../platform/syscalls.c
```

EIDE GCC 构建不得继续使用：

```text
MDK-ARM/startup_stm32f427xx.s
build/m_robot/m_robot.sct
MicroLIB
ARMCC 专用编译参数
```

### 预期输出

EIDE Build 成功后必须生成：

```text
MDK-ARM/build/m_robot/m_robot.elf
```

可以同时生成 HEX、BIN 和 MAP，但 Cortex-Debug 与 ARM LiveWatch 必须统一使用同一个 `m_robot.elf`。

## OpenOCD 配置

OpenOCD 使用：

```text
interface/cmsis-dap.cfg
target/stm32f4x.cfg
transport select swd
adapter speed 1000
```

端口约定：

```text
GDB：由 Cortex-Debug 管理
TCL：127.0.0.1:6666
```

TCL 端口 `6666` 专供 ARM LiveWatch 使用。

pyOCD 只保留用于以下诊断：

```powershell
python -m pyocd list --probes
python -m pyocd commander -u "ATK 20190528" -t stm32f427iihx
```

## Cortex-Debug launch 流程

`launch.json` 只保留一个日常配置：

```text
STM32F427 - OpenOCD (EIDE GCC ELF)
```

必须满足：

- `request` 为 `launch`，不是 `attach`。
- `executable` 指向 EIDE 生成的 `build/m_robot/m_robot.elf`。
- `servertype` 为 `openocd`。
- 调试开始时由 GNU GDB 直接执行 ELF 下载。
- 同一个 ELF 同时提供程序段和 DWARF 调试符号。
- 下载后 reset/halt，并运行到 `main()`。

## ARM LiveWatch

ARM LiveWatch 必须显式指定：

```text
ELF: MDK-ARM/build/m_robot/m_robot.elf
GDB: GNU Arm 14.2 arm-none-eabi-gdb.exe
OpenOCD: 127.0.0.1:6666
```

STM32F427 RAM 范围：

```text
0x20000000-0x20030000
0x10000000-0x10010000
```

已知需要实时查看的结构体包括：

```text
m_ctrl
m_server_ctrl
```

LiveWatch 必须能够展开结构体，并在 CPU 持续运行时更新成员值。

## 最终日常操作

1. 使用 VSCode 打开 `MDK-ARM/m_robot.code-workspace`。
2. 在 EIDE 面板点击 Build。
3. 确认生成 `build/m_robot/m_robot.elf`，且构建时间已更新。
4. 在“运行和调试”中选择 `STM32F427 - OpenOCD (EIDE GCC ELF)`。
5. 点击 Debug 或按 `F5`。
6. Cortex-Debug 启动 OpenOCD并由 GDB下载 ELF。
7. 程序停在 `main()` 后，按继续运行。
8. 打开 ARM LiveWatch，确认状态为 `Connected`。
9. 添加标量、结构体或结构体成员并实时观察。

## 验收标准

阶段 2 只有在以下项目全部满足后才正式完成：

- [x] EIDE Build 使用 GNU Arm GCC 14.2，而不是 AC5/AC6。
- [x] EIDE Build 无错误生成标准 `m_robot.elf`。
- [x] ELF 包含 Flash、初始化数据和 RAM 段的正确 LOAD 映射。
- [x] Cortex-Debug 只保留一个 OpenOCD `launch` 配置。
- [ ] GDB 能直接下载同一个 EIDE ELF，不再出现 `RW_IRAM1 outside of ELF segments`。
- [ ] 下载完成后可以复位、停机、断点和单步调试。
- [ ] ARM LiveWatch 能连接 `127.0.0.1:6666`。
- [ ] ARM LiveWatch 能实时显示标量变量。
- [ ] ARM LiveWatch 能展开并实时显示 `m_ctrl`、`m_server_ctrl` 等结构体。
- [ ] 从 Build 到 Debug 再到 LiveWatch 不需要手工修改路径、端口或烧录文件。

2026-07-14 离线验证结果：

- EIDE 3.27.2 已使用 GNU Arm Toolchain 14.2.Rel1 完整重建 50 个 C 文件和 1 个 GCC 汇编启动文件。
- 已生成 `build/m_robot/m_robot.elf`、`.hex`、`.bin` 和 `.map`。
- EIDE GCC ELF 的 Flash 占用为 54,332 bytes，RAM 占用为 15,712 bytes。
- ELF 包含 Flash、初始化数据和 BSS/RAM 的完整 LOAD 映射。
- GNU GDB 能识别 `Reset_Handler`、`main`、`m_ctrl` 和 `m_server_ctrl`，并能解析两个结构体的完整类型。
- OpenOCD 配置文件已在无探针模式下完成语法和路径解析。

由于验证时未连接调试器，带硬件的下载、复位、断点、单步和 ARM LiveWatch 实时读取保留到探针重新连接后完成。

## 与 Keil 工程的关系

Keil/AC5 工程和可工作的 Keil HEX 继续保留为行为参考与回退固件。

EIDE 中的 AC5 配置应保留在 `toolchainConfigMap` 中，便于必要时切回原编译器；日常目标配置切换到 GCC 后，不删除 Keil 工程和芯片包。
