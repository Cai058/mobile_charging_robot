# STM32CubeMX 迁移状态

最后更新：2026-07-16

## 分支与基线

- 开发分支：`codex/cubemx-rebuild`
- 已验证的父分支：`codex/cmake-vscode`
- 已验证的父版本标签：`v2.0.0-cmake-vscode`
- Keil 参考分支：`master`

当前已经通过整机测试的 CMake/GCC 固件是行为基准。在相应迁移切片完成源码审查、编译和上机测试之前，CubeMX 生成的代码不得覆盖已验证的根目录源码。

## 工具与固件包

- STM32CubeMX：`6.18.0-RC3`
- MCU：`STM32F427IIHx`，UFBGA176
- 固件包：`STM32Cube FW_F4 V1.26.2`
- 固件包来源：官方 `STMicroelectronics/STM32CubeF4` 仓库的 `v1.26.2` 标签
- 本地固件包路径：`C:/Users/Lexi/STM32Cube/Repository/STM32Cube_FW_F4_V1.26.2`
- 编译器：GNU Arm GCC
- 生成的构建系统：CMake

CubeMX 6.18.0-RC3 比原 IOC 记录的 6.3.0 更新。本分支只允许它在隔离目录中生成代码；所有生成的初始化和中断代码在合入前都必须与已验证实现进行对比。

旧文件 `RM_Robot_on_A_no_os.ioc` 无法被 CubeMX 6.18.0-RC3 稳定迁移，加载时会出现 `Range [5, 3) out of bounds for length 3` 和 `Mcu.getDie() is null`。因此，本次迁移采用全新的 STM32F427IIHx 工程，并逐项重建配置。2026-07-16 目录清理后，旧 IOC 不再保留在当前分支；需要对照时从远程 `master` 或清理前提交 `8154cca` 获取。

## 隔离目录结构

```text
cubemx/
  mobile_charging_robot.ioc   # 重建后的唯一硬件配置源
  generate-stage1.txt         # 可重复执行的 CubeMX 命令行生成脚本
  generated/                  # 隔离的 CubeMX CMake 生成工程
```

重建过程中不会原地覆盖 `User/`、`BSP/`、`Controller/`、`Entity/` 以及现有 CMake 工作流。

## 迁移切片 1

范围：

1. STM32F427IIHx 和 UFBGA176 封装。
2. HSE 12 MHz，系统时钟 168 MHz。
3. APB1 42 MHz，APB2 84 MHz。
4. PA13/PA14 上的 SWD。
5. PD0/PD1 上的 CAN1，1 Mbit/s。
6. 初始切片曾暂时保留 PB12/PB13 上的 CAN2 配置但不启用；2026-07-16 确认机器人不使用 CAN2 后，已从最终 IOC 和正式源码彻底删除。
7. 旧 IOC 中已有的 GPIO 配置，用于对比和归属盘点。

初始 IOC 中暂时保留 USART1 和 TIM2，以便生成工程与已验证代码进行对比；这两个外设的所有权迁移及 BSP 清理属于后续切片。

## 验收条件

- [x] 从已验证的 CMake 基线创建专用迁移分支。
- [x] 找到 STM32CubeMX，并安装、验证 STM32Cube FW_F4 V1.26.2。
- [x] 在 `cubemx/` 下保存重建后的 IOC，且不修改根目录 IOC。
- [x] 在 `cubemx/generated/` 下生成独立的 CubeMX CMake 工程。
- [x] 确认生成工程使用 STM32F427IIHx、GCC 和固定版本固件包。
- [x] 对比时钟、CAN、GPIO、DMA 和 NVIC，并在本文记录尚未解决的归属问题。
- [x] 编译隔离生成工程。
- [x] 只将 CAN1 迁移切片合入正式固件，CAN2 保持未启用。
- [x] 在删除任何对应 BSP 初始化前完成 CAN1 初始化、过滤器和接收反馈的上机测试。

## 已完成的离线验证

- 生成工程使用 `STM32F427IIHx`、UFBGA176、GNU Arm GCC 和 CMake。
- 生成的 HAL 源码来自固定的 STM32Cube FW_F4 V1.26.2。已检查的 `stm32f4xx_hal.c` SHA-256 为 `666BFDC37EDD048970795C34D180E98F9C512BBFE000B3782B3D2411DA39F35E`，与已验证的根目录固件完全一致。
- 使用 GNU Arm Toolchain 14.2 和 Ninja 成功完成隔离 Debug 编译。
- 修正 CAN Bus-Off 配置后、合入应用代码前的占用：Flash 14,472 字节，RAM 1,904 字节，CCMRAM 0 字节。
- 时钟参数与已验证固件一致：PLLM 6、PLLN 168、PLLP 2、PLLQ 4、APB1 四分频、APB2 二分频、Flash 延迟 5。
- CAN1 和 CAN2 位时序与已验证固件一致：预分频 3、SJW 1 TQ、BS1 9 TQ、BS2 4 TQ，波特率为 1 Mbit/s。

## 正在处理的差异

### CAN

第一次生成的代码将 `AutoBusOff` 设置为 `DISABLE`，而已验证固件的两路 CAN 都使用 `ENABLE`。CubeMX 6.18 在 IOC 中使用参数名 `ABOM` 表示此配置。迁移过程中曾为两路 CAN 固定设置 `ABOM=ENABLE`；最终删除 CAN2 后，只保留 `CAN1.ABOM=ENABLE`。

已验证固件还会显式启用 `CAN_IT_BUSOFF`。迁移过程中曾在两路 CAN 的 USER CODE 区启用该中断；最终删除 CAN2 后，只保留 CAN1 的 BUSOFF 中断设置。再次运行 CubeMX 生成代码后该设置仍然保留。

其余 CAN 初始化字段、CAN GPIO 复用功能、NVIC 优先级以及 TX/RX0 中断处理函数均与已验证实现一致。

2026-07-15 使用 OpenOCD 对当前机器人进行了只读诊断。复位并运行 2 秒后，程序停在 `main.c:100` 的主循环，与当前 CMake ELF 的符号一致。实测结果如下：

- `hcan1.Instance = 0x40006400`，状态为工作态，CAN1 已正常初始化。
- `hcan2.Instance = 0`，状态为 RESET，错误码为 `0x000C0000`，即 `HAL_CAN_ERROR_NOT_INITIALIZED | HAL_CAN_ERROR_NOT_READY`。
- `MX_CAN2_Init()` 因没有调用而被链接器从最终 ELF 中删除。
- `can_filter_init()` 对未初始化的 `hcan2` 调用配置、启动和通知函数时，HAL 只返回错误，因此程序没有崩溃，但 CAN2 实际从未工作。

同时确认 `CAN_FilterTypeDef can_filter_st` 未进行零初始化，而且第一次配置 CAN1 前没有设置 `SlaveStartFilterBank`。实机 CAN1 的 FMR 寄存器中读到 CAN2 过滤器起始分界值为 60，超出双 CAN 合法范围 0–27。当前 CAN1 仍能接收，是因为该异常分界值没有把过滤器组 0 分配给 CAN2，不能把这种偶然行为作为正确配置保留。

第一次修正试验曾在 `can_filter_init()` 前加入 `MX_CAN2_Init()`。烧录后确认两路 CAN 均进入工作态、错误码为 0、CAN1/CAN2 时钟均开启，过滤器分界值为 14，过滤器组 0 和 14 均已激活，说明 CubeMX 生成的 CAN2 初始化本身有效。

但是进一步审查发现，CAN2 接收回调使用 `i = RX_Header.StdId - CAN2_3508_ID1 + 7`，会写入 `Motor_measure[7...]`，而当前 `Motor_measure` 数组实际上只有 4 个元素。一旦 CAN2 收到对应报文就会产生越界写入。历史固件中的 CAN2 从未真正启用，当前业务代码也没有通过 `hcan2` 发送报文，因此不能在未完成数据模型迁移前直接启用 CAN2。

本切片采用的安全方案是只初始化和启动 CAN1；将过滤器结构体清零，并在配置 CAN1 前固定设置 `SlaveStartFilterBank = 14`；CAN1 的 HAL 配置、启动和通知步骤都检查返回值。初期曾保留 CAN2 IOC 配置等待决策，2026-07-16 已确认不用并彻底删除。

由于当前 CAN1 过滤器使用全零掩码，会接收全部标准帧，接收回调还必须检查报文 ID。只有 `0x201`–`0x204` 才允许映射到四元素 `Motor_measure` 数组；其他 ID 直接忽略。最终源码中已不存在 CAN2 接收回调。

正式接管时，将根目录 `User/Src/can.c` 的 CAN Bus-Off USER CODE 与隔离 CubeMX 生成结果同步：在 `HAL_CAN_Init()` 成功后启用 `CAN_IT_BUSOFF`，不再在 MSP 初始化过程中提前启用。CAN1 的时序、GPIO、NVIC 和中断处理保持与生成结果一致。

### CubeMX 生成文件直接编译验证

完成 CAN1 风险收口后，下一步不再手工同步两份 `can.c`，而是让根目录 CMake 直接编译 `cubemx/generated/mobile_charging_robot_cubemx/Src/can.c`。`User/Src/can.c` 暂时保留为回退和差异参考，但不参与正式 ELF 构建。

验证链路固定为：

