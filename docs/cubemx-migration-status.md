# STM32CubeMX migration status

Last updated: 2026-07-14

## Branch and baseline

- Development branch: `codex/cubemx-rebuild`
- Verified parent branch: `codex/cmake-vscode`
- Verified parent tag: `v2.0.0-cmake-vscode`
- Keil reference branch: `master`

The current CMake/GCC firmware remains the behavioral reference. CubeMX-generated code must not overwrite the verified root source tree until the corresponding migration slice has passed source review, compilation, and on-board testing.

## Tool and firmware package

- STM32CubeMX: `6.18.0-RC3`
- MCU: `STM32F427IIHx`, UFBGA176
- Firmware package: `STM32Cube FW_F4 V1.26.2`
- Package source: official `STMicroelectronics/STM32CubeF4` tag `v1.26.2`
- Local package path: `C:/Users/Lexi/STM32Cube/Repository/STM32Cube_FW_F4_V1.26.2`
- Compiler target: GNU Arm GCC
- Generated build system: CMake

CubeMX 6.18.0-RC3 is newer than the 6.3.0 version recorded by the existing IOC. It is accepted only for isolated generation on this branch. Generated initialization and interrupt code must be compared with the verified implementation before integration.

The legacy `RM_Robot_on_A_no_os.ioc` cannot be migrated reliably by CubeMX 6.18.0-RC3. Loading it produces `Range [5, 3) out of bounds for length 3` and `Mcu.getDie() is null`. The migration therefore uses a clean STM32F427IIHx project and reconstructs the configuration incrementally. The legacy IOC remains unchanged as a reference.

## Isolation layout

```text
cubemx/
  mobile_charging_robot.ioc   # rebuilt hardware configuration
  generate-stage1.txt         # reproducible CubeMX command-line script
  generated/                  # isolated CubeMX-generated CMake project
```

The existing root `RM_Robot_on_A_no_os.ioc`, `User/`, `BSP/`, `Controller/`, `Entity/`, and root CMake workflow are not generated over in place during the reconstruction.

## Migration slice 1

Scope:

1. STM32F427IIHx and UFBGA176 package.
2. HSE 12 MHz and 168 MHz system clock.
3. APB1 42 MHz and APB2 84 MHz.
4. SWD on PA13/PA14.
5. CAN1 on PD0/PD1 at 1 Mbit/s.
6. CAN2 on PB12/PB13 at 1 Mbit/s.
7. Existing GPIO already represented by the legacy IOC.

USART1 and TIM2 remain present in the seed IOC so the generated project can be compared with the verified code, but ownership migration and BSP cleanup for those peripherals are separate later slices.

## Acceptance gates

- [x] Create the dedicated migration branch from the verified CMake baseline.
- [x] Locate STM32CubeMX and install/verify STM32Cube FW_F4 V1.26.2.
- [x] Save a rebuilt IOC under `cubemx/` without modifying the root IOC.
- [x] Generate a standalone CubeMX CMake project under `cubemx/generated/`.
- [x] Confirm the generated project uses STM32F427IIHx, GCC and the pinned firmware package.
- [x] Review clock, CAN, GPIO, DMA and NVIC differences against the verified firmware and record unresolved ownership decisions below.
- [x] Build the isolated generated project.
- [ ] Integrate only slice 1 into the main firmware.
- [ ] Perform on-board tests before removing any corresponding BSP initialization.

## Completed offline verification

- The generated project uses `STM32F427IIHx`, UFBGA176, GNU Arm GCC and CMake.
- The generated HAL source matches the pinned STM32Cube FW_F4 V1.26.2 package. The checked `stm32f4xx_hal.c` SHA-256 is `666BFDC37EDD048970795C34D180E98F9C512BBFE000B3782B3D2411DA39F35E`, identical to the verified root firmware.
- The isolated Debug build succeeds with GNU Arm Toolchain 14.2 and Ninja.
- Build footprint after the CAN bus-off fix and before application integration: Flash 14,472 bytes, RAM 1,904 bytes, CCMRAM 0 bytes.
- Clock parameters match the verified firmware: PLLM 6, PLLN 168, PLLP 2, PLLQ 4, APB1 divider 4, APB2 divider 2 and Flash latency 5.
- CAN1 and CAN2 timing matches the verified firmware: prescaler 3, SJW 1 TQ, BS1 9 TQ and BS2 4 TQ, giving 1 Mbit/s.

