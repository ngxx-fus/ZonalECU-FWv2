/********************************************************************************************************************************
 * @file GPOI.c
 * @ingroup ZonalECU_FWv2
 * @brief Source file for [[GPOI]] driver on ESP32-S3
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"
#include "hal/gpio_types.h"
#include "soc/gpio_periph.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"
#include "soc/soc_caps.h"

#include "Common/Base/All.h"
#include "Common/Bitwise/All.h"
#include "Common/Error/Error.h"
#include "GPIO.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define ZE_GPIO_LOW_PIN_COUNT (32U)

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

ze_gpio_t g_ze_gpio = {
    /* List of valid pin */
    .valid = {.word_l = ZE_GPIO_VALID_MASK_LOW,.word_h = ZE_GPIO_VALID_MASK_HIGH,},
    /* Default value of pin configuration */
    .conf  = {{0}},
};

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Check whether a given GPIO pin number is valid on ESP32-S3 hardware
 *  @param   pin_num Target pin index (0..48)
 *  @return  true if valid, false if invalid or reserved */
static inline bool prv_is_valid_pin(IN uint32_t pin_num) {
    if (pin_num < ZE_GPIO_LOW_PIN_COUNT) {
        return (0U != (g_ze_gpio.valid.word_l & (1U << pin_num)));
    }
    if (pin_num < ZE_GPIO_NUM_MAX) {
        return (0U != (g_ze_gpio.valid.word_h & (1U << (pin_num - ZE_GPIO_LOW_PIN_COUNT))));
    }
    return false;
}

/** @brief   Disable hardware GPIO pad, isolating output and input buffers
 *  @param   pin_num Target GPIO pin index */
static inline void prv_configure_pin_disable(IN uint32_t pin_num) {
   gpio_ll_output_disable(&GPIO, pin_num);
   gpio_ll_input_disable (&GPIO, pin_num);
   gpio_ll_pulldown_dis  (&GPIO, pin_num);
   gpio_ll_pullup_dis    (&GPIO, pin_num);
}

/** @brief   Configure hardware pad according to specific pin configuration
 *  @param   pin_num Target GPIO pin index
 *  @param   conf    Target pin configuration */
static inline void prv_configure_pin(IN uint32_t pin_num, IN ze_gpio_conf_t conf) {
    if (ZE_GPIO_DISABLED == conf.mode) {
       prv_configure_pin_disable(pin_num);
        return;
    }

   esp_rom_gpio_pad_select_gpio(pin_num);
   gpio_ll_func_sel            (&GPIO, (uint8_t)pin_num, PIN_FUNC_GPIO);

    switch (conf.mode) {
        case ZE_OUTPUT_PUSH_PULL:
           gpio_ll_output_enable(&GPIO, pin_num);
           gpio_ll_od_disable   (&GPIO, pin_num);
           gpio_ll_input_enable (&GPIO, pin_num);
            break;
        case ZE_OUTPUT_OPEN_DRAIN:
           gpio_ll_output_enable(&GPIO, pin_num);
           gpio_ll_od_enable    (&GPIO, pin_num);
           gpio_ll_input_enable (&GPIO, pin_num);
            break;
        case ZE_INPUT_ONLY:
           gpio_ll_output_disable(&GPIO, pin_num);
           gpio_ll_input_enable  (&GPIO, pin_num);
            break;
        case ZE_EXT_POS_EDGE_TRIG:
           gpio_ll_output_disable(&GPIO, pin_num);
           gpio_ll_input_enable  (&GPIO, pin_num);
           gpio_ll_set_intr_type (&GPIO, pin_num, GPIO_INTR_POSEDGE);
            break;
        case ZE_EXT_NEG_EDGE_TRIG:
           gpio_ll_output_disable(&GPIO, pin_num);
           gpio_ll_input_enable  (&GPIO, pin_num);
           gpio_ll_set_intr_type (&GPIO, pin_num, GPIO_INTR_NEGEDGE);
            break;
        default:
            break;
    }

    switch (conf.pull) {
        case ZE_PULL_NONE:
           gpio_ll_pullup_dis  (&GPIO, pin_num);
           gpio_ll_pulldown_dis(&GPIO, pin_num);
            break;
        case ZE_PULL_UP:
           gpio_ll_pullup_en   (&GPIO, pin_num);
           gpio_ll_pulldown_dis(&GPIO, pin_num);
            break;
        case ZE_PULL_DOWN:
           gpio_ll_pullup_dis (&GPIO, pin_num);
           gpio_ll_pulldown_en(&GPIO, pin_num);
            break;
        case ZE_PULL_UP_N_DOWN:
           gpio_ll_pullup_en  (&GPIO, pin_num);
           gpio_ll_pulldown_en(&GPIO, pin_num);
            break;
        default:
            break;
    }

    if (ZE_STAT_HIGH == conf.level) {
       gpio_ll_set_level(&GPIO, pin_num, 1U);
    } else if (ZE_STAT_LOW == conf.level) {
       gpio_ll_set_level(&GPIO, pin_num, 0U);
    }
}

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Apply pin configurations and enable all valid active GPIOs according to control structure
 *  @param   p_ctrl Pointer to GPIO control structure containing target pin configurations
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_Enable(IN ze_gpio_t ro_ptr p_ctrl) {
    if (IS_NULL(p_ctrl)) {
        return ZE_ERR_INVALID_ARG;
    }

    for (uint32_t pin = 0U; pin < ZE_GPIO_NUM_MAX; ++pin) {
        if (!prv_is_valid_pin(pin)) {
            continue;
        }

        if (ZE_GPIO_DISABLED != p_ctrl->conf[pin].mode) {
           prv_configure_pin(pin, p_ctrl->conf[pin]);
            g_ze_gpio.conf[pin] = p_ctrl->conf[pin];
        }
    }

    return ZE_OK;
}