```text
mobile_charging_robot.ioc
→ CubeMX Generate Code
→ cubemx/generated/.../Src/can.c
→ 根目录 CMake/GNU Arm GCC ELF
→ OpenOCD 烧录
→ CAN1 上机验证
```

本次只直接接入生成的 `can.c`。根工程暂时继续使用 `User/Inc/can.h` 和现有 HAL/CMSIS 驱动；确认生成源文件可以稳定参与完整机器人固件构建后，再决定头文件和其他外设生成文件的接管顺序。CubeMX 代码生成不应在每次普通 Build 时自动执行，只在 IOC 修改后显式执行，以避免每次编译额外增加约 40 秒生成时间。

2026-07-15 已完成该直接编译验证：

- 从 `mobile_charging_robot.ioc` 重新执行 CubeMX Generate Code 成功。
- 生成文件中的 `AutoBusOff = ENABLE` 和 `CAN_IT_BUSOFF` USER CODE 均在重复生成后保留。
- 根目录 CMake 的 `compile_commands.json` 和链接 Map 均确认正式 ELF 编译的是 `cubemx/generated/mobile_charging_robot_cubemx/Src/can.c`，不再编译 `User/Src/can.c`。
- 完整机器人 Debug ELF 构建成功，Flash 约 53 KB，RAM 15,704 字节。
- OpenOCD 编程和 Flash Verify 成功。
- 上机后 `hcan1` 处于工作态且错误码为 0；CAN2 保持未初始化；过滤器分界值为 14，过滤器组 0 正常激活。
- 连续采样确认前三路电机反馈仍在更新。

因此，直接编译实验已经证明 CubeMX 生成的 `can.c` 可以独立替代旧实现。该实验作为正式目录方案的验证依据保留，后续正式工作流采用下面的受控同步方式。

### 正式采用的受控同步目录方案

直接编译隔离目录生成文件的实验已经证明 CubeMX `can.c` 可以参与完整机器人固件构建并通过上机验证。为了让根工程的源码路径、CMake 配置、调试断点和后续 CubeMX 工程重建保持稳定，正式迁移流程进一步调整为“隔离生成、白名单同步、稳定路径编译”：

```text
cubemx/mobile_charging_robot.ioc
→ CubeMX 在 cubemx/generated/ 中生成代码
→ 审查当前外设的生成差异
→ cubemx/sync-generated.ps1 按白名单复制
→ User/Src/<外设>.c
→ 根目录 CMake 使用原有稳定路径编译
```

目录职责如下：

| 目录 | 职责 |
| --- | --- |
| `cubemx/mobile_charging_robot.ioc` | 唯一硬件配置源。 |
| `cubemx/generated/` | CubeMX 隔离生成输出，用于审查和同步，不直接作为根工程长期源码路径。 |
| `User/archive/` | 保存迁移前的旧初始化源码，只用于对比和回退，不参与 CMake 编译。 |
| `User/Src/` | 当前正式固件的稳定源码路径；迁移完成的文件由同步脚本从 CubeMX 输出更新。 |

执行规则：

1. 每次只迁移一个外设。
2. 第一次接管某文件前，把已验证基线版本保存到 `User/archive/`，归档文件之后不再修改。
3. 修改 IOC 并重新生成后，先审查生成差异，再把对应文件加入同步脚本白名单。
4. 禁止直接复制整个 `Src/` 或 `Inc/` 目录，避免覆盖尚未迁移的业务代码和中断处理。
5. 同步后保持根 `CMakeLists.txt` 中的稳定路径，例如 `User/Src/can.c`。
6. 完成 CMake 编译、BIN 差分检查、OpenOCD 烧录和上机回归后，才能接管下一个外设。

CAN1 是第一个采用该正式流程的文件。归档来源固定为已验证基线提交 `440b4c1`（标签 `v2.0.0-cmake-vscode`），同步来源为 `cubemx/generated/mobile_charging_robot_cubemx/Src/can.c`，正式目标为 `User/Src/can.c`。

受控同步方案验证结果：

- 旧 `can.c` 已保存为 `User/archive/can.c`，并通过 `User/archive/README.md` 记录来源和使用规则。
- 归档文件的 Git Blob SHA-1 为 `099d67ed9ced9be25d87c854cfa14d7a93901cab`，与基线提交 `440b4c1:User/Src/can.c` 完全一致。
- `cubemx/sync-generated.ps1 -Peripheral can` 只同步白名单中的 CAN 文件，并在复制后校验 SHA-256。
- 生成端和正式端 `can.c` 的 SHA-256 均为 `C73BC5507B05ACD10223CC862AEC3A00695024D80CC1AADF61ECC5B5EE497A0B`。
- 根 CMake 已恢复编译稳定路径 `User/Src/can.c`，`compile_commands.json` 也确认使用该路径。
- 在其他工作区内容完全相同的条件下，直接编译生成文件和同步后编译得到的 BIN SHA-256 均为 `2441C9DDFEFBD16CD214BF390CEB3CAA8C4F070A64BD7607EB516EFA92745459`。
- 执行 `cmake --build --preset debug --clean-first` 后，52 个目标从零重新编译成功，BIN 哈希仍保持一致，排除了复用旧对象文件造成误判的可能。
- 再次运行同步脚本会报告文件已经一致，不产生重复修改。

由于同步前后 BIN 逐字节相同，而直接编译版本已经完成烧录和 CAN1 上机回归，因此目录切换不需要再次烧录。

### GPIO 归属与启动电平（迁移前审查）

已验证根固件中的 `MX_GPIO_Init()` 当前只开启端口时钟，下面列出的引脚初始化均被注释。部分引脚稍后由 BSP 模块初始化，而 CubeMX 会立即初始化全部引脚，并在切换为输出模式前把所有已配置输出置为低电平。

| 引脚 | CubeMX 中的用途 | 当前归属或观察结果 | 迁移决定 |
| --- | --- | --- | --- |
| PE11、PF14 | 低电平点亮的红色/绿色 LED | 由 `BSP/src/bsp_led.c` 中的 `LED_GPIO_Config()` 初始化 | 已纳入本次迁移；CubeMX 先写低电平，与复位默认状态一致。LED 会保持点亮，直到业务代码主动关闭。 |
| PB2 | 按键输入 | 由 `BSP/src/bsp_key.c` 中的 `Key_GPIO_Config()` 初始化，但没有业务读取调用 | 不属于当前机器人实际使用范围，已从 IOC 和 `Init()` 删除。 |
| PH2–PH5 | `POWER_1` 至 `POWER_4` 输出 | 未找到这四个引脚的有效 BSP 初始化 | 不属于本切片，已从重建 IOC 删除。 |
| PE4、PE5 | 未命名的下拉输出 | 未找到有效 BSP 初始化 | 用途未确认，已从重建 IOC 删除。 |
| PA1 | 旧 IOC 中为下拉输入 | BSP 的 L298N 有效实现实际把 PA0–PA3 作为输出 | 旧配置作废；PA1 与 PA0、PA2、PA3 一起迁移为 L298N 推挽输出。 |

### GPIO 迁移切片 1：实际使用的基础 GPIO

GPIO 迁移范围以 `Controller/src/Chassiscontroller.c` 的 `Init()` 调用链以及对应 BSP `.c` 文件中的有效初始化代码为准，不继续照搬旧 IOC 中用途不明确的引脚。

本切片接管：

| 模块 | 实际引脚 | 模式 |
| --- | --- | --- |
| L298N | PA0–PA3 | 推挽输出、无上下拉、高速，初始低电平。 |
| RGB | PD14、PD15、PH12 | 推挽输出、下拉、高速，初始低电平。 |
| LimitSwitch | PI6、PI7 | 输入、下拉。 |
| Photogate | PI2 | 输入、上拉。 |
| LED | PE11、PF14 | 推挽输出、无上下拉、低速，初始低电平。 |

代码盘点发现：

- `bsp_L298N.h` 中定义的是 PB 引脚，但有效实现 `bsp_L298N.c` 实际初始化和操作 PA0–PA3；迁移以 `.c` 实现为准。
- `bsp_limitSwitch.h` 中定义的是 PB14/PB15，但有效实现实际使用 PI6/PI7；迁移以 `.c` 实现为准。
- Photogate 的旧注释写着 PF0，但有效实现实际使用 PI2。
- RGB 的有效颜色控制使用 PD14、PD15、PH12；`RGB_OFF()` 中的 PD12/PD13 写入与初始化不一致，后续单独清理，本切片不把 PD12/PD13 加入 CubeMX。
- `Key_GPIO_Config()` 虽然在 `Init()` 中被调用，但没有找到 `Get_action()` 的业务调用。按当前机器人实际用途，本切片取消 PB2 按键配置和重复初始化调用。

迁移方案：

1. 把已验证基线中的 `User/Src/gpio.c` 保存到 `User/archive/gpio.c`。
2. 从重建 IOC 中删除用途未确认或未实际使用的 PE4、PE5、PH2–PH5、PB2 和旧 PA1 输入配置。
3. 按上表把实际使用引脚加入 IOC。
4. CubeMX 重新生成后，通过白名单脚本把 `gpio.c` 同步到稳定路径 `User/Src/gpio.c`。
5. 从 `Init()` 中移除 `LED_GPIO_Config()`、`Key_GPIO_Config()`、`Photogate_Config()`、`Limit_Switch_Config()`、`L298N_Config()` 和 `RGB_Config()` 的重复 GPIO 初始化调用，但保留这些 BSP 文件中的业务操作和状态读取函数。
6. 根 CMake 继续编译 `User/Src/gpio.c`，不改变源码路径。

