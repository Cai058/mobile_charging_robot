# STM32CubeMX 迁移状态

最后更新：2026-07-15

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

旧文件 `RM_Robot_on_A_no_os.ioc` 无法被 CubeMX 6.18.0-RC3 稳定迁移，加载时会出现 `Range [5, 3) out of bounds for length 3` 和 `Mcu.getDie() is null`。因此，本次迁移采用全新的 STM32F427IIHx 工程，并逐项重建配置。旧 IOC 保持不变，只作为参考。

## 隔离目录结构

```text
cubemx/
  mobile_charging_robot.ioc   # 重建后的唯一硬件配置源
  generate-stage1.txt         # 可重复执行的 CubeMX 命令行生成脚本
  generated/                  # 隔离的 CubeMX CMake 生成工程
```

重建过程中不会原地覆盖根目录中的 `RM_Robot_on_A_no_os.ioc`、`User/`、`BSP/`、`Controller/`、`Entity/` 以及现有 CMake 工作流。

## 迁移切片 1

范围：

1. STM32F427IIHx 和 UFBGA176 封装。
2. HSE 12 MHz，系统时钟 168 MHz。
3. APB1 42 MHz，APB2 84 MHz。
4. PA13/PA14 上的 SWD。
5. PD0/PD1 上的 CAN1，1 Mbit/s。
6. PB12/PB13 上的 CAN2 配置暂时保留在 IOC 中，但本切片不启用 CAN2。
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

第一次生成的代码将 `AutoBusOff` 设置为 `DISABLE`，而已验证固件的两路 CAN 都使用 `ENABLE`。CubeMX 6.18 在 IOC 中使用参数名 `ABOM` 表示此配置。重建后的 IOC 已固定设置 `CAN1.ABOM=ENABLE` 和 `CAN2.ABOM=ENABLE`，重新生成后会正确得到 `AutoBusOff = ENABLE`。

已验证固件还会显式启用 `CAN_IT_BUSOFF`。隔离生成工程现在在 `HAL_CAN_Init()` 之后，分别通过 `CAN1_Init 2` 和 `CAN2_Init 2` 的 USER CODE 区启用该中断。再次运行 CubeMX 生成代码后，这两处修改仍然保留，重新生成的工程也能成功编译。

其余 CAN 初始化字段、CAN GPIO 复用功能、NVIC 优先级以及 TX/RX0 中断处理函数均与已验证实现一致。

2026-07-15 使用 OpenOCD 对当前机器人进行了只读诊断。复位并运行 2 秒后，程序停在 `main.c:100` 的主循环，与当前 CMake ELF 的符号一致。实测结果如下：

- `hcan1.Instance = 0x40006400`，状态为工作态，CAN1 已正常初始化。
- `hcan2.Instance = 0`，状态为 RESET，错误码为 `0x000C0000`，即 `HAL_CAN_ERROR_NOT_INITIALIZED | HAL_CAN_ERROR_NOT_READY`。
- `MX_CAN2_Init()` 因没有调用而被链接器从最终 ELF 中删除。
- `can_filter_init()` 对未初始化的 `hcan2` 调用配置、启动和通知函数时，HAL 只返回错误，因此程序没有崩溃，但 CAN2 实际从未工作。

同时确认 `CAN_FilterTypeDef can_filter_st` 未进行零初始化，而且第一次配置 CAN1 前没有设置 `SlaveStartFilterBank`。实机 CAN1 的 FMR 寄存器中读到 CAN2 过滤器起始分界值为 60，超出双 CAN 合法范围 0–27。当前 CAN1 仍能接收，是因为该异常分界值没有把过滤器组 0 分配给 CAN2，不能把这种偶然行为作为正确配置保留。

第一次修正试验曾在 `can_filter_init()` 前加入 `MX_CAN2_Init()`。烧录后确认两路 CAN 均进入工作态、错误码为 0、CAN1/CAN2 时钟均开启，过滤器分界值为 14，过滤器组 0 和 14 均已激活，说明 CubeMX 生成的 CAN2 初始化本身有效。

但是进一步审查发现，CAN2 接收回调使用 `i = RX_Header.StdId - CAN2_3508_ID1 + 7`，会写入 `Motor_measure[7...]`，而当前 `Motor_measure` 数组实际上只有 4 个元素。一旦 CAN2 收到对应报文就会产生越界写入。历史固件中的 CAN2 从未真正启用，当前业务代码也没有通过 `hcan2` 发送报文，因此不能在未完成数据模型迁移前直接启用 CAN2。