## Differences under review

### CAN

The first generated version set `AutoBusOff = DISABLE`, while the verified firmware sets it to `ENABLE` for both CAN controllers. CubeMX 6.18 uses the IOC parameter name `ABOM` for this setting. `CAN1.ABOM=ENABLE` and `CAN2.ABOM=ENABLE` are now pinned in the rebuilt IOC and regenerate as `AutoBusOff = ENABLE`.

The verified firmware also explicitly enables `CAN_IT_BUSOFF`. The isolated generated project now enables this interrupt in the `CAN1_Init 2` and `CAN2_Init 2` USER CODE regions after `HAL_CAN_Init()`. A second CubeMX regeneration confirmed that both additions are preserved, and the regenerated project builds successfully.

The remaining generated CAN initialization fields, CAN GPIO alternate functions, NVIC priorities and TX/RX0 IRQ handlers match the verified implementation.

The verified root startup currently calls `MX_CAN1_Init()` and then `can_filter_init()`, but `can_filter_init()` also configures, starts and enables notifications on `hcan2`; no active call to `MX_CAN2_Init()` was found. This is an existing baseline behavior, not introduced by CubeMX. Do not change it until an on-board test establishes whether CAN2 is connected and required.

### GPIO ownership and startup levels

The verified root `MX_GPIO_Init()` currently enables port clocks but leaves the listed pin initialization commented out. Some pins are initialized later by BSP modules. CubeMX instead initializes all pins immediately and drives every configured output low before changing its mode.

| Pins | CubeMX role | Current owner or observation | Migration decision |
| --- | --- | --- | --- |
| PE11, PF14 | Active-low red/green LEDs | `LED_GPIO_Config()` in `BSP/src/bsp_led.c` | Duplicate ownership confirmed; reset-low turns LEDs on, so do not integrate until the desired startup indication is decided. |
| PB2 | Key input | `Key_GPIO_Config()` in `BSP/src/bsp_key.c` | Duplicate ownership confirmed; electrical configuration matches `GPIO_NOPULL`. |
| PH2-PH5 | `POWER_1` through `POWER_4` outputs | No active BSP initialization found for these four pins | Reset-low electrical safety must be confirmed on the robot before CubeMX takes ownership. |
| PE4, PE5 | Unlabelled outputs with pulldown | No active BSP initialization found | Function and safe startup level are not yet established; keep isolated. |
| PA1 | Input with pulldown | No dedicated initialization found; PA0-PA3 are used by the L298N module | Keep as input; confirm the connected signal before integration. |

### DMA, TIM2 and interrupt ownership

- CubeMX generates a DMA2 Stream2 handler for USART1 RX, while the verified handler is commented out and USART1/RC ownership remains in BSP code.
- CubeMX generates `HAL_TIM_IRQHandler(&htim2)`, while the verified TIM2 handler manually clears the update flag, increments `m_systick` and calls `Update()`.
- These differences belong to later USART1/TIM2 ownership slices. Their generated initialization and handlers must not be copied during the CAN slice.

## Non-negotiable migration rule

For any peripheral, CubeMX initialization and the old BSP initialization must never be active simultaneously. The old implementation is removed only after the generated replacement has passed its acceptance gates.

## Current checkpoint

The isolated reconstruction, CAN bus-off correction, regeneration-safety check and offline Debug build are complete. No generated initialization has been integrated into the verified root firmware. The next executable step is an on-board CAN1/CAN2 usage check and startup-level confirmation for the unresolved GPIO pins; only then may the first ownership change be made in the root firmware.