本切片的验收条件：

- [x] 生成的 `gpio.c` 不包含 PE4、PE5、PH2–PH5、PB2 或未使用 PA1 输入配置。
- [x] L298N、RGB、LimitSwitch、Photogate 和 LED 的模式、上下拉、速度与现有有效 BSP 实现一致。
- [x] 所有输出在切换为输出模式前先置为低电平，L298N 保持停止状态。
- [x] 根工程全量编译成功，CAN1 和主动电机控制代码不受影响。
- [x] 上机确认 L298N、RGB、限位开关、光电门、两颗 LED、CAN1 电机反馈和主动电机控制均正常。

2026-07-15 已完成 GPIO 切片的离线实施与验收：

- `User/archive/gpio.c` 的 Git Blob SHA-1 为 `1cccde9c26600fecdea87c87bd4525c71ce0106a`，与基线提交 `440b4c1:User/Src/gpio.c` 完全一致。
- CubeMX 无头生成成功；生成的 `gpio.c` 只包含本切片确定的五类基础 GPIO。
- `cubemx/sync-generated.ps1 -Peripheral gpio` 已加入白名单，并把生成文件同步到稳定路径 `User/Src/gpio.c`。
- 生成端与正式端 `gpio.c` 的 SHA-256 均为 `9106CF6A4269990EA8E61E45FDD15024AF80AA6F1D466B4BD5508D6B712E9226`。
- `Controller/src/Chassiscontroller.c::Init()` 已移除 `LED_GPIO_Config()`、`Key_GPIO_Config()`、`Photogate_Config()`、`Limit_Switch_Config()`、`L298N_Config()` 和 `RGB_Config()`；各 BSP 的业务控制和状态读取函数继续保留。
- `compile_commands.json` 和链接 Map 均确认根工程编译的是 `User/Src/gpio.c`。
- `cmake --build --preset debug --clean-first` 从零编译 52 个目标成功，生成 `build/debug/mobile_charging_robot.elf`；Flash 54,024 字节，RAM 15,704 字节。
- 重新生成后，CAN 生成端与正式端 `can.c` 的 SHA-256 仍一致，`AutoBusOff` 和 `CAN_IT_BUSOFF` USER CODE 均保留。

该切片已于 2026-07-16 完成整机 GPIO 回归，详见文末“2026-07-16 整机上机回归”。

### USART1/RC 接收 DMA 迁移切片

下一切片选择 USART1 遥控器接收链路。选择依据是 `Controller/src/Chassiscontroller.c::Init()` 调用 `RC_Init()`，而 `Entity/src/RC.c::RC_Init()` 当前继续调用 `bsp_rc_Config()`。USART1 和 DMA2 Stream2 已存在于重建 IOC 中，可以在不同时迁移 TIM2 的前提下独立接管。

已验证 BSP 实现的实际配置为：

| 项目 | 已验证实现 |
| --- | --- |
| USART1 引脚 | 只使用 PB7/USART1_RX；PB6/TX 没有配置。 |
| 串口参数 | 100000 bit/s、`UART_WORDLENGTH_8B`、偶校验、1 个停止位、仅接收、16 倍过采样。 |
| DMA | DMA2 Stream2、Channel 4、外设到内存、字节对齐、内存自增、循环模式、最高优先级、FIFO 关闭。 |
| 中断 | USART1 IRQ 抢占优先级 0；不启用 DMA2 Stream2 IRQ。 |
| 协议启动 | 开启 USART IDLE 中断，并启动 36 字节 DMA 接收。 |
| 帧处理 | USART1 IDLE 中断中计算实际长度；只有 18 字节帧才置完成标志，然后重启 DMA。 |

迁移开始时的 CubeMX 生成结果与已验证实现存在四处不能直接合入的差异：

1. 生成代码同时配置 PB6/TX 和 PB7/RX，并把 USART1 设置为收发模式。
2. 生成 DMA 使用普通模式，而旧实现使用循环模式。
3. 生成 DMA 优先级为低，而旧实现为最高。
4. 生成工程启用了 DMA2 Stream2 IRQ 并生成 `HAL_DMA_IRQHandler()`，旧实现未启用该 IRQ，接收帧边界由 USART IDLE 中断处理。

本切片采用以下所有权拆分：

```text
CubeMX usart.c/dma.c
→ USART1、运行时 PB7、DMA2 Stream2 的硬件初始化

BSP bsp_rc.c
→ 36 字节接收缓冲区、IDLE 中断、18 字节判帧、DMA 重启和数据读取
```

实施步骤：

1. 在修改前把基线 `BSP/src/bsp_rc.c` 归档为 `User/archive/bsp_rc.c`。
2. IOC 把 USART1 固定为 `MODE_RX`，把 DMA 改为循环模式和最高优先级，并关闭 DMA2 Stream2 IRQ。
3. 重新生成并审查 `usart.c`、`usart.h`、`dma.c`、`dma.h`。
4. 将这四个文件加入白名单，同步到 `User/Src/` 和 `User/Inc/` 的稳定路径，并加入根 CMake。
5. 在 `Controller::Init()` 中按 `MX_DMA_Init()`、`MX_USART1_UART_Init()`、`RC_Init()` 的顺序启动。
6. 将 `bsp_rc_Config()` 收缩为协议接收启动函数：只开启 IDLE 中断并启动 DMA，不再重复配置 GPIO、USART、DMA 和 NVIC。
7. 暂不同步整个 `stm32f4xx_it.c`。现有文件包含已经验证的 CAN 和自定义 TIM2 中断；USART1 的 IDLE 处理继续保留在 RC BSP 中。

CubeMX 6.18 对 STM32F4 异步 USART 的生成器有一个限制：即使设置为 `MODE_RX`，IOC 中也必须同时保留 TX/RX 引脚，否则生成器会删除整个 `usart.c` 和 `usart.h`。因此 IOC 中保留 PB6/USART1_TX 作为生成约束，但 `usart.c` 在 `USART1_MspInit 1` USER CODE 区立即执行 `HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6)`。该 USER CODE 已经过再次 Generate Code 验证，会被稳定保留；最终运行时只有 PB7 处于 USART1 复用状态。

离线验收条件：

- [x] USART1 为仅接收模式，运行时只使用 PB7/RX，参数与已验证 BSP 一致。
- [x] DMA2 Stream2 为 Channel 4、循环模式、最高优先级，DMA IRQ 保持关闭。
- [x] CubeMX 生成端和正式端的四个 USART/DMA 文件哈希一致。
- [x] `bsp_rc.c` 不再定义句柄或重复初始化硬件，只保留 RC 接收业务。
- [x] 根工程从零编译成功，ELF 中只存在一份 `huart1`、`hdma_usart1_rx` 和强符号 `USART1_IRQHandler`。
- [x] 上机确认遥控器 18 字节帧持续更新，控制模式和通道数据正常，且 CAN1、GPIO 和主动电机控制无回归。

2026-07-15 已完成 USART1/RC DMA 切片的离线实施与验收：

- `User/archive/bsp_rc.c` 的 Git Blob SHA-1 为 `6ce7b6d25196c7ce7be58bd4e3fb8b75a89192f6`，与基线提交 `440b4c1:BSP/src/bsp_rc.c` 完全一致。
- IOC 已固定为 100000 bit/s、8B、偶校验、1 停止位、RX-only；DMA2 Stream2 Channel 4 为循环模式和最高优先级，`NVIC.ForceEnableDMAVector=false`。
- `usart.c`、`usart.h`、`dma.c`、`dma.h` 已加入 `usart1_rc` 白名单并同步到 `User/Src/`、`User/Inc/`。
- 四个文件的生成端/正式端 SHA-256 分别为：`usart.c` `07E4812EEEA1E6D7E7ED1BF6B9BBD33D5B3152447AFBD2708828BAE5E47B6FA2`、`usart.h` `07D5BA2014126B970BDF7E01EC12E35AEFB3EC1C63C1AE4B8D4E00DB3BBFA8CF`、`dma.c` `59C6CE7F463CE8B5782E48FBB451DC0B14984C1F6AD0BB2B206A1BF4DFB13F51`、`dma.h` `7CF6F7450A29D26303CBBA5A3CA8C6CB24A02635D6A5BA4D05141FDFC75C4C49`。
- `Controller::Init()` 现在按 `MX_DMA_Init()`、`MX_USART1_UART_Init()`、`RC_Init()` 启动；`bsp_rc_Config()` 只开启 IDLE 中断并启动 36 字节 DMA 接收。
- `cmake --build --preset debug --clean-first` 从零编译 54 个目标成功；Flash 54,288 字节，RAM 15,704 字节，BIN SHA-256 为 `11CF0010C541FC493789ABF64C119EBDBD94944541CF6D295ED8146AAB0FB42D`。
- ELF 符号检查确认 `huart1` 和 `hdma_usart1_rx` 各一份，`USART1_IRQHandler` 为 RC BSP 中的唯一强实现；`DMA2_Stream2_IRQHandler` 只有启动文件的弱默认符号，NVIC 未启用它。
- 重复执行白名单同步时四个文件均报告 `Already synchronized`。

该切片已于 2026-07-16 完成上机回归，详见文末统一记录。

### TIM2 系统调度迁移切片

