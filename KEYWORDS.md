# ZonalECU-FWv2 Project Keywords & Acronym Dictionary

This document serves as the single source of truth for all official subsystem prefixes, acronyms, and uppercase naming keywords used across `FW-ESP`, `FW-STM`, and `FW-Common`.

---

## 1. Official Uppercase Subsystem Prefixes (Full Caps)
Used for **Public Functions** (`<PREFIX>_<PascalCase>()`), **Macros**, and **Enum Labels**:

| Prefix | Subsystem / Domain | Scope | Example Public Function | Example Macro / Enum |
| :--- | :--- | :--- | :--- | :--- |
| **`ZECU_`** | Zonal ECU Core & Error System | Cross-target | `ZECU_Init()`, `ZECU_ErrToStr()` | `ZECU_OK`, `ZECU_ERR_INVALID_ARG` |
| **`ZE_`** | Zonal ECU Core Short Prefix | Cross-target | `ZE_Init()`, `ZE_HEADER`, `ZE_FOOTER` | `ZE_VERSION_MAJOR` |
| **`SOMEIP_`** | AUTOSAR SOME/IP Protocol | `FW-ESP` | `SOMEIP_HeaderEncode()`, `SOMEIP_Ser_WriteBytes()` | `SOMEIP_PROTOCOL_VERSION`, `SOMEIP_RES_OK` |
| **`SOMEIP_SD_`** | SOME/IP Service Discovery | `FW-ESP` | `SOMEIP_SD_FindService()`, `SOMEIP_SD_OfferService()` | `SOMEIP_SD_FLAG_REBOOT` |
| **`ETH_`** | Ethernet Subsystem (W5500) | `FW-ESP` | `ETH_Open()`, `ETH_SendPacket()` | `ETH_MAX_PKT_BYTES`, `ETH_STATE_OPEN` |
| **`W5500_`** | W5500 Hardware Controller | `FW-ESP` | `W5500_Reset()`, `W5500_ReadReg()` | `W5500_REG_MR`, `W5500_SPI_MODE` |
| **`CAN_`** | Classic CAN Bus Subsystem | Multi-target | `CAN_Init()`, `CAN_TransmitFrame()` | `CAN_BAUDRATE_500K`, `CAN_STD_ID_MAX` |
| **`CANCC_`** | Classic CAN Controller | `FW-STM` | `CANCC_ConfigureFilter()`, `CANCC_ReadFifo()` | `CANCC_ERR_BUS_OFF` |
| **`SPI_`** | Serial Peripheral Interface | Multi-target | `SPI_Transfer()`, `SPI_Init()` | `SPI_MODE_0`, `SPI_CLOCK_10MHZ` |
| **`UART_`** | Universal Asynchronous Receiver | Multi-target | `UART_Write()`, `UART_Read()` | `UART_BAUD_115200` |
| **`I2C_`** | Inter-Integrated Circuit Bus | Multi-target | `I2C_MasterWrite()`, `I2C_MasterRead()` | `I2C_SPEED_FAST` |
| **`GPIO_`** | General Purpose Input / Output | Multi-target | `GPIO_SetLevel()`, `GPIO_GetLevel()` | `GPIO_DIR_OUTPUT`, `GPIO_PULLUP` |
| **`ADC_`** | Analog-to-Digital Converter | `FW-STM` | `ADC_StartConversion()`, `ADC_GetSample()` | `ADC_RESOLUTION_12BIT` |
| **`PWM_`** | Pulse Width Modulation Actuator | `FW-STM` | `PWM_SetDutyCycle()`, `PWM_Start()` | `PWM_PRESCALER_MAX` |
| **`FLASH_`** | Non-Volatile Flash Storage | Multi-target | `FLASH_Write()`, `FLASH_Erase()` | `FLASH_SECTOR_SIZE` |
| **`NVS_`** | Non-Volatile Storage (ESP32) | `FW-ESP` | `NVS_SetKey()`, `NVS_GetKey()` | `NVS_PARTITION_NAME` |
| **`PSRAM_`** | Pseudo-Static External RAM | `FW-ESP` | `PSRAM_Malloc()`, `PSRAM_Free()` | `PSRAM_CHIP_SIZE` |
| **`LOG_`** | Logging & Telemetry Backend | Multi-target | `LOG_Info()`, `LOG_Warn()`, `LOG_Error()` | `LOG_LEVEL_DEBUG` |
| **`IPC_`** | Inter-Process Communication | `FW-ESP` | `IPC_SendQueue()`, `IPC_ReceiveQueue()` | `IPC_QUEUE_TIMEOUT_MS` |

---

## 2. Official Lowercase Type Prefixes (snake_case_t)
Used for all **Typedefs**, **Structs**, **Unions**, and **Enums** (`<prefix>_<name>_t`):

* `zecu_<name>_t`: e.g. `zecu_err_t`, `zecu_state_t`
* `someip_<name>_t`: e.g. `someip_header_t`, `someip_result_t`, `someip_ser_cursor_t`
* `eth_<name>_t`: e.g. `eth_cfg_t`, `eth_ctrl_t`, `eth_speed_t`
* `can_<name>_t`: e.g. `can_msg_t`, `can_filter_t`
* `spi_<name>_t`: e.g. `spi_dev_t`, `spi_cfg_t`
* `uart_<name>_t`: e.g. `uart_port_t`, `uart_buf_t`
* `gpio_<name>_t`: e.g. `gpio_pin_t`, `gpio_mode_t`

---

## 3. General Acronyms & Spellcheck Words
Registered terms to prevent false positives in IDE spell checkers (`cSpell`, `codespell`):

* `ZECU` (Zonal Electronic Control Unit)
* `ECU` (Electronic Control Unit)
* `AUTOSAR` (Automotive Open System Architecture)
* `SOMEIP` (Scalable service-Oriented MiddlewarE over IP)
* `CANCC` (Controller Area Network Classic Controller)
* `bxCAN` (Basic Extended CAN on STM32)
* `W5500` (WIZnet Hardwired TCP/IP chip)
* `FSP` (Flexible Software Package / Coding Style)
* `MISRA` (Motor Industry Software Reliability Association)
* `CMSIS` (Cortex Microcontroller Software Interface Standard)

---

## 4. Custom User Keywords (Thêm từ khóa mới của bạn vào đây)
Thêm bất kỳ từ khóa hoặc tiền tố viết tắt mới nào bạn muốn chuẩn hóa vào bảng dưới:

| Keyword | Ý nghĩa / Phạm vi sử dụng | Ghi chú |
| :--- | :--- | :--- |
| `ZECU` | Tên chính của hệ thống Zonal ECU | Full hoa |
| `ZE` | Tiền tố viết tắt ngắn của Zonal ECU | Full hoa (VD: `ZE_HEADER`) |
| `SOMEIP` | Tên giao thức SOME/IP | Full hoa trong macro/hàm |
| *(Bạn có thể điền thêm dòng mới vào đây)* | | |
