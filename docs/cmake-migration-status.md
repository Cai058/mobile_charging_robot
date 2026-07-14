# VSCode/CMake 迁移状态

最后更新：2026-07-14

## 目标

在不改变当前机器人硬件初始化和业务行为的前提下，将现有 Keil 工程迁移为可在 VSCode 中使用 CMake 和 GNU Arm Embedded Toolchain 编译的工程。

迁移分为两个阶段：

1. 先让现有代码在 CMake/GCC 下稳定编译，继续保留 BSP 手工初始化。
2. 再单独重建完整 STM32CubeMX 配置，逐项把硬件初始化收归 CubeMX 管理。

## 版本管理策略

- `master`：保留远程仓库现有 Keil/ARMCC 稳定版本，不把 CMake、VSCode 或后续 CubeMX 重建工作直接提交到该分支。
- `codex/cmake-vscode`：阶段 1 和阶段 2 已完成上机验收的 CMake、GNU Arm GCC、VSCode、OpenOCD 和 ARM LiveWatch 稳定基线。
- `codex/cubemx-rebuild`：阶段 3 使用的开发分支，从 `codex/cmake-vscode` 创建，按外设逐项重建完整 CubeMX 配置。
- `v2.0.0-cmake-vscode`：阶段 1/2 封版标签，用于快速回到当前已验证的软件与工具链状态。

Keil 基线和 CMake 基线长期分开维护。阶段 3、4 完成前，不把 CMake/CubeMX 分支合并回 `master`。

## 本次已完成

### 开发工具

已在当前电脑安装并验证：

- GNU Arm Embedded Toolchain 14.2.Rel1
- CMake 4.3.4
- Ninja 1.13.2
- xPack OpenOCD 0.12.0-7
- pyOCD 0.44.1（用于探针和目标芯片连接诊断）
- Cortex-Debug 1.12.1
- ARM LiveWatch 0.1.8

### CMake 构建系统

新增了以下文件：

- `CMakeLists.txt`：按照当前 Keil 工程的有效源码列表组织编译。
- `CMakePresets.json`：提供 Debug 和 Release 两套构建配置。
- `cmake/arm-none-eabi-gcc.cmake`：GNU Arm 工具链配置。
- `linker/STM32F427IIHx_FLASH.ld`：STM32F427IIHx GCC 链接脚本。
- `startup/startup_stm32f427xx.s`：ST 官方 GCC 启动文件。
- `platform/syscalls.c`：Newlib 的堆、标准输入输出和系统调用适配。
- `.gitignore`：忽略 CMake 的 `build` 目录。

CMake 当前严格复用 Keil 工程中的源码集合，没有把尚未加入 Keil 工程的 `APP` 源文件自动加入构建。

### VSCode 配置

新增或更新：

- `.vscode/tasks.json`：Debug/Release 配置和构建任务。
- `.vscode/extensions.json`：推荐 CMake Tools、C/C++ 和 Cortex-Debug。
- `.vscode/settings.json`：使用 CMake Presets 和 `compile_commands.json`。
- `.vscode/tasks.json`：将默认 Build 任务统一为 CMake Debug ELF构建。
- `.vscode/launch.json`：只保留 `STM32F427 - OpenOCD (CMake GCC ELF)` 调试配置。
- `.vscode/settings.json`：固定 CMake Preset、GNU工具路径，以及 ARM LiveWatch 使用的 CMake ELF、OpenOCD端口和 STM32F427 RAM范围。
- `.vscode/openocd-atk-stm32f427.cfg`：统一 CMSIS-DAP、SWD、STM32F4、1 MHz 和 TCL端口 `6666`。
- `docs/vscode-cmake.md`：记录阶段 2 最终唯一工作流、插件、配置和验收标准。

当前使用 CMake Tools 1.23.x。日常 Build和 Debug均从仓库根目录执行，并显式使用本机 CMake与 CMake Preset，不依赖 STM32扩展写入的 `cube-cmake` 覆盖项。Debug/Release构建预设均明确绑定到 `mobile_charging_robot` 目标。

重启 VSCode 后，可以使用 `Ctrl+Shift+B` 执行 Debug 构建，也可以运行：

```powershell
cmake --preset debug
cmake --build --preset debug
```

详细使用方式见 `docs/vscode-cmake.md`。

阶段 2 最终工作流采用与 Keil 接近的单一 ELF 流程：

1. CMake 使用 GNU Arm 14.2 生成 `build/debug/mobile_charging_robot.elf`。
2. Cortex-Debug 启动 OpenOCD。
3. OpenOCD 通过 `ATK-HS-V3-CMSIS-DAP` 和 SWD 连接 STM32F427IIHx。
4. GNU GDB 连接 OpenOCD，并将同一个 ELF 的程序段下载到 Flash。
5. GNU GDB 从同一个 ELF 加载符号，复位并停机后运行到 `main()`。
6. ARM LiveWatch 通过 OpenOCD TCL 端口 `6666` 持续读取 RAM，不需要暂停 CPU。