下一切片选择 TIM2。当前启动链路为 `User/Src/main.c::main()` 调用 `Time_Init()`，而 `BSP/src/time.c::Time_Init()` 同时完成时钟、句柄、计数参数、定时器启动和 NVIC 配置。TIM2 中断不是普通 HAL 回调：`User/Src/stm32f4xx_it.c::TIM2_IRQHandler()` 手工清除更新中断标志、递增 `m_systick`，并直接调用整个控制器的 `Update()`。

已验证实现的实际参数：

| 项目 | 当前值 |
| --- | --- |
| TIM2 时钟 | APB1 Timer Clock = 84 MHz。 |
| Prescaler | 4199，即 4200 分频。 |
| Auto-reload | 9，即 10 个计数。 |
| 实际更新频率 | `84 MHz / 4200 / 10 = 2000 Hz`。 |
| 实际周期 | 0.5 ms。 |
| NVIC | 旧代码请求抢占优先级 0、子优先级 1；在 `NVIC_PRIORITYGROUP_4` 下没有子优先级位，硬件有效编码等价于 0/0。 |
| 中断动作 | `m_systick++`，随后调用 `Update()`。 |

旧注释把 TIM2 时钟写成 42 MHz，并把周期说明为 1 ms；但 APB1 分频不为 1 时，STM32F4 的定时器时钟为 APB1 外设时钟的两倍，因此当前固件实际以 0.5 ms 周期运行。该参数已经随整机固件通过验证，本切片只迁移所有权，不把 Prescaler 擅自改成 8399。是否应改为真实 1 ms 属于迁移完成后的独立控制周期评估。

迁移开始时 CubeMX 生成的 `tim.c` 使用默认 `Prescaler=0`、`Period=0xFFFFFFFF`，而且只生成初始化，不会自动调用 `HAL_TIM_Base_Start_IT()`。生成的 `TIM2_IRQHandler()` 调用通用 `HAL_TIM_IRQHandler(&htim2)`，会丢失当前直接执行 `m_systick++` 和 `Update()` 的语义，因此不能同步整个生成中断文件。

本切片采用以下所有权拆分：

```text
CubeMX tim.c/tim.h
→ TIM2 时钟、PSC、ARR、句柄和 NVIC 初始化

User main.c
→ 调用 MX_TIM2_Init() 并启动 HAL_TIM_Base_Start_IT()

User stm32f4xx_it.c
→ 保留自定义 TIM2_IRQHandler、m_systick++ 和 Update()
```

实施步骤：

1. 把基线 `BSP/src/time.c` 归档为 `User/archive/time.c`。
2. IOC 设置 Prescaler 4199、Period 9、NVIC 优先级 0/0。旧代码的 0/1 在当前 Priority Group 4 下没有可用的子优先级位，0/0 是 CubeMX 接受的等效配置。
3. 重新生成并审查 `tim.c`、`tim.h`，不把生成的 `stm32f4xx_it.c` 加入同步白名单。
4. 将 `tim.c`、`tim.h` 通过 `tim2` 白名单同步到 `User/Src/` 和 `User/Inc/`，根 CMake 改为编译生成实现并停止编译 `BSP/src/time.c`。
5. `main.c` 改为调用 `MX_TIM2_Init()` 和 `HAL_TIM_Base_Start_IT(&htim2)`；保持它在 `Init()` 之前启动的现有顺序，本切片不同时改变启动时序。
6. `User/Src/stm32f4xx_it.c` 继续作为正式中断文件，保持现有直接调度逻辑。

离线验收条件：

- [x] 生成的 TIM2 参数为 PSC 4199、ARR 9、向上计数、预装载关闭，NVIC 有效配置为 0/0。
- [x] CubeMX 生成端和正式端 `tim.c`、`tim.h` 哈希一致。
- [x] 根 CMake 不再编译 `BSP/src/time.c`，ELF 中只有一份 `htim2` 和强符号 `TIM2_IRQHandler`。
- [x] `main.c` 成功初始化并启动 TIM2；自定义 IRQ 仍直接执行 `m_systick++` 和 `Update()`。
- [x] 根工程从零编译成功，CAN1、GPIO、USART1/RC 的生成文件哈希保持不变。
- [x] 上机测量 `m_systick` 约以 2000 次/秒增长，控制循环稳定，已有 CAN、GPIO、RC 和电机功能无回归。

2026-07-15 已完成 TIM2 切片的离线实施与验收：

- `User/archive/time.c` 的 Git Blob SHA-1 为 `ba059aaf9181e3ee393b7c02f285758487f0d9b4`，与基线提交 `440b4c1:BSP/src/time.c` 完全一致。
- 第一次尝试把 NVIC 写为 0/1 时，CubeMX 报告 `IP not ready for code generation: NVIC/TIM2`。确认工程使用 Priority Group 4 后，将无效子优先级修正为等效的 0/0，随后代码生成成功。
- 生成的 `tim.c` 明确包含 `Prescaler=4199`、`Period=9`、向上计数、预装载关闭和 TIM2 NVIC 0/0。
- `tim.c` 和 `tim.h` 已加入 `tim2` 白名单并同步到稳定路径；生成端/正式端 SHA-256 分别为 `1936065C1A195BB73C9514B7E5ECD47F0B2FF6759CC5D9D77C9B46F6F5440707` 和 `F1029AF1571F326CDCB25FA8B7605171D54085A8947C09E5621BBF3E24D3A9F7`。
- 根 CMake 已改为编译 `User/Src/tim.c`，`compile_commands.json` 中不存在 `BSP/src/time.c`。
- `main.c` 现在调用 `MX_TIM2_Init()`，检查 `HAL_TIM_Base_Start_IT(&htim2)` 返回值后再进入原有 `Init()`；启动相对顺序未改变。
- 正式 `User/Src/stm32f4xx_it.c::TIM2_IRQHandler()` 未被生成文件覆盖，仍手工清除更新标志、执行 `m_systick++` 和 `Update()`。
- `cmake --build --preset debug --clean-first` 从零编译 54 个目标成功；Flash 54,888 字节，RAM 15,704 字节，BIN SHA-256 为 `86FC1DB054B4BEFF519D4B71FDA12D41D50CBA17844B97A3266CDBF77C1D3293`。
- ELF 符号检查确认 `htim2` 只有一份，`TIM2_IRQHandler` 是唯一强实现，旧 `Time_Init` 不再进入 ELF。
- CAN、GPIO、USART1 和 DMA 的正式端/生成端哈希在本次重新生成后仍全部一致。

该切片已于 2026-07-16 实测约 2009.7 Hz，控制循环稳定。

### UART8/RFID 迁移切片

下一切片选择 RFID。调用链为 `Controller::Init()` → `RFID_Config()` → `BSP_RFID_Config()`；运行期间 `Sensor_t_Update()` 调用 `RFID_Update()`，周期性发送查询命令并读取 IDLE 中断截取的 DMA 数据。

已验证 BSP 实现的硬件配置：

| 项目 | 当前值 |
| --- | --- |
| 外设 | UART8，38400 bit/s，8-N-1，TX/RX，16 倍过采样。 |
| GPIO | PE1/UART8_TX、PE0/UART8_RX，AF8、无上下拉、Very High Speed。 |
| TX DMA | DMA1 Stream0、Channel 5、内存到外设、普通模式、高优先级、FIFO 关闭。 |
| RX DMA | DMA1 Stream6、Channel 5、外设到内存、普通模式、高优先级、FIFO 关闭。 |
| NVIC | UART8 优先级 0；DMA1 Stream0 优先级 1；DMA1 Stream6 IRQ 不启用。 |
| 接收方式 | 256 字节 DMA 缓冲区，通过 UART8 IDLE 中断计算实际长度、复制到备份缓冲区并重启 DMA。 |
| 发送完成 | DMA1 Stream0 IRQ → HAL UART TX 完成回调 → `uart8_tx_cplt()`。 |

本切片采用以下所有权拆分：

```text
CubeMX usart.c/dma.c
→ UART8、PE0/PE1、DMA1 Stream0/6、句柄和 NVIC 初始化

BSP bsp_485_rfid.c
→ RFID 缓冲区、IDLE 截帧、DMA 重启、发送状态和数据访问

User usart_callback.c
→ 保留 UART8 TX 完成回调分发
```

生成文件边界：

- UART8 将与已迁移 USART1 共用 `User/Src/usart.c`、`User/Inc/usart.h`。
- DMA1 与已迁移 DMA2 共用 `User/Src/dma.c`、`User/Inc/dma.h`。
- 重新同步这四个文件时必须同时确认 USART1 的 RX-only 配置和 PB6 反初始化 USER CODE 没有丢失。
- 不同步生成的 `stm32f4xx_it.c`。正式 UART8 和 DMA1 Stream0 中断继续由 RFID BSP 提供，以保留 IDLE 截帧和发送完成语义。

已发现的边界和风险：

1. 清理前的 `bsp_debug_usart.h` 也把调试串口定义为 UART8/PE0/PE1，但 `DEBUG_USART_Config()` 从未启用；否则会用第二个 UART 句柄和不同波特率重新配置同一外设。阶段 3 收尾时已删除该未使用模块。
2. `RFID_DMA_Rx_ReStart()` 当前直接调用 `HAL_UART_MspInit(&huart8)`，会绕过 CubeMX 所有权并重复配置 GPIO/DMA；迁移时移除该调用，只保留接收停止、状态恢复和 DMA 重启。
3. `RFID_Update()` 在首帧到来前可能通过空的 `pbuf_rfid` 读取地址 0；UART8 IRQ 中 `memcpy(..., rfid_rx_len + 3)` 在接近满缓冲区时也存在越界风险。这两项属于 RFID 业务健壮性问题，本切片先记录，不与硬件初始化迁移同时重写；上机前应再单独收口。

