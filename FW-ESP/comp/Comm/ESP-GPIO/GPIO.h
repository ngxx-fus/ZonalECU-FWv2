/********************************************************************************************************************************
 * @file GPOI.h
 * @ingroup ZonalECU_FWv2
 * @brief Header file for [[GPOI]]
 * @details 
 *      TYPE:
 *          - ze_gpio_t (uint64_t)
 *          - ze_gpio_pin_t (enum-based number, only valid GPIO)
 *      API:
 *          - ZE_IO_PinEnable(ze_gpio_t pin_mask)
 *          - ZE_IO_PinDisable(ze_gpio_t pin_mask)
 *          - ZE_IO_PinWrite()
 *  
 *  
 ********************************************************************************************************************************/

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __ZONALECU_FWV2_FW_ESP_COMP_COMM_ESP_GPIO_GPOI_H__
#define __ZONALECU_FWV2_FW_ESP_COMP_COMM_ESP_GPIO_GPOI_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "Common/Base/All.h"
#include "Common/Error/All.h"
#include "soc/gpio_periph.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"
#include <stdint.h>

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_HEADER

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef VALID
    #define VALID                   (1U)          /*!< Valid GPIO pin indicator */
#endif
#ifndef INVALID
    #define INVALID                 (0U)          /*!< Invalid GPIO pin indicator */
#endif

#define ZE_GPIO_NUM             (0)           /*!< Number of valid GPIOs */
#define ZE_GPIO_NUM_MAX         (64)          /*!< Number of design GPIOs */
#define ZE_GPIO_VALID_MASK_LOW  (0xFC3FFFFFU) /*!< Valid GPIO mask for pins 0..31 (excluding pins 22..25) */
#define ZE_GPIO_VALID_MASK_HIGH (0x0001FFFFU) /*!< Valid GPIO mask for pins 32..48 */
#define ZE_GPIO_QUICK_GET_ERR   (0xFFFFFFFFU) /*!< Error return value for ZE_GPIO_LevelQuickGet */

#define ZE_GPIO_INIT_DEFAULT    {.valid = {.word_l = ZE_GPIO_VALID_MASK_LOW, .word_h = ZE_GPIO_VALID_MASK_HIGH}, .conf = {0}}

/** @brief Fast direct register write to set GPIO pin to HIGH level */
#define ZE_GPIO_FAST_SET(pin)                                                                                                    \
    do {                                                                                                                         \
        if ((pin) < 32U) {                                                                                                       \
            GPIO.out_w1ts = (1U << (pin));                                                                                       \
        } else {                                                                                                                 \
            GPIO.out1_w1ts.val = (1U << ((pin) - 32U));                                                                          \
        }                                                                                                                        \
    } while (0)

/** @brief Fast direct register write to clear GPIO pin to LOW level */
#define ZE_GPIO_FAST_CLR(pin)                                                                                                    \
    do {                                                                                                                         \
        if ((pin) < 32U) {                                                                                                       \
            GPIO.out_w1tc = (1U << (pin));                                                                                       \
        } else {                                                                                                                 \
            GPIO.out1_w1tc.val = (1U << ((pin) - 32U));                                                                          \
        }                                                                                                                        \
    } while (0)

/** @brief Fast direct register write to set GPIO pin to specific logic level */
#define ZE_GPIO_FAST_WRITE(pin, level)                                                                                           \
    do {                                                                                                                         \
        if (level) {                                                                                                             \
           ZE_GPIO_FAST_SET(pin);                                                                                                \
        } else {                                                                                                                 \
           ZE_GPIO_FAST_CLR(pin);                                                                                                \
        }                                                                                                                        \
    } while (0)

/** @brief Fast direct register read of GPIO pin logic level (pure register access, returns 0 or 1) */
#define ZE_GPIO_FAST_GET(pin)                                                                                                    \
    (((pin) < 32U) ? ((REG_READ(GPIO_IN_REG) >> (pin)) & 1U) : ((REG_READ(GPIO_IN1_REG) >> ((pin) - 32U)) & 1U))

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Operating mode configuration for GPIO pins */
typedef enum ze_io_mode_e {
    ZE_GPIO_DISABLED     = 0U, /*!< Pin is disabled / floating high-impedance */
    ZE_OUTPUT_PUSH_PULL  = 1U, /*!< General-purpose output with push-pull driving */
    ZE_OUTPUT_OPEN_DRAIN = 2U, /*!< General-purpose output with open-drain driving */
    ZE_INPUT_ONLY        = 3U, /*!< Input mode with digital input buffer enabled */
    ZE_EXT_POS_EDGE_TRIG = 4U, /*!< External interrupt trigger on positive (rising) edge */
    ZE_EXT_NEG_EDGE_TRIG = 5U, /*!< External interrupt trigger on negative (falling) edge */
    ZE_PERIPHERAL_RSVD_0 = 6U, /*!< Reserved for peripheral hardware function 0 */
    ZE_PERIPHERAL_RSVD_1 = 7U, /*!< Reserved for peripheral hardware function 1 */
} ze_io_mode_t;

/** @brief Logical level and operational state of GPIO pins */
typedef enum ze_io_level_e {
    /* Mode for default, not set. */

    ZE_STAT_DISABLED = 0U, /*!< Pin is disabled (level is not determined / inactive). */
    ZE_STAT_DANGLING = 1U, /*!< Pin is used in peripheral mode (level is managed by peripheral). */

    /* Mode for actively config by user. */

    ZE_STAT_LOW      = 2U, /*!< Pin has LOW logic level (0V / GND). */
    ZE_STAT_HIGH     = 3U, /*!< Pin has HIGH logic level (VCC / floating in OpenDrain). */
} ze_io_level_t;

