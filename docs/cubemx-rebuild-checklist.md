# STM32CubeMX 重建检查清单

在 `codex/cubemx-rebuild` 分支上重建配置。该分支来自已验证的 `codex/cmake-vscode` / `v2.0.0-cmake-vscode` 基线。不得在 `master` 上直接生成 CubeMX 代码，也不得让 CubeMX 原地覆盖正式源码。每个外设切片完成上机测试前，都以已验证的 CMake 固件作为行为参考。

## 芯片与时钟

- MCU: STM32F427IIHx
- Flash: 2 MB at `0x08000000`
- Main SRAM: 192 KB at `0x20000000`
- CCM SRAM: 64 KB at `0x10000000`
- HSE: 12 MHz
- SYSCLK: 168 MHz
- APB1: 42 MHz, timer clock 84 MHz
- APB2: 84 MHz, timer clock 168 MHz
- FPU: single precision, hard-float ABI

首次重建期间保持现有时钟树不变。

## 已重建的 CubeMX 资源

- CAN1: PD0 RX, PD1 TX
- USART1: PB7 RX, PB6 TX
- USART1 RX DMA: DMA2 Stream2
- TIM2: internal clock and update interrupt
- SWD: PA13 and PA14
- HSE: PH0 and PH1
- Red LED: PE11 output
- Green LED: PF14 output
- Power outputs: PH2, PH3, PH4, PH5
- Other existing GPIO: PA1 input, PE4 output, PE5 output

## 已迁移的串口资源

### Battery

- Peripheral: USART6, 9600 8-N-1
- TX: PG14, AF8
- RX: PG9, AF8
- TX DMA: DMA2 Stream6, channel 5
- RX DMA: DMA2 Stream1, channel 5
- IRQs: USART6 and DMA2 Stream6
- 业务源文件：`BSP/src/bsp_485_battery.c`

### Server

- Peripheral: UART7, 9600 8-N-1
- TX: PE8, AF8
- RX: PE7, AF8
- TX DMA: DMA1 Stream1, channel 5
- RX DMA: DMA1 Stream3, channel 5
- IRQs: UART7, DMA1 Stream1, and DMA1 Stream3
- 业务源文件：`BSP/src/bsp_485_server.c`

### RFID

- Peripheral: UART8, 38400 8-N-1
- TX: PE1, AF8
- RX: PE0, AF8
- TX DMA: DMA1 Stream0, channel 5
- RX DMA: DMA1 Stream6, channel 5
- IRQs: UART8 and DMA1 Stream0
- 业务源文件：`BSP/src/bsp_485_rfid.c`

### Remote control

- Peripheral: USART1, 100000 baud, 8-bit word length, even parity, 1 stop bit, RX only
- RX: PB7, AF7
- RX DMA: DMA2 Stream2, channel 4, circular mode, very high priority
- IRQ: USART1
- 业务源文件：`BSP/src/bsp_rc.c`

USART1 不是常规 8-N-1 配置，重建时必须以已验证参数为准。

## 定时器与 GPIO

### 定时器

- TIM2：当前实际周期约 0.5 ms，由 CubeMX 初始化并驱动 `Update()` 控制调度。
- TIM4/TIM5：原用于 Ultrawave，但机器人实际未使用；阶段 3 明确排除，不迁移进 IOC。

Ultrawave 的 TIM4/TIM5、PD12/PD13、PH10/PH11 旧实现已在清理前硬件验证基线提交 `8154cca` 中保留，正式分支不再编译或维护该模块。

### 数字 GPIO

- Push rod outputs: PA0, PA1, PA2, PA3
- Front/rear limit switches: PI6, PI7 inputs with pulldown
- Photogate: PI2 input with pullup
- RGB outputs: PD14, PD15, PH12
- LEDs: PE11 and PF14
- PB2 Key：没有业务读取，阶段 3 明确排除。

以正式 `.c` 实现和已验证 IOC 为准；部分旧 BSP 头文件中的引脚注释已经过时。

## 迁移顺序与当前状态

1. [x] 重建 MCU、时钟树、SWD、CAN1 和实际使用的 GPIO。
2. [x] 迁移 USART1 100000 baud、偶校验、RX DMA，并验证遥控器。
3. [x] 迁移 TIM2 并验证控制调度。
4. [x] 依次迁移 USART6、UART7、UART8、DMA 和 NVIC。
5. [x] 迁移 L298N、限位、光电门、RGB 和 LED GPIO。
6. [x] 删除已由 CubeMX 替代的 BSP GPIO 初始化函数。
7. [x] 删除未使用的 Ultrawave、Key 和 Debug USART 模块。
8. [x] 删除当前 CMake 分支中的 Keil/EIDE 工程、旧 IOC、未编译示例和可重新生成缓存；远程 `master` 保留完整 Keil 基线。
9. [x] 确认 CAN2 不使用，并从 IOC、生成代码、正式源码、NVIC 和 BSP 枚举中删除。
10. [x] 对清理后的最终版本执行整机上机回归，CAN1、RC、TIM2、RFID、Battery、Server、光电门、后限位、RGB、推杆、LED 和 LiveWatch 通过。
11. [x] 阶段 3 封版并使用 `v3.0.0-cubemx` 标签保存；该版本与远程 `master` 上的 Keil 基线分开维护。

不得在 CubeMX 已启用某个外设后，继续保留针对同一句柄、DMA Stream 或 IRQ 的第二套 BSP 初始化路径。每个 IRQ 处理函数必须只有一个强定义。