/** @brief   Disable GPIO pins and isolate hardware pads according to control structure
 *  @param   p_ctrl Pointer to GPIO control structure
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_Disable(IN ze_gpio_t ro_ptr p_ctrl) {
    if (IS_NULL(p_ctrl)) {
        return ZE_ERR_INVALID_ARG;
    }

    for (uint32_t pin = 0U; pin < ZE_GPIO_NUM_MAX; ++pin) {
        if (!prv_is_valid_pin(pin)) {
            continue;
        }

        if (ZE_GPIO_DISABLED == p_ctrl->conf[pin].mode) {
           prv_configure_pin_disable(pin);
            g_ze_gpio.conf[pin].mode = ZE_GPIO_DISABLED;
            g_ze_gpio.conf[pin].level = ZE_STAT_DISABLED;
        }
    }

    return ZE_OK;
}

/** @brief   Synchronize entire software configuration down to physical hardware registers
 *  @param   p_ctrl Pointer to GPIO control structure with target configurations
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_HardwareSync(IN ze_gpio_t ro_ptr p_ctrl) {
    if (IS_NULL(p_ctrl)) {
        return ZE_ERR_INVALID_ARG;
    }

    for (uint32_t pin = 0U; pin < ZE_GPIO_NUM_MAX; ++pin) {
        if (!prv_is_valid_pin(pin)) {
            continue;
        }

       prv_configure_pin(pin, p_ctrl->conf[pin]);
        g_ze_gpio.conf[pin] = p_ctrl->conf[pin];
    }

    return ZE_OK;
}

/** @brief   Set output logic level for a specific GPIO pin
 *  @param   pin_num Target GPIO pin index (0..48)
 *  @param   level   Desired logic level (ZE_STAT_LOW or ZE_STAT_HIGH)
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PinSet(IN uint32_t pin_num, IN ze_io_level_t level) {
    if (!prv_is_valid_pin(pin_num)) {
        return ZE_ERR_INVALID_ARG;
    }

    /* Guard: Do not allow setting level on an uninitialized / disabled pin */
    if (ZE_GPIO_DISABLED == g_ze_gpio.conf[pin_num].mode) {
        return ZE_ERR_INVALID_STATE;
    }

    if (ZE_STAT_HIGH == level) {
        if (pin_num < ZE_GPIO_LOW_PIN_COUNT) {
            GPIO.out_w1ts = (1U << pin_num);
        } else {
            GPIO.out1_w1ts.val = (1U << (pin_num - ZE_GPIO_LOW_PIN_COUNT));
        }
        g_ze_gpio.conf[pin_num].level = ZE_STAT_HIGH;
    } else if (ZE_STAT_LOW == level) {
        if (pin_num < ZE_GPIO_LOW_PIN_COUNT) {
            GPIO.out_w1tc = (1U << pin_num);
        } else {
            GPIO.out1_w1tc.val = (1U << (pin_num - ZE_GPIO_LOW_PIN_COUNT));
        }
        g_ze_gpio.conf[pin_num].level = ZE_STAT_LOW;
    } else {
        return ZE_ERR_INVALID_ARG;
    }

    return ZE_OK;
}