/** @brief Internal pull resistor configuration */
typedef enum ze_io_pull_mode_e {
    ZE_PULL_NONE      = 0U, /*!< No internal pull resistor enabled (floating) */
    ZE_PULL_UP        = 1U, /*!< Internal pull-up resistor enabled */
    ZE_PULL_DOWN      = 2U, /*!< Internal pull-down resistor enabled */
    ZE_PULL_UP_N_DOWN = 3U, /*!< Both pull-up and pull-down resistors enabled (bus keeper) */
} ze_io_pull_mode_t;

/** @brief Hardware validity indicator for GPIO pins */
typedef enum ze_io_hw_stat_e {
    ZE_HW_INVALID = 0U, /*!< Pin does not physically exist or is reserved by hardware */
    ZE_HW_VALID   = 1U, /*!< Pin physically exists and is available for GPIO use */
} ze_io_hw_stat_t;

/** @brief 1-byte packed configuration structure for a single GPIO pin */
typedef struct ze_gpio_conf_st {
    rw ze_io_mode_t mode : 3;      /*!< Operating mode (3 bits: 0..7) */
    rw ze_io_pull_mode_t pull : 3; /*!< Internal pull resistor mode (3 bits: 0..3) */
    rw ze_io_level_t level : 2;    /*!< Logical level / state (2 bits: 0..3) */
} ze_gpio_conf_t;

/** @brief 64-bit mask for GPIO operations across lower and upper 32-bit registers */
typedef union ze_gpio_64_un {
    uint64_t dword;      /*!< Full 64-bit double-word mask */
    struct {
        uint32_t word_l; /*!< Lower 32 pins (GPIO0..GPIO31) */
        uint32_t word_h; /*!< Upper 32 pins (GPIO32..GPIO63) */
    };
} ze_gpio_dword_t;

/** @brief Global ECU GPIO controller state and configuration structure */
typedef struct ze_gpio_st {
    ro ze_gpio_dword_t valid;                  /*!< Constant mask of all GPIO pins valid to use on hardware */
    rw ze_gpio_conf_t  conf [ZE_GPIO_NUM_MAX]; /*!< Configuration array for each pin (GPIO0..GPIO63) */
} ze_gpio_t;

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* SHARED VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Global GPIO master controller state and hardware validity structure */
extern ze_gpio_t g_ze_gpio;

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Apply pin configurations and enable all valid active GPIOs according to control structure
 *  @param   p_ctrl Pointer to GPIO control structure containing target pin configurations
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_Enable(IN ze_gpio_t ro_ptr p_ctrl);

/** @brief   Disable GPIO pins and isolate hardware pads according to control structure
 *  @param   p_ctrl Pointer to GPIO control structure
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_Disable(IN ze_gpio_t ro_ptr p_ctrl);

/** @brief   Synchronize entire software configuration down to physical hardware registers
 *  @param   p_ctrl Pointer to GPIO control structure with target configurations
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_HardwareSync(IN ze_gpio_t ro_ptr p_ctrl);

/** @brief   Set output logic level for a specific GPIO pin
 *  @param   pin_num Target GPIO pin index (0..48)
 *  @param   level   Desired logic level (ZE_STAT_LOW or ZE_STAT_HIGH)
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PinSet(IN uint32_t pin_num, IN ze_io_level_t level);

/** @brief   Read physical logic level of a specific GPIO pin
 *  @param   pin_num Target GPIO pin index (0..48)
 *  @param   p_level Pointer to receive detected logic level
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PinGet(IN uint32_t pin_num, OUT ze_io_level_t rw_ptr p_level);

/** @brief   Fast atomic write of logic level to a single GPIO pin register
 *  @param   pin_num Target GPIO pin index (0..48)
 *  @param   level   Desired logic level (ZE_STAT_LOW or ZE_STAT_HIGH)
 *  @return  0 on success, or ZE_GPIO_QUICK_GET_ERR on invalid pin/argument */
uint32_t ZE_GPIO_LevelQuickSet(IN uint32_t pin_num, IN ze_io_level_t level);

/** @brief   Fast direct read of logic level from single GPIO pin hardware register
 *  @param   pin_num Target GPIO pin index (0..48)
 *  @return  0 or 1 on success, or ZE_GPIO_QUICK_GET_ERR on invalid pin */
uint32_t ZE_GPIO_LevelQuickGet(IN uint32_t pin_num);

/** @brief   Atomically set multiple GPIO pins to HIGH level simultaneously according to 64-bit mask
 *  @param   mask 64-bit mask of target pins to set HIGH
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PortSet(IN ze_gpio_dword_t mask);

/** @brief   Atomically clear multiple GPIO pins to LOW level simultaneously according to 64-bit mask
 *  @param   mask 64-bit mask of target pins to clear LOW
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PortClear(IN ze_gpio_dword_t mask);

/** @brief   Read physical logic levels of all GPIO pins simultaneously from hardware input registers
 *  @param   p_val Pointer to receive 64-bit pin level mask
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PortGet(OUT ze_gpio_dword_t rw_ptr p_val);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __ZONALECU_FWV2_FW_ESP_COMP_COMM_ESP_GPIO_GPOI_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/