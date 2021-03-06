/*
 * Register File
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2019-2020 Udo Steinberg, BedRock Systems, Inc.
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

#include "arch.hpp"
#include "hazards.hpp"
#include "svm.hpp"
#include "types.hpp"
#include "vmx.hpp"

class Sys_regs
{
    public:
        struct {
            uintptr_t   r15 { 0 };
            uintptr_t   r14 { 0 };
            uintptr_t   r13 { 0 };
            uintptr_t   r12 { 0 };
            uintptr_t   r11 { 0 };
            uintptr_t   r10 { 0 };
            uintptr_t   r9  { 0 };
            uintptr_t   r8  { 0 };
            uintptr_t   rdi { 0 };
            uintptr_t   rsi { 0 };
            uintptr_t   rbp { 0 };
            uintptr_t   cr2 { 0 };
            uintptr_t   rbx { 0 };
            uintptr_t   rdx { 0 };
            uintptr_t   rcx { 0 };
            uintptr_t   rax { 0 };
        };

        enum Status
        {
            SUCCESS,
            COM_TIM,
            COM_ABT,
            BAD_HYP,
            BAD_CAP,
            BAD_PAR,
            BAD_FTR,
            BAD_CPU,
            BAD_DEV,
        };

        ALWAYS_INLINE
        inline unsigned flags() const { return ARG_1 >> 4 & 0xf; }

        ALWAYS_INLINE
        inline void set_status (Status status) { ARG_1 = status; }

        ALWAYS_INLINE
        inline void set_pt (mword pt) { ARG_1 = pt; }

        ALWAYS_INLINE
        inline void set_ip (mword ip) { ARG_IP = ip; }

        ALWAYS_INLINE
        inline void set_sp (mword sp) { ARG_SP = sp; }
};

class Exc_regs : public Sys_regs
{
    public:
        union {
            struct {
                uintptr_t   gs;
                uintptr_t   fs;
                uintptr_t   es;
                uintptr_t   ds;
                uintptr_t   err;
                uintptr_t   vec;
                uintptr_t   rip;
                uintptr_t   cs;
                uintptr_t   rfl;
                uintptr_t   rsp;
                uintptr_t   ss;
            };
            struct {
                union {
                    Vmcs *  vmcs;
                    Vmcb *  vmcb;
                };
                uintptr_t   reserved1;
                uintptr_t   reserved2;
                uintptr_t   cr0_shadow;
                uintptr_t   cr4_shadow;
                uintptr_t   dst_portal;
                uint8       fpu_on;
            };
        };

    private:
        template <typename T> ALWAYS_INLINE inline mword get_g_cs_dl() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_flags() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr0() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr2() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr3() const;
        template <typename T> ALWAYS_INLINE inline mword get_g_cr4() const;
        template <typename T> ALWAYS_INLINE inline void  set_g_cr0 (mword) const;
        template <typename T> ALWAYS_INLINE inline void  set_g_cr2 (mword);
        template <typename T> ALWAYS_INLINE inline void  set_g_cr3 (mword) const;
        template <typename T> ALWAYS_INLINE inline void  set_g_cr4 (mword) const;
        template <typename T> ALWAYS_INLINE inline void  set_s_cr0 (mword);
        template <typename T> ALWAYS_INLINE inline void  set_s_cr4 (mword);
        template <typename T> ALWAYS_INLINE inline void  set_e_bmp (uint32) const;

    protected:
        template <typename T> ALWAYS_INLINE inline uint64 get_g_efer() const;

        template <typename T>
        mword cr0_set() const
        {
            mword set = 0;

            if (!fpu_on)
                set |= CR0_TS;

            return T::fix_cr0_set | set;
        }

        template <typename T>
        mword cr0_msk() const
        {
            return T::fix_cr0_clr | cr0_set<T>();
        }

        template <typename T>
        mword cr4_set() const
        {
            mword set = 0;

            return T::fix_cr4_set | set;
        }

        template <typename T>
        mword cr4_msk() const
        {
            return T::fix_cr4_clr | cr4_set<T>();
        }

    public:
        ALWAYS_INLINE
        inline bool user() const { return cs & 3; }

        void fpu_ctrl (bool);

        template <typename T> void set_cpu_ctrl0 (uint32);
        template <typename T> void set_cpu_ctrl1 (uint32);

        template <typename T> void nst_ctrl();

        template <typename T> void tlb_flush (bool) const;
        template <typename T> void tlb_flush (mword) const;

        template <typename T>
        mword get_cr0() const
        {
            mword msk = cr0_msk<T>();
            return (get_g_cr0<T>() & ~msk) | (cr0_shadow & msk);
        }

        template <typename T>
        mword get_cr2() const
        {
            return get_g_cr2<T>();
        }

        template <typename T>
        mword get_cr3() const
        {
            return get_g_cr3<T>();
        }

        template <typename T>
        mword get_cr4() const
        {
            mword msk = cr4_msk<T>();
            return (get_g_cr4<T>() & ~msk) | (cr4_shadow & msk);
        }

        template <typename T>
        void set_cr0 (mword v)
        {
            set_g_cr0<T> ((v & (~cr0_msk<T>() | CR0_PE)) | (cr0_set<T>() & ~CR0_PE));
            set_s_cr0<T> (v);
        }

        template <typename T>
        void set_cr2 (mword v)
        {
            set_g_cr2<T> (v);
        }

        template <typename T>
        void set_cr3 (mword v)
        {
            set_g_cr3<T> (v);
        }

        template <typename T>
        void set_cr4 (mword v)
        {
            set_g_cr4<T> ((v & ~cr4_msk<T>()) | cr4_set<T>());
            set_s_cr4<T> (v);
        }

        template <typename T>
        void set_exc()
        {
            uint32 val = BIT (EXC_AC);

            if (!fpu_on)
                val |= BIT (EXC_NM);

            set_e_bmp<T> (val);
        }

        template <typename T> void write_efer (uint64);
};

class Cpu_regs : public Exc_regs
{
    private:
        mword   hzd;

    public:
        uint64  tsc_offset;
        mword   mtd;

        ALWAYS_INLINE
        inline mword hazard() const { return hzd; }

        ALWAYS_INLINE
        inline void set_hazard (mword h) { __atomic_or_fetch (&hzd, h, __ATOMIC_SEQ_CST); }

        ALWAYS_INLINE
        inline void clr_hazard (mword h) { __atomic_and_fetch (&hzd, ~h, __ATOMIC_SEQ_CST); }

        ALWAYS_INLINE
        inline void add_tsc_offset (uint64 tsc)
        {
            tsc_offset += tsc;
            set_hazard (HZD_TSC);
        }
};

template <> inline mword  Exc_regs::get_g_cs_dl<Vmcb>()        const { return static_cast<mword>(vmcb->cs.ar) >> 9 & 0x3; }
template <> inline mword  Exc_regs::get_g_cs_dl<Vmcs>()        const { return Vmcs::read<uint32> (Vmcs::GUEST_AR_CS) >> 13 & 0x3; }
template <> inline mword  Exc_regs::get_g_flags<Vmcb>()        const { return static_cast<mword>(vmcb->rflags); }
template <> inline mword  Exc_regs::get_g_flags<Vmcs>()        const { return Vmcs::read<mword>  (Vmcs::GUEST_RFLAGS); }
template <> inline uint64 Exc_regs::get_g_efer<Vmcb>()         const { return vmcb->efer; }
template <> inline uint64 Exc_regs::get_g_efer<Vmcs>()         const { return Vmcs::read<uint64> (Vmcs::GUEST_EFER); }

template <> inline mword  Exc_regs::get_g_cr0<Vmcb>()          const { return static_cast<mword>(vmcb->cr0); }
template <> inline mword  Exc_regs::get_g_cr2<Vmcb>()          const { return static_cast<mword>(vmcb->cr2); }
template <> inline mword  Exc_regs::get_g_cr3<Vmcb>()          const { return static_cast<mword>(vmcb->cr3); }
template <> inline mword  Exc_regs::get_g_cr4<Vmcb>()          const { return static_cast<mword>(vmcb->cr4); }
template <> inline mword  Exc_regs::get_g_cr0<Vmcs>()          const { return Vmcs::read<mword>  (Vmcs::GUEST_CR0); }
template <> inline mword  Exc_regs::get_g_cr2<Vmcs>()          const { return cr2; }
template <> inline mword  Exc_regs::get_g_cr3<Vmcs>()          const { return Vmcs::read<mword>  (Vmcs::GUEST_CR3); }
template <> inline mword  Exc_regs::get_g_cr4<Vmcs>()          const { return Vmcs::read<mword>  (Vmcs::GUEST_CR4); }

template <> inline void   Exc_regs::set_g_cr0<Vmcb> (mword v)  const { vmcb->cr0 = v; }
template <> inline void   Exc_regs::set_g_cr2<Vmcb> (mword v)        { vmcb->cr2 = v; }
template <> inline void   Exc_regs::set_g_cr3<Vmcb> (mword v)  const { vmcb->cr3 = v; }
template <> inline void   Exc_regs::set_g_cr4<Vmcb> (mword v)  const { vmcb->cr4 = v; }
template <> inline void   Exc_regs::set_g_cr0<Vmcs> (mword v)  const { Vmcs::write (Vmcs::GUEST_CR0, v); }
template <> inline void   Exc_regs::set_g_cr2<Vmcs> (mword v)        { cr2 = v; }
template <> inline void   Exc_regs::set_g_cr3<Vmcs> (mword v)  const { Vmcs::write (Vmcs::GUEST_CR3, v); }
template <> inline void   Exc_regs::set_g_cr4<Vmcs> (mword v)  const { Vmcs::write (Vmcs::GUEST_CR4, v); }

template <> inline void   Exc_regs::set_s_cr0<Vmcb> (mword v)        { cr0_shadow = v; }
template <> inline void   Exc_regs::set_s_cr4<Vmcb> (mword v)        { cr4_shadow = v; }
template <> inline void   Exc_regs::set_s_cr0<Vmcs> (mword v)        { Vmcs::write (Vmcs::CR0_READ_SHADOW, cr0_shadow = v); }
template <> inline void   Exc_regs::set_s_cr4<Vmcs> (mword v)        { Vmcs::write (Vmcs::CR4_READ_SHADOW, cr4_shadow = v); }

template <> inline void   Exc_regs::set_e_bmp<Vmcb> (uint32 v) const { vmcb->intercept_exc = v; }
template <> inline void   Exc_regs::set_e_bmp<Vmcs> (uint32 v) const { Vmcs::write (Vmcs::EXC_BITMAP, v); }
