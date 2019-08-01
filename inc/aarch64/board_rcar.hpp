/*
 * Board-Specific Memory Layout: Renesas R-Car
 *
 * Copyright (C) 2019 Udo Steinberg, BedRock Systems, Inc.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#pragma once

#define LOAD_ADDR       0x48000000

#define CL0_CORES       2
#define CL1_CORES       4
#define SPIN_ADDR       0x0
#define SPIN_INCR       0x0

#define HTIMER_PPI      10
#define HTIMER_FLG      0x8

#define VTIMER_PPI      11
#define VTIMER_FLG      0x8

#define SMMU_SPI        0xffffffff
#define SMMU_FLG        0x0

#define SMMU_BASE       0x0
#define SMMU_SIZE       0x0

#define GICD_BASE       0xf1010000
#define GICD_SIZE       0x1000

#define GICR_BASE       0x0
#define GICR_SIZE       0x0

#define GICC_BASE       0xf1020000
#define GICC_SIZE       0x20000

#define GICH_BASE       0xf1040000
#define GICH_SIZE       0x20000

#define UART_BASE_CD    0x0
#define UART_BASE_IM    0x0
#define UART_BASE_MI    0x0
#define UART_BASE_PL    0x0
#define UART_BASE_SC    0xe6e88000