/** @brief   Read physical logic level or state of a specific GPIO pin
 *  @param   pin_num Target GPIO pin index (0..48)
 *  @param   p_level Pointer to receive detected logic level / state
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PinGet(IN uint32_t pin_num, OUT ze_io_level_t rw_ptr p_level) {
    if (!prv_is_valid_pin(pin_num) || IS_NULL(p_level)) {
        return ZE_ERR_INVALID_ARG;
    }

    /* Pin is disabled: return ZE_STAT_DISABLED directly */
    if (ZE_GPIO_DISABLED == g_ze_gpio.conf[pin_num].mode) {
        *p_level = ZE_STAT_DISABLED;
        return ZE_OK;
    }

    /* Pin is assigned to peripheral: return ZE_STAT_DANGLING */
    if (g_ze_gpio.conf[pin_num].mode >= ZE_PERIPHERAL_RSVD_0) {
        *p_level = ZE_STAT_DANGLING;
        return ZE_OK;
    }

    /* Active GPIO pin: Read hardware input register */
    uint32_t val = 0U;
    if (pin_num < ZE_GPIO_LOW_PIN_COUNT) {
        val = (REG_READ(GPIO_IN_REG) >> pin_num) & 1U;
    } else {
        val = (REG_READ(GPIO_IN1_REG) >> (pin_num - ZE_GPIO_LOW_PIN_COUNT)) & 1U;
    }

    *p_level = (0U != val) ? ZE_STAT_HIGH : ZE_STAT_LOW;
    g_ze_gpio.conf[pin_num].level = *p_level;

    return ZE_OK;
}

/** @brief   Fast atomic write of logic level to a single GPIO pin register
 *  @param   pin_num Target GPIO pin index (0..48)
 *  @param   level   Desired logic level (ZE_STAT_LOW or ZE_STAT_HIGH)
 *  @return  0 on success, or ZE_GPIO_QUICK_GET_ERR on invalid pin/argument */
uint32_t ZE_GPIO_LevelQuickSet(IN uint32_t pin_num, IN ze_io_level_t level) {
    if (!prv_is_valid_pin(pin_num)) {
        return ZE_GPIO_QUICK_GET_ERR;
    }

    if (ZE_STAT_HIGH == level) {
        if (pin_num < ZE_GPIO_LOW_PIN_COUNT) {
            GPIO.out_w1ts = (1U << pin_num);
        } else {
            GPIO.out1_w1ts.val = (1U << (pin_num - ZE_GPIO_LOW_PIN_COUNT));
        }
        g_ze_gpio.conf[pin_num].level = ZE_STAT_HIGH;
        return 0U;
    } else if (ZE_STAT_LOW == level) {
        if (pin_num < ZE_GPIO_LOW_PIN_COUNT) {
            GPIO.out_w1tc = (1U << pin_num);
        } else {
            GPIO.out1_w1tc.val = (1U << (pin_num - ZE_GPIO_LOW_PIN_COUNT));
        }
        g_ze_gpio.conf[pin_num].level = ZE_STAT_LOW;
        return 0U;
    }

    return ZE_GPIO_QUICK_GET_ERR;
}

