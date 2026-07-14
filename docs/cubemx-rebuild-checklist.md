# STM32CubeMX rebuild checklist

Rebuild this configuration on `codex/cubemx-rebuild`, created from the verified `codex/cmake-vscode` / `v2.0.0-cmake-vscode` baseline. Do not generate CubeMX code directly on `master` or overwrite the current verified source tree in place. The CMake firmware remains the behavioral reference until each peripheral has been tested on hardware.

## Device and clock

- MCU: STM32F427IIHx
- Flash: 2 MB at `0x08000000`
- Main SRAM: 192 KB at `0x20000000`
- CCM SRAM: 64 KB at `0x10000000`
- HSE: 12 MHz
- SYSCLK: 168 MHz
- APB1: 42 MHz, timer clock 84 MHz
- APB2: 84 MHz, timer clock 168 MHz
- FPU: single precision, hard-float ABI

Keep the existing clock tree unchanged during the first CubeMX reconstruction.

## Existing CubeMX resources

- CAN1: PD0 RX, PD1 TX
- CAN2: PB12 RX, PB13 TX
- USART1: PB7 RX, PB6 TX
- USART1 RX DMA: DMA2 Stream2
- TIM2: internal clock and update interrupt
- SWD: PA13 and PA14
- HSE: PH0 and PH1
- Key: PB2 input
- Red LED: PE11 output
- Green LED: PF14 output
- Power outputs: PH2, PH3, PH4, PH5
- Other existing GPIO: PA1 input, PE4 output, PE5 output

## BSP-owned serial resources

### Battery

- Peripheral: USART6, 9600 8-N-1
- TX: PG9, AF8
- RX: PG14, AF8
- TX DMA: DMA2 Stream6, channel 5
- RX DMA: DMA2 Stream1, channel 5
- IRQs: USART6 and DMA2 Stream6
- Source: `BSP/src/bsp_485_battery.c`

### Server

- Peripheral: UART7, 9600 8-N-1
- TX: PE8, AF8
- RX: PE7, AF8
- TX DMA: DMA1 Stream1, channel 5
- RX DMA: DMA1 Stream3, channel 5
- IRQs: UART7, DMA1 Stream1, and DMA1 Stream3
- Source: `BSP/src/bsp_485_server.c`

### RFID

- Peripheral: UART8, 38400 8-N-1
- TX: PE1, AF8
- RX: PE0, AF8
- TX DMA: DMA1 Stream0, channel 5
- RX DMA: DMA1 Stream6, channel 5
- IRQs: UART8 and DMA1 Stream0
- Source: `BSP/src/bsp_485_rfid.c`

### Remote control

- Peripheral: USART1, 100000 baud, 8-bit word length, even parity, 1 stop bit, RX only
- RX: PB7, AF7
- RX DMA: DMA2 Stream2, channel 4, circular mode, very high priority
- IRQ: USART1
- Source: `BSP/src/bsp_rc.c`

The USART1 settings in the BSP must be treated as authoritative when reconstructing CubeMX; they are not conventional 8-N-1 settings.

## BSP-owned timers and GPIO

### Timing

- TIM2: 1 ms update interrupt used by `Time_Init()` and `Update()`
- TIM4 CH2 PWM trigger: PD13, AF2
- Ultrasonic echo input paired with TIM4: PD12, GPIO input with pulldown
- TIM5 CH1 PWM trigger: PH10, AF2
- Ultrasonic echo input paired with TIM5: PH11, GPIO input with pulldown

Verify TIM4/TIM5 prescalers, periods, polarity, and channels directly against `BSP/src/bsp_ultrawave.c` before entering them in CubeMX.

### Digital I/O

- Push rod outputs: PA0, PA1, PA2, PA3
- Front/rear limit switches: PI6, PI7 inputs with pulldown
- Photogate: PI2 input with pullup
- RGB outputs: PD14, PD15, PH12
- LEDs: PE11 and PF14
- Key: PB2

Use the active `.c` implementation as the source of truth. Some older BSP headers contain stale pin comments that do not match the current implementation.

## Migration order

1. Recreate the MCU, clock tree, SWD, CAN1/CAN2, and existing GPIO.
2. Add USART1 with its actual 100000 baud/parity/DMA settings and verify remote control input.
3. Add TIM2 and verify the 1 ms control update.
4. Add USART6, UART7, and UART8 one at a time, including DMA and NVIC.
5. Add TIM4/TIM5 ultrasonic resources.
6. Add remaining GPIO and verify actuators with motors mechanically disconnected.
7. Move each hardware initialization function out of BSP only after its CubeMX-generated replacement passes an on-board test.
8. Keep each IRQ handler defined in exactly one translation unit.

Do not enable a peripheral in CubeMX while retaining a second BSP initialization path for the same handle, DMA stream, or IRQ.