OpenOCD 当前使用：

- 接口配置：`interface/cmsis-dap.cfg`
- 芯片配置：`target/stm32f4x.cfg`
- SWD 速度：1 MHz
- GDB Server：由 Cortex-Debug 管理
- TCL Server：固定为 `127.0.0.1:6666`，供 ARM LiveWatch 使用

### 编译验证

Debug 和 Release 均已使用 GNU Arm 14.2 完整编译成功，并生成：

- `mobile_charging_robot.elf`
- `mobile_charging_robot.hex`
- `mobile_charging_robot.bin`
- `mobile_charging_robot.map`

2026-07-14 阶段 1/2 封版前执行 Debug 和 Release `--fresh` 干净构建，`arm-none-eabi-size` 结果为：

- Debug：text 53,396 bytes，data 552 bytes，bss 15,136 bytes；Flash 镜像 53,952 bytes。
- Release：text 52,668 bytes，data 556 bytes，bss 15,132 bytes；Flash 镜像 53,228 bytes。
- 两套构建均未使用 CCM RAM。

已检查固件入口：

- 初始栈地址：`0x20030000`
- Reset Vector：`0x08000D41`
- Reset Handler、`main()` 以及 USART1、USART6、UART7、UART8 和对应 DMA 中断均已链接进入固件。

2026-07-13 重新配置并构建 GCC Debug ELF 成功，资源占用为：

- Flash：54,336 bytes / 2 MB
- RAM：15,712 bytes / 192 KB
- CCM RAM：0 bytes / 64 KB

2026-07-14 曾验证 EIDE也可以切换到 GNU Arm GCC 14.2并生成标准 ELF：

- `MDK-ARM/build/m_robot/m_robot.elf`
- `MDK-ARM/build/m_robot/m_robot.hex`
- `MDK-ARM/build/m_robot/m_robot.bin`
- `MDK-ARM/build/m_robot/m_robot.map`

EIDE GCC实验构建资源占用：

- Flash：54,332 bytes / 2 MB
- RAM：15,712 bytes / 192 KB
- CCM RAM：0 bytes / 64 KB

该实验确认 EIDE GCC ELF具有完整 LOAD映射，但最终决定不维护 EIDE和 CMake两套构建配置。日常主链路统一回到 CMake；EIDE实验配置不再作为阶段 2交付物。

### 上板下载、调试和实时变量验证

已完成以下实机验证：

- pyOCD 和 OpenOCD 均能识别 `ATK-HS-V3-CMSIS-DAP`。
- SWD 能读取 STM32F427IIHx，DAP IDCODE 为 `0x2BA01477`。
- OpenOCD 能识别 Cortex-M4 r0p1、6 个硬件断点和 4 个硬件观察点。
- CMake/GCC 生成的固件已成功烧录到真实机器人控制器。
- Cortex-Debug 能使用 OpenOCD 和 GNU GDB 启动调试、加载 ELF 符号并停在源码位置。
- ARM LiveWatch 已成功连接 OpenOCD，并能在 CPU 运行时实时查看全局标量、结构体及结构体成员。
- 已验证 `Controller/src/ControllerDummy.c` 中的 `m_ctrl`、`m_server_ctrl` 等结构体变量可以由 ARM LiveWatch 正确解析和显示。

ARM LiveWatch 必须单独指定 GNU GDB 14.2 的绝对路径。插件不会自动复用 Cortex-Debug 的 `gdbPath`，并且其默认 Windows PATH 查找存在兼容性问题。

### CubeMX 配置盘点

确认当前 `RM_Robot_on_A_no_os.ioc` 不是完整硬件配置。

`.ioc` 当前主要包含：

- CAN1、CAN2
- USART1 和部分 DMA
- TIM2
- 时钟、SWD 和少量 GPIO

BSP 代码还手工配置了：

- USART6：电池通信
- UART7：服务器通信
- UART8：RFID 通信
- TIM4、TIM5：超声波
- 其他执行器、限位开关、光电门、RGB 和电源 GPIO

详细资源和迁移顺序见 `docs/cubemx-rebuild-checklist.md`。

## 阶段 1 和阶段 2 封版结论

2026-07-14 已在真实机器人上完成最终回归：