/** @brief   Fast direct read of logic level from single GPIO pin hardware register
 *  @param   pin_num Target GPIO pin index (0..48)
 *  @return  0 or 1 on success, or ZE_GPIO_QUICK_GET_ERR on invalid pin */
uint32_t ZE_GPIO_LevelQuickGet(IN uint32_t pin_num) {
    if (!prv_is_valid_pin(pin_num)) {
        return ZE_GPIO_QUICK_GET_ERR;
    }

    if (pin_num < ZE_GPIO_LOW_PIN_COUNT) {
        return (REG_READ(GPIO_IN_REG) >> pin_num) & 1U;
    }
    return (REG_READ(GPIO_IN1_REG) >> (pin_num - ZE_GPIO_LOW_PIN_COUNT)) & 1U;
}

/** @brief   Atomically set multiple GPIO pins to HIGH level simultaneously according to 64-bit mask
 *  @param   mask 64-bit mask of target pins to set HIGH
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PortSet(IN ze_gpio_dword_t mask) {
    uint32_t set_l = mask.word_l & g_ze_gpio.valid.word_l;
    uint32_t set_h = mask.word_h & g_ze_gpio.valid.word_h;

    if (0U != set_l) {
        GPIO.out_w1ts = set_l;
        for (uint32_t pin = 0U; pin < ZE_GPIO_LOW_PIN_COUNT; ++pin) {
            if (0U != (set_l & (1U << pin))) {
                g_ze_gpio.conf[pin].level = ZE_STAT_HIGH;
            }
        }
    }

    if (0U != set_h) {
        GPIO.out1_w1ts.val = set_h;
        for (uint32_t pin = 0U; pin < (ZE_GPIO_NUM_MAX - ZE_GPIO_LOW_PIN_COUNT); ++pin) {
            if (0U != (set_h & (1U << pin))) {
                g_ze_gpio.conf[pin + ZE_GPIO_LOW_PIN_COUNT].level = ZE_STAT_HIGH;
            }
        }
    }

    return ZE_OK;
}

/** @brief   Atomically clear multiple GPIO pins to LOW level simultaneously according to 64-bit mask
 *  @param   mask 64-bit mask of target pins to clear LOW
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PortClear(IN ze_gpio_dword_t mask) {
    uint32_t clr_l = mask.word_l & g_ze_gpio.valid.word_l;
    uint32_t clr_h = mask.word_h & g_ze_gpio.valid.word_h;

    if (0U != clr_l) {
        GPIO.out_w1tc = clr_l;
        for (uint32_t pin = 0U; pin < ZE_GPIO_LOW_PIN_COUNT; ++pin) {
            if (0U != (clr_l & (1U << pin))) {
                g_ze_gpio.conf[pin].level = ZE_STAT_LOW;
            }
        }
    }

    if (0U != clr_h) {
        GPIO.out1_w1tc.val = clr_h;
        for (uint32_t pin = 0U; pin < (ZE_GPIO_NUM_MAX - ZE_GPIO_LOW_PIN_COUNT); ++pin) {
            if (0U != (clr_h & (1U << pin))) {
                g_ze_gpio.conf[pin + ZE_GPIO_LOW_PIN_COUNT].level = ZE_STAT_LOW;
            }
        }
    }

    return ZE_OK;
}

/** @brief   Read physical logic levels of all GPIO pins simultaneously from hardware input registers
 *  @param   p_val Pointer to receive 64-bit pin level mask
 *  @return  ZE_OK on success, or ZE_ERR_INVALID_ARG */
ze_err_t ZE_GPIO_PortGet(OUT ze_gpio_dword_t rw_ptr p_val) {
    if (IS_NULL(p_val)) {
        return ZE_ERR_INVALID_ARG;
    }

    p_val->word_l = REG_READ(GPIO_IN_REG) & g_ze_gpio.valid.word_l;
    p_val->word_h = REG_READ(GPIO_IN1_REG) & g_ze_gpio.valid.word_h;

    return ZE_OK;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/