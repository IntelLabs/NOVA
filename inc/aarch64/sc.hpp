/*
 * Scheduling Context (SC)
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
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
#include "kobject.hpp"
#include "memory.hpp"
#include "queue.hpp"
#include "slab.hpp"

class Ec;

class Sc : private Kobject, public Queue<Sc>::Element
{
    friend class Ec;

    private:
        unsigned const          cpu                 { 0 };
        Ec *     const          ec                  { nullptr };
        unsigned const          prio                { 0 };
        uint64   const          budget              { 0 };
        Atomic<uint64>          used                { 0 };
        uint64                  left                { 0 };
        uint64                  last                { 0 };

        static constexpr unsigned priorities = 128;

        static unsigned         prio_top            CPULOCAL;
        static Queue<Sc>        list[priorities]    CPULOCAL;
        static Slab_cache       cache;

        static struct Rq {
            Spinlock            lock;
            Queue<Sc>           queue;
        } rq CPULOCAL;

        ALWAYS_INLINE
        static inline auto remote_queue (unsigned cpu)
        {
            return reinterpret_cast<decltype (rq) *>(reinterpret_cast<uintptr_t>(&rq) - CPU_LOCAL_DATA + CPU_GLOBL_DATA + cpu * PAGE_SIZE);
        }

        void ready_enqueue (uint64);
        void ready_dequeue (uint64);

        Sc (unsigned, Ec *, unsigned, uint32);

        NODISCARD
        static inline void *operator new (size_t) noexcept
        {
            return cache.alloc();
        }

        static inline void operator delete (void *ptr)
        {
            if (EXPECT_TRUE (ptr))
                cache.free (ptr);
        }

    public:
        static Sc *             current             CPULOCAL;
        static unsigned         ctr_link            CPULOCAL;
        static unsigned         ctr_loop            CPULOCAL;

        NODISCARD
        static Sc *create (unsigned c, Ec *e, unsigned p, uint32 b)
        {
            return new Sc (c, e, p, b);
        }

        void destroy() { delete this; }

        ALWAYS_INLINE
        inline uint64 get_used() const { return used; }

        ALWAYS_INLINE
        static inline auto max_prio() { return priorities - 1; }

        void remote_enqueue();

        static void rrq_handler();

        NORETURN
        static void schedule (bool = false);
};
