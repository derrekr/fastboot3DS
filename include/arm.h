#pragma once

/*
 *   This file is part of fastboot 3DS
 *   Copyright (C) 2017 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


// Program status register (CPSR/SPSR)
#define PSR_USER_MODE   (16)
#define PSR_FIQ_MODE    (17)
#define PSR_IRQ_MODE    (18)
#define PSR_SVC_MODE    (19)
#define PSR_ABORT_MODE  (23)
#define PSR_UNDEF_MODE  (27)
#define PSR_SYS_MODE    (31)
#define PSR_MODE_MASK   (PSR_SYS_MODE)

#define PSR_T           (1<<5)          // Thumb mode
#define PSR_F           (1<<6)          // Interrupts (FIQ) disable flag
#define PSR_I           (1<<7)          // Interrupts (IRQ) disable flag
#define PSR_A           (1<<8)          // Imprecise aborts disable flag
#define PSR_E           (1<<9)          // Big endian
#define PSR_J           (1<<24)         // Jazelle mode
#define PSR_Q           (1<<27)
#define PSR_V           (1<<28)         // Overflow flag
#define PSR_C           (1<<29)         // Carry flag
#define PSR_Z           (1<<30)         // Zero flag
#define PSR_N           (1<<31)         // Negative flag
#define PSR_INT_OFF     (PSR_I | PSR_F) // IRQ and FIQ disabled flags