- `Ctrl+Shift+B` 能通过 CMake Preset 和 GNU Arm GCC 生成 Debug ELF。
- F5 能自动完成 Build、启动 OpenOCD、连接 CMSIS-DAP、下载同一 ELF、加载符号并进入源码调试。
- reset、halt、断点、继续运行和单步调试均通过。
- ARM LiveWatch 能在 CPU 运行时连接 `127.0.0.1:6666`，实时显示标量、结构体及结构体成员。
- CAN、电机、USART1、USART6、UART7、UART8、DMA、中断、超声波、推杆、限位开关、光电门、RFID、电池通信和充电相关模块均已完成上机检测。
- 日常流程不再需要打开 Keil，也不需要手工切换 ELF、端口或烧录文件。

因此阶段 1“CMake 固件整机行为回归”和阶段 2“VSCode 编译、下载、调试、LiveWatch 链路”正式完成。

## 当前没有做

- 没有使用当前不完整的 `.ioc` 覆盖或重新生成现有源码。
- 没有开始阶段 3 的完整 CubeMX 工程重建。
- 没有修改现有 BSP 的外设初始化归属。
- 没有执行阶段 4 的 GCC 警告、编码、重复全局变量和 `-fcommon` 清理。
- 没有移除远程 `master` 中的 Keil 工程；它继续作为独立稳定基线保留。

## 已知问题

### 旧代码 GCC 警告

现有源码仍有一些警告：

- `Entity/inc/RC.h` 的 ARMCC 风格 `__packed` 写法被 GCC 提示属性位置无效。当前字段布局本身没有额外填充，但后续应统一改为 CMSIS 的 `__PACKED_STRUCT`。
- 部分头文件声明了只应存在于 `.c` 文件中的 `static` 函数。
- `Entity/src/Battery.c` 存在有符号数和无符号数比较警告。

当前构建使用 `-fcommon` 兼容旧 ARMCC 工程的全局变量行为。在清理全局变量定义之后，应考虑移除该选项。

### ARMCC AXF 与 GNU GDB 的兼容性

EIDE 从 Keil 工程导入后，仍可使用 ARMCC V5 生成 `m_robot.axf`。该 AXF 可以由 Keil 正常烧录和调试，但其 `RW_IRAM1` 不在 GNU GDB 可识别的 ELF LOAD 段内，GNU GDB 直接执行 `target-download` 时会失败。

最终调试方案不再使用 ARMCC AXF，而是使用 CMake/GCC生成的标准 `build/debug/mobile_charging_robot.elf`，由同一个 ELF同时承担 Flash下载和调试符号加载。Keil/AC5工程只保留为行为参考和固件回退路径。

### 调试探针信息

- 型号：`ATK-HS-V3-CMSIS-DAP`
- 唯一标识：`ATK 20190528`
- USB VID/PID：`04D8:00DF`
- CMSIS-DAP 调试接口：USB HID
- 调试器附带串口：`COM3`
- 目标芯片：`STM32F427IIHx`

## 下一步计划

### 阶段 1：上板验证当前 CMake 固件（已完成）

当前 CMake/GCC 固件已完成真实机器人整机回归，主要模块行为检测通过，Keil 固件继续作为独立回退基线。

### 阶段 2：VSCode 下载、调试和实时变量查看（已完成）

固定日常路径 `Ctrl+Shift+B → F5 → ARM LiveWatch` 已完成实机验证。Build、Flash、GDB 符号、源码调试和运行时变量查看统一使用 `build/debug/mobile_charging_robot.elf`。

### 阶段 3：重建完整 CubeMX 工程

1. 在独立 Git 分支或独立目录中创建 STM32F427IIHx CMake 工程。
2. 第一轮继续使用 STM32Cube FW_F4 V1.26.2，避免同时升级 HAL。
3. 按 `docs/cubemx-rebuild-checklist.md` 还原时钟、引脚、UART、CAN、TIM、DMA 和 NVIC。
4. 每次只迁移一个外设，并完成上板验证。
5. CubeMX 接管某外设后，从 BSP 中删除该外设的重复 GPIO、时钟、DMA、NVIC 和 Handle 初始化。
6. 确保每个 IRQ Handler 只在一个源文件中定义。

阶段完成标准：完整 `.ioc` 成为硬件配置的唯一来源，BSP 只保留设备协议和业务操作。

### 阶段 4：清理和收口

1. 修复 GCC 警告和非 UTF-8 源文件。
2. 清理头文件中的 `static` 实现和重复全局变量。
3. 移除 `-fcommon`，重新验证链接。
4. 建立 Debug/Release 固件发布和回退流程。
5. CMake/CubeMX 工程稳定后，再决定是否归档 Keil 工程。

## 继续工作前需要的信息

- 当前机器人可正常工作的 Keil 固件版本或 HEX。
- 上板测试时可以安全断开的电机、推杆和充电执行器范围。