实施步骤：

1. 把基线 `BSP/src/bsp_485_rfid.c` 归档为 `User/archive/bsp_485_rfid.c`。
2. 在 IOC 中新增 UART8、PE0/PE1、DMA1 Stream0/6 和对应 NVIC，参数与上表一致。
3. 重新生成并审查共享的 `usart.c/usart.h/dma.c/dma.h`，确保既有 USART1 USER CODE 保留。
4. 将共享文件通过 `rfid_uart8` 白名单同步到正式路径。
5. `Controller::Init()` 在 `RFID_Config()` 前调用 `MX_UART8_Init()`；`MX_DMA_Init()` 已在更早位置统一开启 DMA1/DMA2。
6. 将 `BSP_RFID_Config()` 收缩为启动 256 字节 DMA 接收和开启 IDLE 中断，不再定义句柄或重复初始化硬件。
7. 保留 BSP 中的 `UART8_IRQHandler()`、`DMA1_Stream0_IRQHandler()`、缓冲区和发送完成状态逻辑。

离线验收条件：

- [x] 生成的 UART8、GPIO、TX/RX DMA 和 NVIC 参数与已验证 BSP 一致。
- [x] 共享 USART/DMA 文件的生成端与正式端哈希一致，USART1 USER CODE 保持不变。
- [x] `bsp_485_rfid.c` 不再定义句柄或初始化 GPIO/UART/DMA/NVIC，且不再直接调用 `HAL_UART_MspInit()`。
- [x] ELF 中 `huart8`、两路 DMA 句柄、`UART8_IRQHandler` 和 `DMA1_Stream0_IRQHandler` 均只有一份强实现。
- [x] 根工程从零编译成功；CAN、USART1、TIM2 无回归，GPIO 只有端口时钟开启顺序的生成器调整并已同步。
- [x] 上机确认 RFID 查询发送、DMA TX 完成、IDLE 接收、标签编号解析和原有整机功能正常。

2026-07-15 已完成 UART8/RFID 切片的离线实施与验收：

- CubeMX 首次加载新 IOC 时出现 UART8 TX/RX DMA 请求未加载弹窗，并伴随 `Pin26 (VP_RIF_VS_RIF1) cannot be retrieved for this MCU` 日志。最小化测试确认 UART8 和 PE0/PE1 本身可以生成。第一层问题是 DMA 参数键后缀必须对应全局请求索引；进一步在 CubeMX 6.18.0-RC3 中验证，同一 UART 的请求还必须按 RX 后 TX 的顺序排列。因此最终配置为 `Dma.Request1=UART8_RX`/`Dma.UART8_RX.1.*` 和 `Dma.Request2=UART8_TX`/`Dma.UART8_TX.2.*`。修正后在沙箱和正常 Windows 用户环境中均无头生成成功，UART8 两路 DMA 都被保留，阻塞弹窗不再复现。日志仍可能出现非阻塞的 RIF 可选警告，但不影响生成结果。
- `User/archive/bsp_485_rfid.c` 保存了迁移前初始化逻辑的 UTF-8 归档，SHA-256 为 `D6B4950C526A27AE0131E1182344EB2F2CB9A973FBBDF14A2713FDE8BFD8728E`；未经编码归一化的精确历史版本仍可由基线提交 `440b4c1:BSP/src/bsp_485_rfid.c` 和 Git Blob `b00bf1ea4cec7585a3a25d571282566ee081f66a` 获取。
- IOC 已固定 UART8 38400、8-N-1、PE0/PE1 AF8、DMA1 Stream0/6 Channel 5、普通模式和高优先级；UART8 IRQ 为 0/0，DMA1 Stream0 IRQ 为 1/0，DMA1 Stream6 IRQ 保持关闭。
- `usart.c`、`usart.h`、`dma.c`、`dma.h` 已加入 `rfid_uart8` 白名单并同步到稳定路径。生成端/正式端 SHA-256 分别为：`usart.c` `8F07F1B91D54A28657168ACCC2A6C96E74B22BFF9DC713BB152A0C9EE561AA5E`、`usart.h` `0ADAFBA1937D231FAE58C47C3600F68704891BDDE101863151A1C95BBC93A9A9`、`dma.c` `712311FB7BDB4BEA4EDAF2E2A32EC6887D95FDDB0B091905E05AB87E837A42ED`、`dma.h` `7CF6F7450A29D26303CBBA5A3CA8C6CB24A02635D6A5BA4D05141FDFC75C4C49`。
- 再次 Generate Code 后，`usart.h` 中三路 DMA 句柄声明、USART1 RX-only 参数和 `HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6)` USER CODE 均被稳定保留。
- `Controller::Init()` 现在按 `MX_DMA_Init()` → `MX_UART8_Init()` → `RFID_Config()` 启动 RFID 链路；`BSP_RFID_Config()` 只启动 256 字节 DMA 接收和 UART IDLE 中断。BSP 继续持有 `UART8_IRQHandler()`、`DMA1_Stream0_IRQHandler()`、缓冲区和发送完成状态。
- UART8 加入 PE0/PE1 后，CubeMX 只把 `gpio.c` 中 `GPIOE` 时钟开启语句移到端口时钟列表首位，所有引脚配置和安全初始电平未改变；该纯顺序变化已通过 GPIO 白名单同步，生成端/正式端 SHA-256 为 `5F1047F06FEF4890DC7AE2930086CDD5E6AA8E50E0BC707BB3F2916177E4DB4D`。
- `cmake --build --preset debug --clean-first` 从零编译 54 个目标成功；Flash 54,912 字节，RAM 15,704 字节，BIN SHA-256 为 `FCBD3423EF224053F7CEE9B11169D8777F6CD6C131957974DEF48C8DADAED917`。
- ELF 符号检查确认 `huart8`、`hdma_uart8_tx`、`hdma_uart8_rx` 各一份，`UART8_IRQHandler` 和 `DMA1_Stream0_IRQHandler` 是唯一强实现；关闭的 `DMA1_Stream6_IRQHandler` 仍为启动文件弱默认实现。USART1 的 `huart1`、`hdma_usart1_rx` 和强 `USART1_IRQHandler` 同时保持正常。
- 重复执行 `rfid_uart8` 白名单同步时四个共享文件均报告 `Already synchronized`。`DEBUG_USART_Config()` 继续保持注释，未与 RFID 争用 UART8。

该切片已于 2026-07-16 完成上机回归，详见文末统一记录。

### USART6/Battery 迁移切片

下一切片选择 Battery。调用链为 `Controller::Init()` → `Battery_Init()` → `BSP_Battery_Config()`；运行期间 `Battery_Update()` 周期性发送 Modbus 查询，并读取 USART6 IDLE 中断截取的 DMA 数据。

已验证 BSP 实现的硬件配置：

| 项目 | 当前值 |
| --- | --- |
| 外设 | USART6，9600 bit/s，8-N-1，TX/RX，16 倍过采样。 |
| GPIO | PG9/USART6_RX、PG14/USART6_TX，AF8、无上下拉、Very High Speed。旧 BSP 注释把 TX/RX 角色写反，但因两个引脚一起配置为 AF8，不影响已验证固件的实际工作。 |
| TX DMA | DMA2 Stream6、Channel 5、内存到外设、普通模式、高优先级、FIFO 关闭。 |
| RX DMA | DMA2 Stream1、Channel 5、外设到内存、普通模式、高优先级、FIFO 关闭。 |
| NVIC | USART6 优先级 0；DMA2 Stream6 优先级 1；DMA2 Stream1 IRQ 不启用。 |
| 接收方式 | 1024 字节 DMA 缓冲区，通过 USART6 IDLE 中断计算实际长度、复制到备份缓冲区并重启 DMA。 |
| 发送完成 | DMA2 Stream6 IRQ → HAL UART TX 完成回调 → `usart6_tx_cplt()`。 |

本切片采用以下所有权拆分：

```text
CubeMX usart.c/dma.c
→ USART6、PG9/PG14、DMA2 Stream1/6、句柄和 NVIC 初始化

BSP bsp_485_battery.c
→ Battery 缓冲区、IDLE 截帧、DMA 重启、发送状态和数据访问

User usart_callback.c
→ 保留 USART6 TX 完成回调分发
```

生成文件边界：

- USART6 与 USART1、UART8 共用 `User/Src/usart.c` 和 `User/Inc/usart.h`。
- DMA2 与已有 USART1、UART8 共用 `User/Src/dma.c` 和 `User/Inc/dma.h`。
- CubeMX 6.18.0-RC3 对同一 UART 的 DMA 请求顺序敏感，USART6 请求必须按 RX 后 TX 加入，并保持全局请求索引后缀一致。
- 不同步生成的 `stm32f4xx_it.c`。正式 USART6 和 DMA2 Stream6 中断继续由 Battery BSP 提供，以保留 IDLE 截帧和发送完成语义。

已发现但不在本硬件所有权切片中同时修改的业务风险：

