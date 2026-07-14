# STM32CubeMX 迁移状态

最后更新：2026-07-14

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
6. PB12/PB13 上的 CAN2，1 Mbit/s。
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
- [ ] 只将迁移切片 1 合入正式固件。
- [ ] 删除对应 BSP 初始化前完成上机测试。

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

当前已验证根固件先调用 `MX_CAN1_Init()`，然后调用 `can_filter_init()`；但是 `can_filter_init()` 还会配置、启动 `hcan2` 并启用其通知。目前没有找到有效的 `MX_CAN2_Init()` 调用。这是原有基线中的行为，不是 CubeMX 迁移引入的。上机确认 CAN2 是否连接、是否实际使用之前，不修改这一行为。

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

隔离工程重建、CAN Bus-Off 修正、重复生成保持性验证和离线 Debug 编译均已完成。尚未将任何生成的初始化代码合入已验证的根固件。

下一步是在连接机器人后确认 CAN1/CAN2 的实际使用情况，并确认未解决 GPIO 引脚的安全启动电平。完成这些验证后，才能进行第一次正式的外设初始化所有权切换。
