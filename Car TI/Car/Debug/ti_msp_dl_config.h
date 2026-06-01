/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMA0
#define PWM_0_INST_IRQHandler                                   TIMA0_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMA0_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                             32000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOA
#define GPIO_PWM_0_C0_PIN                                          DL_GPIO_PIN_8
#define GPIO_PWM_0_C0_IOMUX                                      (IOMUX_PINCM19)
#define GPIO_PWM_0_C0_IOMUX_FUNC                     IOMUX_PINCM19_PF_TIMA0_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOA
#define GPIO_PWM_0_C1_PIN                                          DL_GPIO_PIN_9
#define GPIO_PWM_0_C1_IOMUX                                      (IOMUX_PINCM20)
#define GPIO_PWM_0_C1_IOMUX_FUNC                     IOMUX_PINCM20_PF_TIMA0_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX



/* Defines for UART_2 */
#define UART_2_INST                                                        UART2
#define UART_2_INST_FREQUENCY                                           32000000
#define UART_2_INST_IRQHandler                                  UART2_IRQHandler
#define UART_2_INST_INT_IRQN                                      UART2_INT_IRQn
#define GPIO_UART_2_RX_PORT                                                GPIOA
#define GPIO_UART_2_TX_PORT                                                GPIOA
#define GPIO_UART_2_RX_PIN                                        DL_GPIO_PIN_22
#define GPIO_UART_2_TX_PIN                                        DL_GPIO_PIN_23
#define GPIO_UART_2_IOMUX_RX                                     (IOMUX_PINCM47)
#define GPIO_UART_2_IOMUX_TX                                     (IOMUX_PINCM53)
#define GPIO_UART_2_IOMUX_RX_FUNC                      IOMUX_PINCM47_PF_UART2_RX
#define GPIO_UART_2_IOMUX_TX_FUNC                      IOMUX_PINCM53_PF_UART2_TX
#define UART_2_BAUD_RATE                                                (115200)
#define UART_2_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_2_FBRD_32_MHZ_115200_BAUD                                      (23)





/* Port definition for Pin Group GPIO_GRP_0 */
#define GPIO_GRP_0_PORT                                                  (GPIOB)

/* Defines for PIN_2: GPIOB.2 with pinCMx 15 on package pin 50 */
#define GPIO_GRP_0_PIN_2_PIN                                     (DL_GPIO_PIN_2)
#define GPIO_GRP_0_PIN_2_IOMUX                                   (IOMUX_PINCM15)
/* Port definition for Pin Group GPIO_GRP_1 */
#define GPIO_GRP_1_PORT                                                  (GPIOB)

/* Defines for PIN_3: GPIOB.3 with pinCMx 16 on package pin 51 */
#define GPIO_GRP_1_PIN_3_PIN                                     (DL_GPIO_PIN_3)
#define GPIO_GRP_1_PIN_3_IOMUX                                   (IOMUX_PINCM16)
/* Port definition for Pin Group GPIO_GRP_2 */
#define GPIO_GRP_2_PORT                                                  (GPIOB)

/* Defines for PIN_6: GPIOB.6 with pinCMx 23 on package pin 58 */
#define GPIO_GRP_2_PIN_6_PIN                                     (DL_GPIO_PIN_6)
#define GPIO_GRP_2_PIN_6_IOMUX                                   (IOMUX_PINCM23)
/* Port definition for Pin Group GPIO_GRP_3 */
#define GPIO_GRP_3_PORT                                                  (GPIOB)

/* Defines for PIN_7: GPIOB.7 with pinCMx 24 on package pin 59 */
#define GPIO_GRP_3_PIN_7_PIN                                     (DL_GPIO_PIN_7)
#define GPIO_GRP_3_PIN_7_IOMUX                                   (IOMUX_PINCM24)
/* Port definition for Pin Group ENCODER_LEFTA */
#define ENCODER_LEFTA_PORT                                               (GPIOA)

/* Defines for PIN_0: GPIOA.0 with pinCMx 1 on package pin 33 */
// groups represented: ["ENCODER_RIGHTA","ENCODER_LEFTA"]
// pins affected: ["PIN_26","PIN_0"]
#define GPIO_MULTIPLE_GPIOA_INT_IRQN                            (GPIOA_INT_IRQn)
#define GPIO_MULTIPLE_GPIOA_INT_IIDX            (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define ENCODER_LEFTA_PIN_0_IIDX                             (DL_GPIO_IIDX_DIO0)
#define ENCODER_LEFTA_PIN_0_PIN                                  (DL_GPIO_PIN_0)
#define ENCODER_LEFTA_PIN_0_IOMUX                                 (IOMUX_PINCM1)
/* Port definition for Pin Group ENCODER_LEFTB */
#define ENCODER_LEFTB_PORT                                               (GPIOA)

/* Defines for PIN_1: GPIOA.1 with pinCMx 2 on package pin 34 */
#define ENCODER_LEFTB_PIN_1_PIN                                  (DL_GPIO_PIN_1)
#define ENCODER_LEFTB_PIN_1_IOMUX                                 (IOMUX_PINCM2)
/* Port definition for Pin Group ENCODER_RIGHTA */
#define ENCODER_RIGHTA_PORT                                              (GPIOA)

/* Defines for PIN_26: GPIOA.26 with pinCMx 59 on package pin 30 */
#define ENCODER_RIGHTA_PIN_26_IIDX                          (DL_GPIO_IIDX_DIO26)
#define ENCODER_RIGHTA_PIN_26_PIN                               (DL_GPIO_PIN_26)
#define ENCODER_RIGHTA_PIN_26_IOMUX                              (IOMUX_PINCM59)
/* Port definition for Pin Group ENCODER_RIGHTB */
#define ENCODER_RIGHTB_PORT                                              (GPIOA)