1. `Battery_Update()` 在首帧到来前会让空的 `pbuf_battery` 进入 `battery_if_rx_lock()` 并读取 `data[0]`。
2. USART6 IDLE 中断使用 `memcpy(..., battery_rx_len + 3)`；接近 1024 字节满缓冲区时存在越界风险。
3. `BATTERY_DMA_Rx_ReStart()` 当前直接调用 `HAL_UART_MspInit(&huart6)`，迁移时必须删除该重复硬件初始化，只保留接收状态恢复和 DMA 重启。

实施步骤：

1. 把基线 `BSP/src/bsp_485_battery.c` 归档为 `User/archive/bsp_485_battery.c`。
2. 在 IOC 中新增 USART6、PG9/RX、PG14/TX、DMA2 Stream1/6 和对应 NVIC，参数与上表一致。
3. 重新生成并审查共享的 `usart.c/usart.h/dma.c/dma.h`，确认 USART1 的 RX-only USER CODE 与 UART8 两路 DMA 均未丢失。
4. 将共享文件通过 `battery_usart6` 白名单同步到正式路径。
5. `Controller::Init()` 在 `Battery_Init()` 前调用 `MX_USART6_UART_Init()`；统一的 `MX_DMA_Init()` 已在更早位置开启 DMA2。
6. 将 `BSP_Battery_Config()` 收缩为启动 1024 字节 DMA 接收和开启 IDLE 中断，不再定义句柄或重复初始化 GPIO、USART、DMA 和 NVIC。
7. 保留 BSP 中的 `USART6_IRQHandler()`、`DMA2_Stream6_IRQHandler()`、缓冲区及发送完成状态逻辑。

离线验收条件：

- [x] 生成的 USART6、GPIO、TX/RX DMA 和 NVIC 参数与已验证 BSP 一致。
- [x] 共享 USART/DMA 文件的生成端与正式端哈希一致，USART1 和 UART8 配置保持不变。
- [x] `bsp_485_battery.c` 不再定义句柄或初始化 GPIO/USART/DMA/NVIC，且不再调用 `HAL_UART_MspInit()`。
- [x] ELF 中 `huart6`、两路 DMA 句柄、`USART6_IRQHandler` 和 `DMA2_Stream6_IRQHandler` 均只有一份强实现。
- [x] 根工程从零编译成功，已有 CAN、GPIO、RC、TIM2 和 RFID 链路无离线回归。
- [x] 上机确认 Battery 查询发送、DMA TX 完成、IDLE 接收、SOC/电流/容量解析及充电状态判断正常。

2026-07-15 已完成 USART6/Battery 切片的离线实施与验收：

- `User/archive/bsp_485_battery.c` 的 Git Blob SHA-1 为 `75939e01a5da148e3eea157ecc4964bd1963bd12`，与基线提交 `440b4c1:BSP/src/bsp_485_battery.c` 完全一致。
- 首次无头生成时 CubeMX 报告 `Cannot map signal (USART6_TX) on pin (PG9)` 和 `Cannot map signal (USART6_RX) on pin (PG14)`。检查 STM32F427IIHx 芯片数据库确认旧 BSP 注释和旧重建清单把引脚角色写反；正确映射是 PG9/RX、PG14/TX。旧 BSP 一次性把两个引脚配置成相同 AF8，因此该注释错误不影响已经上机验证的旧固件。
- IOC 已固定 USART6 9600、8-N-1、PG9/RX、PG14/TX、DMA2 Stream1/6 Channel 5、普通模式和高优先级；USART6 IRQ 为 0/0，DMA2 Stream6 IRQ 为 1/0，DMA2 Stream1 IRQ 保持关闭。两路请求按 CubeMX 6.18.0-RC3 所需的 RX 后 TX 顺序排列。
- `usart.c`、`usart.h`、`dma.c`、`dma.h` 已加入 `battery_usart6` 白名单并同步到稳定路径。生成端/正式端 SHA-256 分别为：`usart.c` `8903606E6CE49892D97144B20BFF1A01D613BA093499D1AB32513A38B0467ECA`、`usart.h` `DF849D09CE4B4F566580E4274F2E4EF77825A6B44460633E390A263618FC8437`、`dma.c` `05BD05F33069CAC4C7E3F10D9782BBA29EC4AE03DB30C90D3D8A8BEDCDDB5B4B`、`dma.h` `7CF6F7450A29D26303CBBA5A3CA8C6CB24A02635D6A5BA4D05141FDFC75C4C49`。
- 再次 Generate Code 后，USART6 的两个 DMA 句柄声明、USART1 的 PB6 反初始化 USER CODE 和 UART8 两路 DMA 配置均被稳定保留。
- `Controller::Init()` 现在按统一 `MX_DMA_Init()` → `MX_USART6_UART_Init()` → `Battery_Init()` 启动 Battery 链路；`BSP_Battery_Config()` 只启动 1024 字节 DMA 接收和 USART6 IDLE 中断，`BATTERY_DMA_Rx_ReStart()` 不再调用 `HAL_UART_MspInit()`。
- `cmake --build --preset debug --clean-first` 从零编译 54 个目标成功；Flash 54,920 字节，RAM 15,704 字节，BIN SHA-256 为 `CAC6E19857267B59E8E417A4287255B4D174BBCB9C5229D0D45A0657ABB919AC`。
- ELF 符号检查确认 `huart6`、`hdma_usart6_rx`、`hdma_usart6_tx` 各一份，`USART6_IRQHandler` 和 `DMA2_Stream6_IRQHandler` 是唯一强实现；关闭的 `DMA2_Stream1_IRQHandler` 仍为启动文件弱默认实现。
- 重复执行 `battery_usart6` 白名单同步时四个共享文件均报告 `Already synchronized`，`git diff --check` 通过。

该切片已于 2026-07-16 完成上机回归，详见文末统一记录。

### UART7/Server 迁移切片

下一切片选择 Server。调用链为 `Controller::Init()` → `Server_Init()` → `UART7_Config()`；运行期间 `Server_Update()` 接收并拼接 JSON 消息，通过发送队列和 UART DMA 返回机器人状态。

已验证 BSP 实现的硬件配置：

| 项目 | 当前值 |
| --- | --- |
| 外设 | UART7，9600 bit/s，8-N-1，TX/RX，16 倍过采样。 |
| GPIO | PE8/UART7_TX、PE7/UART7_RX，AF8、上拉、Very High Speed；旧代码的兼容宏 `GPIO_SPEED_HIGH` 在 STM32F4 上映射到该档位。 |
| TX DMA | DMA1 Stream1、Channel 5、内存到外设、普通模式、低优先级、FIFO 关闭。 |
| RX DMA | DMA1 Stream3、Channel 5、外设到内存、普通模式、低优先级、FIFO 关闭。 |
| NVIC | UART7 优先级 0；DMA1 Stream1 和 Stream3 均为优先级 1，两个 DMA IRQ 都启用。 |
| 接收方式 | 1024 字节 DMA 缓冲区，通过 UART7 IDLE 中断计算本次长度并重启 DMA。 |
| 发送完成 | DMA1 Stream1 IRQ → HAL UART TX 完成回调 → `uart7_tx_cplt()`。 |

本切片采用以下所有权拆分：

```text
CubeMX usart.c/dma.c
→ UART7、PE7/PE8、DMA1 Stream1/3、句柄和 NVIC 初始化

BSP bsp_485_server.c
→ Server 接收缓冲、IDLE 截帧、DMA 重启、发送状态和三个中断处理函数

Entity Server.c
→ JSON 拼接/解析、发送队列和机器人业务消息
```

生成文件边界：

- UART7 与 USART1、USART6、UART8 共用 `User/Src/usart.c` 和 `User/Inc/usart.h`。
- DMA1 与现有 UART8、DMA2 外设共用 `User/Src/dma.c` 和 `User/Inc/dma.h`。
- CubeMX 6.18.0-RC3 中 UART7 的 DMA 请求继续按 RX 后 TX 排列，并保持全局请求索引后缀一致。
- 不同步生成的 `stm32f4xx_it.c`。正式 UART7、DMA1 Stream1 和 DMA1 Stream3 中断继续由 Server BSP 提供。
- 清理前的 `bsp_debug_usart.h` 中存在被注释的 UART7 调试串口备选定义，但从未启用；阶段 3 收尾时已删除，不再存在与 Server 争用外设的风险。

已发现但不在硬件所有权迁移中同时修改的业务风险：

1. `Server_Update()` 在没有新数据时可能以 `pbuf_server == NULL`、`len_server == 0` 调用 `memcpy()`；虽然常见库实现对零长度不访问地址，但应在后续业务健壮性切片中显式跳过。
2. BSP 接收缓冲区为 1024 字节，而 Entity 拼接缓冲区为 256 字节；当前会在累计长度达到上限前清空并返回，长 JSON 消息需要上机覆盖测试。

实施步骤：

1. 把基线 `BSP/src/bsp_485_server.c` 归档为 `User/archive/bsp_485_server.c`。
2. 在 IOC 中新增 UART7、PE7/RX、PE8/TX、DMA1 Stream3/1 和对应 NVIC。
3. 重新生成并审查共享的 `usart.c/usart.h/dma.c/dma.h`，确认 USART1、USART6 和 UART8 的配置与 USER CODE 均未丢失。
4. 将共享文件通过 `server_uart7` 白名单同步到正式路径。
5. `Controller::Init()` 在 `Server_Init()` 前调用 `MX_UART7_Init()`；统一的 `MX_DMA_Init()` 已在更早位置开启 DMA1。
6. 将 `UART7_Config()` 收缩为启动 1024 字节 DMA 接收和开启 IDLE 中断，不再定义句柄或重复初始化 GPIO、UART、DMA 和 NVIC。
7. 保留 BSP 中的 `UART7_IRQHandler()`、`DMA1_Stream1_IRQHandler()`、`DMA1_Stream3_IRQHandler()`、缓冲区和发送完成状态逻辑。

