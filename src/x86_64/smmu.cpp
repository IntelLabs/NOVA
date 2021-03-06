/*
 * System Memory Management Unit (Intel IOMMU)
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

#include "bits.hpp"
#include "extern.hpp"
#include "lapic.hpp"
#include "pd.hpp"
#include "smmu.hpp"
#include "stdio.hpp"
#include "vectors.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache  Smmu::cache (sizeof (Smmu), 8);

Smmu *      Smmu::list;
Smmu_ctx *  Smmu::ctx = new Smmu_ctx;
Smmu_irt *  Smmu::irt = new Smmu_irt;
uint32      Smmu::gcmd = GCMD_TE;

Smmu::Smmu (Paddr p) : List<Smmu> (list), reg_base ((hwdev_addr -= PAGE_SIZE) | (p & PAGE_MASK)), invq (static_cast<Smmu_qi *>(Buddy::alloc (ord, Buddy::Fill::BITS0))), invq_idx (0)
{
    Pd::kern.Space_mem::delreg (p & ~PAGE_MASK);
    Pd::kern.Space_mem::insert (reg_base, 0, Hpt::HPT_NX | Hpt::HPT_G | Hpt::HPT_UC | Hpt::HPT_W | Hpt::HPT_P, p & ~PAGE_MASK);

    cap  = read<uint64>(REG_CAP);
    ecap = read<uint64>(REG_ECAP);

    Dpt::ord = min (Dpt::ord, static_cast<mword>(bit_scan_reverse (static_cast<mword>(cap >> 34) & 0xf) + 2) * Dpt::bpl() - 1);

    write<uint32>(REG_FEADDR, 0xfee00000 | Cpu::apic_id[0] << 12);
    write<uint32>(REG_FEDATA, VEC_MSI_SMMU);
    write<uint32>(REG_FECTL,  0);

    write<uint64>(REG_RTADDR, Kmem::ptr_to_phys (ctx));
    command (GCMD_SRTP);

    if (ir()) {
        write<uint64>(REG_IRTA, Kmem::ptr_to_phys (irt) | 7);
        command (GCMD_SIRTP);
        gcmd |= GCMD_IRE;
    }

    if (qi()) {
        write<uint64>(REG_IQT, 0);
        write<uint64>(REG_IQA, Kmem::ptr_to_phys (invq));
        command (GCMD_QIE);
        gcmd |= GCMD_QIE;
    }
}

void Smmu::assign (unsigned long rid, Pd *p)
{
    mword lev = bit_scan_reverse (read<mword>(REG_CAP) >> 8 & 0x1f);

    Smmu_ctx *r = ctx + (rid >> 8);
    if (!r->present())
        r->set (0, Kmem::ptr_to_phys (new Smmu_ctx) | 1);

    Smmu_ctx *c = static_cast<Smmu_ctx *>(Kmem::phys_to_ptr (r->addr())) + (rid & 0xff);
    if (c->present())
        c->set (0, 0);

    flush_ctx();

    c->set (lev | p->did << 8, p->dpt.root (lev + 1) | 1);
}

void Smmu::fault_handler()
{
    for (uint32 fsts; fsts = read<uint32>(REG_FSTS), fsts & 0xff;) {

        if (fsts & 0x2) {
            uint64 hi, lo;
            for (unsigned frr = fsts >> 8 & 0xff; read (frr, hi, lo), hi & 1ull << 63; frr = (frr + 1) % nfr())
                trace (TRACE_SMMU, "SMMU:%p FRR:%u FR:%#x BDF:%x:%x:%x FI:%#010llx",
                       this,
                       frr,
                       static_cast<uint32>(hi >> 32) & 0xff,
                       static_cast<uint32>(hi >> 8) & 0xff,
                       static_cast<uint32>(hi >> 3) & 0x1f,
                       static_cast<uint32>(hi) & 0x7,
                       lo);
        }

        write<uint32>(REG_FSTS, 0x7d);
    }
}

void Smmu::vector (unsigned vector)
{
    unsigned msi = vector - VEC_MSI;

    if (EXPECT_TRUE (msi == 0))
        for (Smmu *smmu = list; smmu; smmu = smmu->next)
            smmu->fault_handler();

    Lapic::eoi();
}