本切片最终采用安全方案：只初始化和启动 CAN1；将过滤器结构体清零，并在配置 CAN1 前固定设置 `SlaveStartFilterBank = 14`；CAN1 的 HAL 配置、启动和通知步骤都检查返回值。CAN2 的 IOC 配置继续保留，等后续明确电气连接、消息 ID、目标数组容量和业务需求后再单独迁移。

由于当前 CAN1 过滤器使用全零掩码，会接收全部标准帧，接收回调还必须检查报文 ID。只有 `0x201`–`0x204` 才允许映射到四元素 `Motor_measure` 数组；其他 ID 直接忽略。CAN2 未完成独立迁移前，不允许其回调写入该数组。

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

因此，CAN1 已从“手工同步生成结果”升级为“正式 CMake 直接编译 CubeMX 生成的 `can.c`”。旧 `User/Src/can.c` 目前只作为回退参考保留。

### GPIO 归属与启动电平

已验证根固件中的 `MX_GPIO_Init()` 当前只开启端口时钟，下面列出的引脚初始化均被注释。部分引脚稍后由 BSP 模块初始化，而 CubeMX 会立即初始化全部引脚，并在切换为输出模式前把所有已配置输出置为低电平。

| 引脚 | CubeMX 中的用途 | 当前归属或观察结果 | 迁移决定 |
| --- | --- | --- | --- |
| PE11、PF14 | 低电平点亮的红色/绿色 LED | 由 `BSP/src/bsp_led.c` 中的 `LED_GPIO_Config()` 初始化 | 已确认重复归属；复位输出低电平会点亮 LED，确定期望的启动指示状态前不合入。 |
| PB2 | 按键输入 | 由 `BSP/src/bsp_key.c` 中的 `Key_GPIO_Config()` 初始化 | 已确认重复归属；电气配置均为 `GPIO_NOPULL`。 |
| PH2–PH5 | `POWER_1` 至 `POWER_4` 输出 | 未找到这四个引脚的有效 BSP 初始化 | CubeMX 接管前，必须在机器人上确认输出低电平是否安全。 |
| PE4、PE5 | 未命名的下拉输出 | 未找到有效 BSP 初始化 | 用途和安全启动电平尚不明确，继续留在隔离工程中。 |
| PA1 | 下拉输入 | 未找到专用初始化；PA0–PA3 由 L298N 模块使用 | 保持输入模式，合入前确认连接信号。 |

### DMA、TIM2 与中断归属

- CubeMX 为 USART1 RX 生成 DMA2 Stream2 中断处理函数；已验证固件中的对应处理函数被注释，USART1/RC 的初始化归属仍在 BSP 中。
- CubeMX 生成 `HAL_TIM_IRQHandler(&htim2)`；已验证固件的 TIM2 中断处理函数会手工清除更新标志、递增 `m_systick` 并调用 `Update()`。
- 这些差异属于后续 USART1/TIM2 迁移切片。在 CAN 切片中不得复制它们的生成初始化和中断处理函数。

## 不可违反的迁移规则

任何外设都不能同时启用 CubeMX 初始化和旧 BSP 初始化。只有生成实现通过相应验收条件后，才能删除旧实现。

## 当前检查点

CAN1 迁移切片已经完成编译、烧录和上机技术验证，最终 Debug 固件占用 Flash 53,896 字节、RAM 15,672 字节。最终实测结果：

- 程序稳定运行在 `main.c:100` 主循环。
- `hcan1.Instance = 0x40006400`，状态为工作态，错误码为 0。
- CAN1 时钟已开启，CAN2 时钟保持关闭。
- CAN 过滤器分界值为 14，过滤器组 0 已激活。
- `hcan2` 保持全零复位态，且不再被错误调用，因此错误码为 0。
- 连续两次采样中，前三个 `Motor_measure` 均具有有效角度，转矩电流字段持续变化，确认 CAN1 接收中断和电机反馈正常。
- CAN1 接收回调只允许 `0x201`–`0x204` 映射到四元素 `Motor_measure` 数组，其他报文和未迁移的 CAN2 报文不会再造成越界写入。
- 正式 CMake 已直接编译 CubeMX 生成的 `can.c`；`User/Src/can.c` 不再参与 ELF 构建。

CAN2 不属于本次已完成切片。后续必须先明确 CAN2 的硬件连接和业务需求，并修正当前回调对 `Motor_measure[7...]` 的越界风险，才能单独启用和测试 CAN2。

下一步在用户确认机器人执行机构没有异常后，开始 GPIO 归属和安全启动电平切片；不同时迁移 USART1 或 TIM2。
