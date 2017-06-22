/* ----------------------------------------------------------------------------
 *         SAM Software Package License
 * ----------------------------------------------------------------------------
 * Copyright (c) 2011-2012, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition is met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

#pragma once


/*
 * Starting address for the application section.
 * For example, with an 8KB bootloader, APP_START_ADDRESS = 0x00002000.
 * SAMD11 devices should probably use a 4KB bootloader.
 */
#define APP_START_ADDRESS                 0x00002000
//#define APP_START_ADDRESS                 0x00001000

/*
 * If BOOT_LOAD_PIN is defined the bootloader is started if the selected
 * pin is tied LOW.
 */
#define BOOT_LOAD_PIN                     PIN_PA27
//#define BOOT_LOAD_PIN                     PIN_PA15
#define BOOT_PIN_MASK                     (1U << (BOOT_LOAD_PIN & 0x1f))

/*
 * If LED_PIN is defined, it will turn on during bootloader operation.
 */
#define	LED_PIN                           PIN_PA28
//#define	LED_PIN                           PIN_PA16
#define LED_PIN_MASK                      (1U << (LED_PIN & 0x1f))

/*
 * If BOOT_DOUBLE_TAP is defined the bootloader is started by
 * quickly tapping two times on the reset button.
 * BOOT_DOUBLE_TAP_ADDRESS must point to a free SRAM cell that must not
 * be touched from the loaded application.
 */
#define BOOT_DOUBLE_TAP	1

#if defined(__SAMD21J18A__) || defined(__SAMD21G18A__) || defined(__SAMD21E18A__)
	#define BOOT_DOUBLE_TAP_ADDRESS           0x20007FFC
#elif defined(__SAMD21J17A__) || defined(__SAMD21G17A__) || defined(__SAMD21E17A__)
	#define BOOT_DOUBLE_TAP_ADDRESS           0x20003FFC
#elif defined(__SAMD21J16A__) || defined(__SAMD21G16A__) || defined(__SAMD21E16A__)
	#define BOOT_DOUBLE_TAP_ADDRESS           0x20001FFC
#elif defined(__SAMD21J15A__) || defined(__SAMD21G15A__) || defined(__SAMD21E15A__)
	#define BOOT_DOUBLE_TAP_ADDRESS           0x20000FFC
#elif defined(__SAMD11D14AM__) || defined(__SAMD11C14A__) || defined(__SAMD11D14AS__)
	#define BOOT_DOUBLE_TAP_ADDRESS           0x20000FFC
#else
	#error "main.h: Unsupported MCU"
#endif

#define BOOT_DOUBLE_TAP_DATA              (*((volatile uint32_t *) BOOT_DOUBLE_TAP_ADDRESS))

/*
 * If bootloader UART support is compiled in (see sam_ba_monitor.h), configure it here.
 */
#define BOOT_USART_MODULE                 SERCOM0
#define BOOT_USART_MUX_SETTINGS           UART_RX_PAD3_TX_PAD2
#define BOOT_USART_PAD3                   PINMUX_PA11C_SERCOM0_PAD3
#define BOOT_USART_PAD2                   PINMUX_PA10C_SERCOM0_PAD2
//#define BOOT_USART_PAD3                   PINMUX_UNUSED
//#define BOOT_USART_PAD2                   PINMUX_UNUSED
#define BOOT_USART_PAD1                   PINMUX_UNUSED
#define BOOT_USART_PAD0                   PINMUX_UNUSED


#define CPU_FREQUENCY                     8000000
#define FLASH_WAIT_STATES                 1