离线验收条件：

- [x] 生成的 UART7、GPIO、TX/RX DMA 和 NVIC 参数与已验证 BSP 一致。
- [x] 共享 USART/DMA 文件的生成端与正式端哈希一致，已有三个串口链路配置保持不变。
- [x] `bsp_485_server.c` 不再定义句柄或初始化 GPIO/UART/DMA/NVIC。
- [x] ELF 中 `huart7`、两路 DMA 句柄以及三个 Server 中断均只有一份强实现。
- [x] 根工程从零编译成功，CAN、GPIO、RC、TIM2、RFID 和 Battery 无离线回归。
- [x] 上机确认 JSON 接收/拼接/解析、发送队列、DMA TX 完成和机器人状态上报正常。

2026-07-15 已完成 UART7/Server 切片的离线实施与验收：

- 旧 `bsp_485_server.c` 使用本地代码页编码，补丁工具无法直接读取。迁移时先机械转换为 UTF-8，再修改正式文件；可读归档 `User/archive/bsp_485_server.c` 的 SHA-256 为 `0E5AD34ADBF6EC4673C92164BC2D786F238CDD8B989FC43D6E8DB41B3362EB11`。未经编码转换的精确历史版本仍由基线提交 `440b4c1:BSP/src/bsp_485_server.c` 和 Git Blob `3e8d26211157fa4af75029b16e9e9f25f69a371d` 保留。
- IOC 已固定 UART7 9600、8-N-1、PE7/RX、PE8/TX、上拉和 Very High Speed；DMA1 Stream3/1 Channel 5 均为普通模式、低优先级，UART7 IRQ 为 0/0，两个 DMA IRQ 均为 1/0。两路请求按 RX 后 TX 顺序排列。
- CubeMX 第一次生成默认使用 `GPIO_NOPULL`，审查发现与旧 BSP 的 `GPIO_PULLUP` 不一致后已修正 IOC；旧代码的 `GPIO_SPEED_HIGH` 在 STM32F4 兼容层映射为 `GPIO_SPEED_FREQ_VERY_HIGH`，生成速度参数一致。
- `usart.c`、`usart.h`、`dma.c`、`dma.h` 已加入 `server_uart7` 白名单并同步到稳定路径。生成端/正式端 SHA-256 分别为：`usart.c` `9953C4C4074D7368C16137059703051179E5223D111FEBD10D3B108A0831756F`、`usart.h` `5FD199BB235933A53D07E700AC0DA9CFEBD29A32BAB843F3D2D18B55584A9309`、`dma.c` `6CDB1486DB8EC25CA1228A7103167FE63DB255096E2C8366C35F40736E6E0C5D`、`dma.h` `7CF6F7450A29D26303CBBA5A3CA8C6CB24A02635D6A5BA4D05141FDFC75C4C49`。
- 再次 Generate Code 后，UART7/USART6/UART8 的 DMA 句柄声明、USART1 的 PB6 反初始化 USER CODE 和四路串口配置均被稳定保留。
- `Controller::Init()` 现在在 `Server_Init()` 前调用 `MX_UART7_Init()`；`UART7_Config()` 只启动 1024 字节 DMA 接收和 UART IDLE 中断，三个自定义中断继续由 Server BSP 持有。
- `cmake --build --preset debug --clean-first` 从零编译 54 个目标成功；Flash 54,904 字节，RAM 15,704 字节，BIN SHA-256 为 `A48EE0047D889F9193AB3EB0260DF3938B1EED86EB125AB878BCDFE6D6DE2D96`。
- ELF 符号检查确认 `huart7`、`hdma_uart7_rx`、`hdma_uart7_tx` 各一份，`UART7_IRQHandler`、`DMA1_Stream1_IRQHandler` 和 `DMA1_Stream3_IRQHandler` 均为唯一强实现。
- 重复执行 `server_uart7` 白名单同步时四个共享文件均报告 `Already synchronized`，`git diff --check` 通过。

该切片已于 2026-07-16 完成上机回归，详见文末统一记录。

### USART1、TIM2 与中断归属

- USART1、PB7 和 DMA2 Stream2 的硬件初始化已经由 CubeMX 接管；DMA2 Stream2 IRQ 已在 IOC 中关闭，生成工程不再生成该中断处理函数。RC 的 IDLE 判帧和 DMA 重启仍归 BSP 所有。
- TIM2 的硬件初始化已经由 CubeMX 接管，但正式中断文件继续使用已验证的自定义 `TIM2_IRQHandler()`，不会同步生成的通用 `HAL_TIM_IRQHandler(&htim2)` 实现。
- USART1 和 TIM2 切片均已完成离线迁移；RC IDLE 判帧和 TIM2 控制调度仍分别保留在对应的业务/中断层。

## 不可违反的迁移规则

任何外设都不能同时启用 CubeMX 初始化和旧 BSP 初始化。只有生成实现通过相应验收条件后，才能删除旧实现。

## 当前检查点

CAN1 迁移切片已经完成编译、烧录和上机技术验证，最终 Debug 固件占用 Flash 53,896 字节、RAM 15,672 字节。最终实测结果：

- 程序稳定运行在 `main.c:100` 主循环。
- `hcan1.Instance = 0x40006400`，状态为工作态，错误码为 0。
- CAN1 时钟已开启；最终源码中不存在 CAN2 句柄、初始化或时钟配置。
- CAN 过滤器分界值为 14，过滤器组 0 已激活。
- `hcan2`、`MX_CAN2_Init()`、CAN2 强中断处理函数和 CAN2 电机 ID 枚举均已从 ELF 输入源码删除。
- 连续两次采样中，前三个 `Motor_measure` 均具有有效角度，转矩电流字段持续变化，确认 CAN1 接收中断和电机反馈正常。
- CAN1 接收回调只允许 `0x201`–`0x204` 映射到四元素 `Motor_measure` 数组，其他报文直接忽略。
- 正式 CMake 编译稳定路径 `User/Src/can.c`；该文件由白名单脚本从 CubeMX 生成目录同步更新。

2026-07-16 用户确认当前机器人不使用 CAN2。最终 IOC 已删除 CAN2 外设、PB12/PB13 和 CAN2 NVIC；生成端与正式端均不再包含 CAN2 句柄、初始化、MSP 或强中断实现。启动文件保留的 CAN2 弱向量是 STM32F427 固定中断向量表的一部分，不会启用外设。

GPIO、USART1/RC DMA、TIM2、UART8/RFID、USART6/Battery 和 UART7/Server 切片均已完成离线迁移及整机回归，现有四路业务串口的硬件初始化均已由 CubeMX 接管。Ultrawave、PB2 Key、Debug USART 和 CAN2 已确认不属于当前机器人的有效功能，阶段 3 主动排除并从最终配置与正式构建中删除。

## 2026-07-16 阶段 3 收尾清理

删除前，已将完成整机验证的版本提交并推送到远程 `codex/cubemx-rebuild` 分支：

- 清理前硬件验证基线提交：`8154cca`（`stage3: establish hardware-validated CubeMX migration baseline`）。
- 该提交对应本文件“2026-07-16 整机上机回归”记录的 ELF/BIN 和实测结果，可作为本轮清理的精确回退点。

本轮清理内容：

- 从 CMake 和正式源码中删除 `bsp_ultrawave.c/.h`、`bsp_key.c/.h`、`bsp_debug_usart.c/.h`。
- 删除 `Init()` 与 `Sensor_t_Update()` 中的 Ultrawave 初始化/更新调用，并移除 `Sensor_t::ultra_stop`。原逻辑一直将该字段固定为 0，清理后保持“无超声波阻挡”的现有行为。
- 删除由 `MX_GPIO_Init()` 完整替代的 `LED_GPIO_Config()`、`l298n_GPIO_Config()`、`L298N_Config()`、`Limit_Switch_Config()`、`Photogate_Config()` 和 `RGB_Config()` 及其头文件声明。
- 保留 LED/RGB 控制、推杆控制、前后限位读取和光电门读取等业务接口。
- 标准输入输出系统调用改为非阻塞空实现，避免删除未初始化的 Debug USART 重定向后发生 Newlib 递归；当前固件没有启用控制台输出。
- 为了能安全修改历史 GBK/ANSI 文件，只将本轮涉及的少量源码机械转换为 UTF-8；不改变业务逻辑。
- 删除未进入 CMake 的旧 RTOS 示例 `APP/`、已由 `tim.c/tim.h` 替代的 `BSP/src/time.c` 和 `BSP/inc/time.h`，以及内容全部被注释的 `User/Src/Robot_Config.c`；实际机器人配置宏继续保留在 `User/Inc/Robot_Config.h`。
- 当前分支删除 `MDK-ARM/`、旧 EIDE 文档、旧损坏 IOC/`.mxproject`、Keil 数据脚本和过期测试 JSON；这些历史内容仍由远程 `master` 和提交 `8154cca` 保存。
- 清除 EIDE 芯片包缓存、旧构建目录、隔离 CubeMX 工程中可重新生成的 Drivers/build 副本、`.bak` 和编辑器临时状态，释放约 669 MB。本次保留根 `build/`，因为 F5、GDB 和 ARM LiveWatch 仍需要当前 Debug ELF。
- 删除空的 `Robot_Config.c` 对 ELF 调试/符号元数据有影响，但 Debug BIN 哈希保持不变，说明烧录到 Flash 的程序字节没有因这一步目录清理而变化。

