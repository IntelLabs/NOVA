/*
 * Event Counters
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2021 Udo Steinberg, BedRock Systems, Inc.
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

#include "atomic.hpp"
#include "compiler.hpp"
#include "config.hpp"
#include "memory.hpp"
#include "types.hpp"

class Counter
{
    private:
        Atomic<unsigned> val { 0 };

    public:
        static Counter req[NUM_IPI] CPULOCAL;
        static Counter loc[NUM_LVT] CPULOCAL;
        static Counter gsi[NUM_GSI] CPULOCAL;
        static Counter exc[NUM_EXC] CPULOCAL;
        static Counter vmi[NUM_VMI] CPULOCAL;
        static Counter schedule     CPULOCAL;
        static Counter helping      CPULOCAL;

        ALWAYS_INLINE
        inline void inc()
        {
            val = val + 1;
        }

        ALWAYS_INLINE
        inline unsigned get (unsigned cpu) const
        {
            return *reinterpret_cast<decltype (val) *>(reinterpret_cast<uintptr_t>(&val) - CPU_LOCAL_DATA + HV_GLOBAL_CPUS + cpu * PAGE_SIZE);
        }
};