/* Defines for PIN_27: GPIOA.27 with pinCMx 60 on package pin 31 */
#define ENCODER_RIGHTB_PIN_27_PIN                               (DL_GPIO_PIN_27)
#define ENCODER_RIGHTB_PIN_27_IOMUX                              (IOMUX_PINCM60)
/* Port definition for Pin Group GRAY_L3 */
#define GRAY_L3_PORT                                                     (GPIOB)

/* Defines for PIN_24: GPIOB.24 with pinCMx 52 on package pin 23 */
#define GRAY_L3_PIN_24_PIN                                      (DL_GPIO_PIN_24)
#define GRAY_L3_PIN_24_IOMUX                                     (IOMUX_PINCM52)
/* Port definition for Pin Group GRAY_L2 */
#define GRAY_L2_PORT                                                     (GPIOB)

/* Defines for PIN_20: GPIOB.20 with pinCMx 48 on package pin 19 */
#define GRAY_L2_PIN_20_PIN                                      (DL_GPIO_PIN_20)
#define GRAY_L2_PIN_20_IOMUX                                     (IOMUX_PINCM48)
/* Port definition for Pin Group GRAY_L1 */
#define GRAY_L1_PORT                                                     (GPIOB)

/* Defines for PIN_19: GPIOB.19 with pinCMx 45 on package pin 16 */
#define GRAY_L1_PIN_19_PIN                                      (DL_GPIO_PIN_19)
#define GRAY_L1_PIN_19_IOMUX                                     (IOMUX_PINCM45)
/* Port definition for Pin Group GRAY_M */
#define GRAY_M_PORT                                                      (GPIOB)

/* Defines for PIN_18: GPIOB.18 with pinCMx 44 on package pin 15 */
#define GRAY_M_PIN_18_PIN                                       (DL_GPIO_PIN_18)
#define GRAY_M_PIN_18_IOMUX                                      (IOMUX_PINCM44)
/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOA)

/* Defines for PIN_12: GPIOA.12 with pinCMx 34 on package pin 5 */
#define LED_PIN_12_PIN                                          (DL_GPIO_PIN_12)
#define LED_PIN_12_IOMUX                                         (IOMUX_PINCM34)
/* Port definition for Pin Group BUZZER */
#define BUZZER_PORT                                                      (GPIOA)

/* Defines for PIN_13: GPIOA.13 with pinCMx 35 on package pin 6 */
#define BUZZER_PIN_13_PIN                                       (DL_GPIO_PIN_13)
#define BUZZER_PIN_13_IOMUX                                      (IOMUX_PINCM35)
/* Port definition for Pin Group GRAY_R1 */
#define GRAY_R1_PORT                                                     (GPIOA)

/* Defines for PINA_18: GPIOA.18 with pinCMx 40 on package pin 11 */
#define GRAY_R1_PINA_18_PIN                                     (DL_GPIO_PIN_18)
#define GRAY_R1_PINA_18_IOMUX                                    (IOMUX_PINCM40)
/* Port definition for Pin Group GRAY_R2 */
#define GRAY_R2_PORT                                                     (GPIOA)

/* Defines for PIN_17: GPIOA.17 with pinCMx 39 on package pin 10 */
#define GRAY_R2_PIN_17_PIN                                      (DL_GPIO_PIN_17)
#define GRAY_R2_PIN_17_IOMUX                                     (IOMUX_PINCM39)
/* Port definition for Pin Group GRAY_R3 */
#define GRAY_R3_PORT                                                     (GPIOA)

/* Defines for PIN_16: GPIOA.16 with pinCMx 38 on package pin 9 */
#define GRAY_R3_PIN_16_PIN                                      (DL_GPIO_PIN_16)
#define GRAY_R3_PIN_16_IOMUX                                     (IOMUX_PINCM38)
/* Port definition for Pin Group KEY1 */
#define KEY1_PORT                                                        (GPIOA)

/* Defines for PIN_21: GPIOA.21 with pinCMx 46 on package pin 17 */
#define KEY1_PIN_21_PIN                                         (DL_GPIO_PIN_21)
#define KEY1_PIN_21_IOMUX                                        (IOMUX_PINCM46)
/* Port definition for Pin Group KEY2 */
#define KEY2_PORT                                                        (GPIOA)

/* Defines for PINA_24: GPIOA.24 with pinCMx 54 on package pin 25 */
#define KEY2_PINA_24_PIN                                        (DL_GPIO_PIN_24)
#define KEY2_PINA_24_IOMUX                                       (IOMUX_PINCM54)
/* Port definition for Pin Group KEY3 */
#define KEY3_PORT                                                        (GPIOA)

/* Defines for PIN_02: GPIOA.2 with pinCMx 7 on package pin 42 */
#define KEY3_PIN_02_PIN                                          (DL_GPIO_PIN_2)
#define KEY3_PIN_02_IOMUX                                         (IOMUX_PINCM7)
/* Port definition for Pin Group KEY4 */
#define KEY4_PORT                                                        (GPIOA)

/* Defines for PIN_28: GPIOA.28 with pinCMx 3 on package pin 35 */
#define KEY4_PIN_28_PIN                                         (DL_GPIO_PIN_28)
#define KEY4_PIN_28_IOMUX                                         (IOMUX_PINCM3)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_UART_2_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