离线验证结果：

- Debug 和 Release 均使用 GNU Arm GCC 14.2、CMake Preset 和 Ninja 完成 `--fresh` 全新构建。
- Debug：text 51,612 字节、data 552 字节、bss 14,896 字节；目录清理后的 ELF SHA-256 `212389B330273EF58A824BD4A900582BB9D3854600F34AD8DE8E1428C897B92E`，BIN SHA-256 `6ECF9E1F7B3904CF91D2FFB22A55E15CD714FFDE5B96B51EECC28AE6D2EE4C27`。
- Release：text 51,260 字节、data 556 字节、bss 14,892 字节；ELF SHA-256 `7D2347407E59F285631AE08CB89E5AD855F55287055E0126CE7C37808EB879D0`。
- 没有新增编译错误；剩余警告仍是阶段 4 已记录的 `__packed`、头文件无效 `static` 声明和 Battery 有符号/无符号比较。
- 按用户要求，本轮暂不上机测试。清理版本仍需在阶段 3 封版前完成一次烧录、启动和主要链路冒烟验证。

### CAN2 最终删除

2026-07-16 用户确认机器人不使用 CAN2，阶段 3 不再保留“未来可能启用”的半成品配置：

- 从 `cubemx/mobile_charging_robot.ioc` 删除 CAN2 IP、PB12/PB13、CAN2 RX0/TX NVIC 和 `MX_CAN2_Init()` 生成顺序，并重新连续编号 IP 与 Pin 列表。
- 使用 STM32CubeMX 6.18.0-RC3 重新生成隔离工程；生成结果只加载 CAN1，不再生成 CAN2 句柄、初始化、MSP 或强中断实现。
- `can` 白名单扩展为同时同步 `can.c` 和 `can.h`，并自动清除 CubeMX 在 `can.h` 末尾生成的多余空行。生成端与正式端哈希一致：`can.c` SHA-256 `02674253579699A0B21A752EC79FFB0177B330B4E05E0D7A672A295E8297E820`，`can.h` SHA-256 `76384587DEA3CBB5276BC7DA4B923704059B655B0B205BD8A76C6CDFA66AF9DF`。
- 从正式 `stm32f4xx_it.c/.h` 删除 CAN2 强中断处理函数，从 CAN BSP 删除 `hcan2` 外部声明和全部 CAN2 电机 ID 枚举。
- Debug 和 Release 再次完成 `--fresh` 全新构建。Debug：text 51,396、data 552、bss 14,848，ELF SHA-256 `BEC090AE1FAF60974187594A8554179860D51C091E0380DBF1D45F1C35EC60BE`，BIN SHA-256 `BE2A1DBA4A81338BC7ED406D8EA201789E6C42290D85C4640E4788074FBFA9A6`。
- Release：text 51,052、data 556、bss 14,852，ELF SHA-256 `E9CAF4D0B7CA1374A21C6975EEA3A02AE7C39173173D58F427F26AB0B66C18A6`，BIN SHA-256 `B2F67F350BB66B7E31B2F6DA7A8410A4225B4DB6ED2469843FF96713BC41B0EC`。
- ELF 中只剩启动文件提供的 CAN2 RX0/TX 弱默认向量，没有 CAN2 强实现或业务符号。与删除 CAN2 前相比，Debug 的 text 减少 216 字节、bss 减少 48 字节。
- 本轮仍按用户要求只做离线验证；最终上机冒烟测试尚未执行。

## 2026-07-16 清理前基线整机上机回归

本次使用 `ATK ATK-HS-V3-CMSIS-DAP`（序列号 `ATK 20190528`）、xPack OpenOCD、SWD 1 MHz 和当前 CMake Debug ELF 完成烧录及运行验证。OpenOCD 识别 STM32F42x/43x 2 MB Flash，程序下载完成并通过逐字节校验。

最终烧录产物：

- ELF SHA-256：`C34D90182527AACAC2F9C23BA8F60E6FE6BAD220A47478CA66DC15DD421E3F9A`。
- BIN SHA-256：`DA0C562F40534E96B4653EB2557EF03E9A832662AE94EE5FAEBB5D2EC14E33CB`。
- GNU size：text 53,964 字节、data 552 字节、bss 15,128 字节；链接器报告 Flash 54,520 字节、RAM 15,672 字节。

自动采样与现场确认结果：

- 程序稳定运行在 `main()` 主循环，ARM LiveWatch 成功连接 OpenOCD TCL 端口 `6666` 并可非停机读取 RAM。
- `m_systick` 在 2.191 秒内增长 4,403，实测约 2009.7 Hz，与 TIM2 当前 0.5 ms 周期一致。
- CAN1 状态正常、错误码为 0；前三个 `Motor_measure` 具有有效角度，转矩电流连续变化，主动电机控制现场无异常。
- USART1/RC 的五个通道处于居中值，两个模式开关均为 3；接收超时计数持续被新帧复位，确认 18 字节 DMA/IDLE 链路正常。
- UART8/RFID 连续识别标签原始码 `0x9E1D` 为点位编号 2，帧长 11，帧数据为 `30 E0 04 01 53 08 C3 9E 1D 23 0D`。
- USART6/Battery 成功解析容量约 20.07 Ah、SOC 100%；周期查询计数持续运行，TX DMA 完成标志正常恢复。
- UART7/Server 的安全注入帧成功解析为 `robot_id=014`、`command=0`、`take_id=2`、`chg_id=TEST`、`task_id=CODEX`；发送队列成功出队，TX DMA 完成回调恢复。用户同时完成实际平台通信检查并确认无异常。
- 前限位按下时 PI6 和 `m_ctrl.front_state` 同步由 0 变为 1，松开后恢复；后限位按下/松开状态及光电门由用户现场确认正常。
- RGB 输出寄存器与绿色状态一致；L298N、LED、推杆和其余 GPIO/执行机构由用户现场测试并确认无异常。

回归完成后已关闭测试用后台 OpenOCD，CMSIS-DAP 和端口 `3333/4444/6666` 均已释放。Ultrawave 因机器人实际未使用而被阶段 3 主动排除；其旧实现已由清理前基线提交 `8154cca` 保留，正式源码不再编译该模块。

## 2026-07-16 清理后最终整机回归（通过）

本次使用删除 CAN2、Ultrawave、Key、Debug USART、重复 GPIO 初始化和冗余工程文件后的最终 Debug 固件。通过 `ATK ATK-HS-V3-CMSIS-DAP`、xPack OpenOCD 和 SWD 1 MHz 完成烧录，OpenOCD 使用 GDB/Telnet/TCL 端口 `3333/4444/6666`。

本次烧录产物：

- Debug ELF SHA-256：`BEC090AE1FAF60974187594A8554179860D51C091E0380DBF1D45F1C35EC60BE`。
- Debug BIN SHA-256：`BE2A1DBA4A81338BC7ED406D8EA201789E6C42290D85C4640E4788074FBFA9A6`。
- GNU size：text 51,396 字节、data 552 字节、bss 14,848 字节。

已通过：

- 程序稳定运行在主循环，状态机保持 `STATE_IDLE`；TIM2 约为 2 kHz。
- CAN1 处于 LISTENING，错误码为 0，三个电机反馈持续更新。
- RC 通道数据、模式开关和新帧接收正常。
- Battery 成功解析约 20.01 Ah、SOC 99%，TX DMA 正常。
- Server RX 成功解析安全测试帧，Server TX 队列、UART7 TX DMA 和完成回调正常。
- UART1、USART6、UART7 和 UART8 状态正常。
- RFID 识别原始码 `0x9E1D` 为编号 2，11 字节帧为 `30 E0 04 01 53 08 C3 9E 1D 23 0D`。
- 光电门触发与释放均正常。
- RGB 红、青、绿输出切换正常，测试后恢复绿色。
- ARM LiveWatch 通过 TCL 端口实现非停机变量读取。
- 推杆正向和反向各短动约 0.3 秒，用户现场确认两个方向均有实际动作；测试后 PA0–PA3 已强制恢复低电平。
- 后限位按下时 PI7 为低电平，`m_ctrl.rear_state` 同步为触发状态 1；用户确认该链路正常，不再重复检查松开状态。
- 红色 LED（PE11）和绿色 LED（PF14）依次完成“全灭→仅红灯亮→仅绿灯亮→两灯亮”切换，寄存器与现场亮灭结果一致；最后恢复两灯亮的初始状态。

说明：

- 按用户要求，本轮不重复测试前限位；清理前基线已记录 PI6 和 `m_ctrl.front_state` 的按下、松开联动正常。
- 回归完成后已关闭后台 OpenOCD，CMSIS-DAP 及端口 `3333/4444/6666` 已释放。
- 清理后最终固件通过主要硬件和通信链路回归，阶段 3 的 CubeMX 重建、重复初始化清理和未使用模块删除已完成验收。
