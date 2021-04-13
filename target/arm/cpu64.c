/*
 * QEMU AArch64 CPU
 *
 * Copyright (c) 2013 Linaro Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/qemu-print.h"
#include "cpu.h"
#include "cpu32.h"
#include "cpu-sve.h"
#include "tcg/cpu-pauth.h"
#include "qemu/module.h"
#include "sysemu/tcg.h"
#include "sysemu/kvm.h"
#include "kvm/kvm_arm.h"
#include "qapi/visitor.h"
#include "hw/qdev-properties.h"
#include "cpregs.h"

#ifndef CONFIG_USER_ONLY
static uint64_t a57_a53_l2ctlr_read(CPUARMState *env, const ARMCPRegInfo *ri)
{
    ARMCPU *cpu = env_archcpu(env);

    /* Number of cores is in [25:24]; otherwise we RAZ */
    return (cpu->core_count - 1) << 24;
}
#endif

static const ARMCPRegInfo cortex_a72_a57_a53_cp_reginfo[] = {
#ifndef CONFIG_USER_ONLY
    { .name = "L2CTLR_EL1", .state = ARM_CP_STATE_AA64,
      .opc0 = 3, .opc1 = 1, .crn = 11, .crm = 0, .opc2 = 2,
      .access = PL1_RW, .readfn = a57_a53_l2ctlr_read,
      .writefn = arm_cp_write_ignore },
    { .name = "L2CTLR",
      .cp = 15, .opc1 = 1, .crn = 9, .crm = 0, .opc2 = 2,
      .access = PL1_RW, .readfn = a57_a53_l2ctlr_read,
      .writefn = arm_cp_write_ignore },
#endif
    { .name = "L2ECTLR_EL1", .state = ARM_CP_STATE_AA64,
      .opc0 = 3, .opc1 = 1, .crn = 11, .crm = 0, .opc2 = 3,
      .access = PL1_RW, .type = ARM_CP_CONST, .resetvalue = 0 },
    { .name = "L2ECTLR",
      .cp = 15, .opc1 = 1, .crn = 9, .crm = 0, .opc2 = 3,
      .access = PL1_RW, .type = ARM_CP_CONST, .resetvalue = 0 },
    { .name = "L2ACTLR", .state = ARM_CP_STATE_BOTH,
      .opc0 = 3, .opc1 = 1, .crn = 15, .crm = 0, .opc2 = 0,
      .access = PL1_RW, .type = ARM_CP_CONST, .resetvalue = 0 },
    { .name = "CPUACTLR_EL1", .state = ARM_CP_STATE_AA64,
      .opc0 = 3, .opc1 = 1, .crn = 15, .crm = 2, .opc2 = 0,
      .access = PL1_RW, .type = ARM_CP_CONST, .resetvalue = 0 },
    { .name = "CPUACTLR",
      .cp = 15, .opc1 = 0, .crm = 15,
      .access = PL1_RW, .type = ARM_CP_CONST | ARM_CP_64BIT, .resetvalue = 0 },
    { .name = "CPUECTLR_EL1", .state = ARM_CP_STATE_AA64,
      .opc0 = 3, .opc1 = 1, .crn = 15, .crm = 2, .opc2 = 1,
      .access = PL1_RW, .type = ARM_CP_CONST, .resetvalue = 0 },
    { .name = "CPUECTLR",
      .cp = 15, .opc1 = 1, .crm = 15,
      .access = PL1_RW, .type = ARM_CP_CONST | ARM_CP_64BIT, .resetvalue = 0 },
    { .name = "CPUMERRSR_EL1", .state = ARM_CP_STATE_AA64,
      .opc0 = 3, .opc1 = 1, .crn = 15, .crm = 2, .opc2 = 2,
      .access = PL1_RW, .type = ARM_CP_CONST, .resetvalue = 0 },
    { .name = "CPUMERRSR",
      .cp = 15, .opc1 = 2, .crm = 15,
      .access = PL1_RW, .type = ARM_CP_CONST | ARM_CP_64BIT, .resetvalue = 0 },
    { .name = "L2MERRSR_EL1", .state = ARM_CP_STATE_AA64,
      .opc0 = 3, .opc1 = 1, .crn = 15, .crm = 2, .opc2 = 3,
      .access = PL1_RW, .type = ARM_CP_CONST, .resetvalue = 0 },
    { .name = "L2MERRSR",
      .cp = 15, .opc1 = 3, .crm = 15,
      .access = PL1_RW, .type = ARM_CP_CONST | ARM_CP_64BIT, .resetvalue = 0 },
    REGINFO_SENTINEL
};

static void aarch64_a57_initfn(Object *obj)
{
    ARMCPU *cpu = ARM_CPU(obj);

    cpu->dtb_compatible = "arm,cortex-a57";
    set_feature(&cpu->env, ARM_FEATURE_V8);
    set_feature(&cpu->env, ARM_FEATURE_NEON);
    set_feature(&cpu->env, ARM_FEATURE_GENERIC_TIMER);
    set_feature(&cpu->env, ARM_FEATURE_AARCH64);
    set_feature(&cpu->env, ARM_FEATURE_CBAR_RO);
    set_feature(&cpu->env, ARM_FEATURE_EL2);
    set_feature(&cpu->env, ARM_FEATURE_EL3);
    set_feature(&cpu->env, ARM_FEATURE_PMU);
    cpu->kvm_target = QEMU_KVM_ARM_TARGET_CORTEX_A57;
    cpu->midr = 0x411fd070;
    cpu->revidr = 0x00000000;
    cpu->reset_fpsid = 0x41034070;
    cpu->isar.mvfr0 = 0x10110222;
    cpu->isar.mvfr1 = 0x12111111;
    cpu->isar.mvfr2 = 0x00000043;
    cpu->ctr = 0x8444c004;
    cpu->reset_sctlr = 0x00c50838;
    cpu->isar.id_pfr0 = 0x00000131;
    cpu->isar.id_pfr1 = 0x00011011;
    cpu->isar.id_dfr0 = 0x03010066;
    cpu->id_afr0 = 0x00000000;
    cpu->isar.id_mmfr0 = 0x10101105;
    cpu->isar.id_mmfr1 = 0x40000000;
    cpu->isar.id_mmfr2 = 0x01260000;
    cpu->isar.id_mmfr3 = 0x02102211;
    cpu->isar.id_isar0 = 0x02101110;
    cpu->isar.id_isar1 = 0x13112111;
    cpu->isar.id_isar2 = 0x21232042;
    cpu->isar.id_isar3 = 0x01112131;
    cpu->isar.id_isar4 = 0x00011142;
    cpu->isar.id_isar5 = 0x00011121;
    cpu->isar.id_isar6 = 0;
    cpu->isar.id_aa64pfr0 = 0x00002222;
    cpu->isar.id_aa64dfr0 = 0x10305106;
    cpu->isar.id_aa64isar0 = 0x00011120;
    cpu->isar.id_aa64mmfr0 = 0x00001124;
    cpu->isar.dbgdidr = 0x3516d000;
    cpu->clidr = 0x0a200023;
    cpu->ccsidr[0] = 0x701fe00a; /* 32KB L1 dcache */
    cpu->ccsidr[1] = 0x201fe012; /* 48KB L1 icache */
    cpu->ccsidr[2] = 0x70ffe07a; /* 2048KB L2 cache */
    cpu->dcz_blocksize = 4; /* 64 bytes */
    cpu->gic_num_lrs = 4;
    cpu->gic_vpribits = 5;
    cpu->gic_vprebits = 5;
    define_arm_cp_regs(cpu, cortex_a72_a57_a53_cp_reginfo);
}

static void aarch64_a53_initfn(Object *obj)
{
    ARMCPU *cpu = ARM_CPU(obj);

    cpu->dtb_compatible = "arm,cortex-a53";
    set_feature(&cpu->env, ARM_FEATURE_V8);
    set_feature(&cpu->env, ARM_FEATURE_NEON);
    set_feature(&cpu->env, ARM_FEATURE_GENERIC_TIMER);
    set_feature(&cpu->env, ARM_FEATURE_AARCH64);
    set_feature(&cpu->env, ARM_FEATURE_CBAR_RO);
    set_feature(&cpu->env, ARM_FEATURE_EL2);
    set_feature(&cpu->env, ARM_FEATURE_EL3);
    set_feature(&cpu->env, ARM_FEATURE_PMU);
    cpu->kvm_target = QEMU_KVM_ARM_TARGET_CORTEX_A53;
    cpu->midr = 0x410fd034;
    cpu->revidr = 0x00000000;
    cpu->reset_fpsid = 0x41034070;
    cpu->isar.mvfr0 = 0x10110222;
    cpu->isar.mvfr1 = 0x12111111;
    cpu->isar.mvfr2 = 0x00000043;
    cpu->ctr = 0x84448004; /* L1Ip = VIPT */
    cpu->reset_sctlr = 0x00c50838;
    cpu->isar.id_pfr0 = 0x00000131;
    cpu->isar.id_pfr1 = 0x00011011;
    cpu->isar.id_dfr0 = 0x03010066;
    cpu->id_afr0 = 0x00000000;
    cpu->isar.id_mmfr0 = 0x10101105;
    cpu->isar.id_mmfr1 = 0x40000000;
    cpu->isar.id_mmfr2 = 0x01260000;
    cpu->isar.id_mmfr3 = 0x02102211;
    cpu->isar.id_isar0 = 0x02101110;
    cpu->isar.id_isar1 = 0x13112111;
    cpu->isar.id_isar2 = 0x21232042;
    cpu->isar.id_isar3 = 0x01112131;
    cpu->isar.id_isar4 = 0x00011142;
    cpu->isar.id_isar5 = 0x00011121;
    cpu->isar.id_isar6 = 0;
    cpu->isar.id_aa64pfr0 = 0x00002222;
    cpu->isar.id_aa64dfr0 = 0x10305106;
    cpu->isar.id_aa64isar0 = 0x00011120;
    cpu->isar.id_aa64mmfr0 = 0x00001122; /* 40 bit physical addr */
    cpu->isar.dbgdidr = 0x3516d000;
    cpu->clidr = 0x0a200023;
    cpu->ccsidr[0] = 0x700fe01a; /* 32KB L1 dcache */
    cpu->ccsidr[1] = 0x201fe00a; /* 32KB L1 icache */
    cpu->ccsidr[2] = 0x707fe07a; /* 1024KB L2 cache */
    cpu->dcz_blocksize = 4; /* 64 bytes */
    cpu->gic_num_lrs = 4;
    cpu->gic_vpribits = 5;
    cpu->gic_vprebits = 5;
    define_arm_cp_regs(cpu, cortex_a72_a57_a53_cp_reginfo);
}

static void aarch64_a72_initfn(Object *obj)
{
    ARMCPU *cpu = ARM_CPU(obj);

    cpu->dtb_compatible = "arm,cortex-a72";
    set_feature(&cpu->env, ARM_FEATURE_V8);
    set_feature(&cpu->env, ARM_FEATURE_NEON);
    set_feature(&cpu->env, ARM_FEATURE_GENERIC_TIMER);
    set_feature(&cpu->env, ARM_FEATURE_AARCH64);
    set_feature(&cpu->env, ARM_FEATURE_CBAR_RO);
    set_feature(&cpu->env, ARM_FEATURE_EL2);
    set_feature(&cpu->env, ARM_FEATURE_EL3);
    set_feature(&cpu->env, ARM_FEATURE_PMU);
    cpu->midr = 0x410fd083;
    cpu->revidr = 0x00000000;
    cpu->reset_fpsid = 0x41034080;
    cpu->isar.mvfr0 = 0x10110222;
    cpu->isar.mvfr1 = 0x12111111;
    cpu->isar.mvfr2 = 0x00000043;
    cpu->ctr = 0x8444c004;
    cpu->reset_sctlr = 0x00c50838;
    cpu->isar.id_pfr0 = 0x00000131;
    cpu->isar.id_pfr1 = 0x00011011;
    cpu->isar.id_dfr0 = 0x03010066;
    cpu->id_afr0 = 0x00000000;
    cpu->isar.id_mmfr0 = 0x10201105;
    cpu->isar.id_mmfr1 = 0x40000000;
    cpu->isar.id_mmfr2 = 0x01260000;
    cpu->isar.id_mmfr3 = 0x02102211;
    cpu->isar.id_isar0 = 0x02101110;
    cpu->isar.id_isar1 = 0x13112111;
    cpu->isar.id_isar2 = 0x21232042;
    cpu->isar.id_isar3 = 0x01112131;
    cpu->isar.id_isar4 = 0x00011142;
    cpu->isar.id_isar5 = 0x00011121;
    cpu->isar.id_aa64pfr0 = 0x00002222;
    cpu->isar.id_aa64dfr0 = 0x10305106;
    cpu->isar.id_aa64isar0 = 0x00011120;
    cpu->isar.id_aa64mmfr0 = 0x00001124;
    cpu->isar.dbgdidr = 0x3516d000;
    cpu->clidr = 0x0a200023;
    cpu->ccsidr[0] = 0x701fe00a; /* 32KB L1 dcache */
    cpu->ccsidr[1] = 0x201fe012; /* 48KB L1 icache */
    cpu->ccsidr[2] = 0x707fe07a; /* 1MB L2 cache */
    cpu->dcz_blocksize = 4; /* 64 bytes */
    cpu->gic_num_lrs = 4;
    cpu->gic_vpribits = 5;
    cpu->gic_vprebits = 5;
    define_arm_cp_regs(cpu, cortex_a72_a57_a53_cp_reginfo);
}

/* -cpu max: if KVM is enabled, like -cpu host (best possible with this host);
 * otherwise, a CPU with as many features enabled as our emulation supports.
 * The version of '-cpu max' for qemu-system-arm is defined in cpu.c;
 * this only needs to handle 64 bits.
 */
static void aarch64_max_initfn(Object *obj)
{
    ARMCPU *cpu = ARM_CPU(obj);

    if (kvm_enabled()) {
        kvm_arm_set_cpu_features_from_host(cpu);
    } else if (tcg_enabled()) {
        uint64_t t;
        uint32_t u;
        aarch64_a57_initfn(obj);

        /*
         * Reset MIDR so the guest doesn't mistake our 'max' CPU type for a real
         * one and try to apply errata workarounds or use impdef features we
         * don't provide.
         * An IMPLEMENTER field of 0 means "reserved for software use";
         * ARCHITECTURE must be 0xf indicating "v7 or later, check ID registers
         * to see which features are present";
         * the VARIANT, PARTNUM and REVISION fields are all implementation
         * defined and we choose to define PARTNUM just in case guest
         * code needs to distinguish this QEMU CPU from other software
         * implementations, though this shouldn't be needed.
         */
        t = FIELD_DP64(0, MIDR_EL1, IMPLEMENTER, 0);
        t = FIELD_DP64(t, MIDR_EL1, ARCHITECTURE, 0xf);
        t = FIELD_DP64(t, MIDR_EL1, PARTNUM, 'Q');
        t = FIELD_DP64(t, MIDR_EL1, VARIANT, 0);
        t = FIELD_DP64(t, MIDR_EL1, REVISION, 0);
        cpu->midr = t;

        t = cpu->isar.id_aa64isar0;
        t = FIELD_DP64(t, ID_AA64ISAR0, AES, 2); /* AES + PMULL */
        t = FIELD_DP64(t, ID_AA64ISAR0, SHA1, 1);
        t = FIELD_DP64(t, ID_AA64ISAR0, SHA2, 2); /* SHA512 */
        t = FIELD_DP64(t, ID_AA64ISAR0, CRC32, 1);
        t = FIELD_DP64(t, ID_AA64ISAR0, ATOMIC, 2);
        t = FIELD_DP64(t, ID_AA64ISAR0, RDM, 1);
        t = FIELD_DP64(t, ID_AA64ISAR0, SHA3, 1);
        t = FIELD_DP64(t, ID_AA64ISAR0, SM3, 1);
        t = FIELD_DP64(t, ID_AA64ISAR0, SM4, 1);
        t = FIELD_DP64(t, ID_AA64ISAR0, DP, 1);
        t = FIELD_DP64(t, ID_AA64ISAR0, FHM, 1);
        t = FIELD_DP64(t, ID_AA64ISAR0, TS, 2); /* v8.5-CondM */
        t = FIELD_DP64(t, ID_AA64ISAR0, RNDR, 1);
        cpu->isar.id_aa64isar0 = t;

        t = cpu->isar.id_aa64isar1;
        t = FIELD_DP64(t, ID_AA64ISAR1, DPB, 2);
        t = FIELD_DP64(t, ID_AA64ISAR1, JSCVT, 1);
        t = FIELD_DP64(t, ID_AA64ISAR1, FCMA, 1);
        t = FIELD_DP64(t, ID_AA64ISAR1, SB, 1);
        t = FIELD_DP64(t, ID_AA64ISAR1, SPECRES, 1);
        t = FIELD_DP64(t, ID_AA64ISAR1, FRINTTS, 1);
        t = FIELD_DP64(t, ID_AA64ISAR1, LRCPC, 2); /* ARMv8.4-RCPC */
        cpu->isar.id_aa64isar1 = t;

        t = cpu->isar.id_aa64pfr0;
        t = FIELD_DP64(t, ID_AA64PFR0, SVE, 1);
        t = FIELD_DP64(t, ID_AA64PFR0, FP, 1);
        t = FIELD_DP64(t, ID_AA64PFR0, ADVSIMD, 1);
        t = FIELD_DP64(t, ID_AA64PFR0, SEL2, 1);
        t = FIELD_DP64(t, ID_AA64PFR0, DIT, 1);
        cpu->isar.id_aa64pfr0 = t;

        t = cpu->isar.id_aa64pfr1;
        t = FIELD_DP64(t, ID_AA64PFR1, BT, 1);
        t = FIELD_DP64(t, ID_AA64PFR1, SSBS, 2);
        /*
         * Begin with full support for MTE. This will be downgraded to MTE=0
         * during realize if the board provides no tag memory, much like
         * we do for EL2 with the virtualization=on property.
         */
        t = FIELD_DP64(t, ID_AA64PFR1, MTE, 2);
        cpu->isar.id_aa64pfr1 = t;

        t = cpu->isar.id_aa64mmfr0;
        t = FIELD_DP64(t, ID_AA64MMFR0, PARANGE, 5); /* PARange: 48 bits */
        cpu->isar.id_aa64mmfr0 = t;

        t = cpu->isar.id_aa64mmfr1;
        t = FIELD_DP64(t, ID_AA64MMFR1, HPDS, 1); /* HPD */
        t = FIELD_DP64(t, ID_AA64MMFR1, LO, 1);
        t = FIELD_DP64(t, ID_AA64MMFR1, VH, 1);
        t = FIELD_DP64(t, ID_AA64MMFR1, PAN, 2); /* ATS1E1 */
        t = FIELD_DP64(t, ID_AA64MMFR1, VMIDBITS, 2); /* VMID16 */
        t = FIELD_DP64(t, ID_AA64MMFR1, XNX, 1); /* TTS2UXN */
        cpu->isar.id_aa64mmfr1 = t;

        t = cpu->isar.id_aa64mmfr2;
        t = FIELD_DP64(t, ID_AA64MMFR2, UAO, 1);
        t = FIELD_DP64(t, ID_AA64MMFR2, CNP, 1); /* TTCNP */
        t = FIELD_DP64(t, ID_AA64MMFR2, ST, 1); /* TTST */
        cpu->isar.id_aa64mmfr2 = t;

        /* Replicate the same data to the 32-bit id registers.  */
        u = cpu->isar.id_isar5;
        u = FIELD_DP32(u, ID_ISAR5, AES, 2); /* AES + PMULL */
        u = FIELD_DP32(u, ID_ISAR5, SHA1, 1);
        u = FIELD_DP32(u, ID_ISAR5, SHA2, 1);
        u = FIELD_DP32(u, ID_ISAR5, CRC32, 1);
        u = FIELD_DP32(u, ID_ISAR5, RDM, 1);
        u = FIELD_DP32(u, ID_ISAR5, VCMA, 1);
        cpu->isar.id_isar5 = u;

        u = cpu->isar.id_isar6;
        u = FIELD_DP32(u, ID_ISAR6, JSCVT, 1);
        u = FIELD_DP32(u, ID_ISAR6, DP, 1);
        u = FIELD_DP32(u, ID_ISAR6, FHM, 1);
        u = FIELD_DP32(u, ID_ISAR6, SB, 1);
        u = FIELD_DP32(u, ID_ISAR6, SPECRES, 1);
        cpu->isar.id_isar6 = u;

        u = cpu->isar.id_pfr0;
        u = FIELD_DP32(u, ID_PFR0, DIT, 1);
        cpu->isar.id_pfr0 = u;

        u = cpu->isar.id_pfr2;
        u = FIELD_DP32(u, ID_PFR2, SSBS, 1);
        cpu->isar.id_pfr2 = u;

        u = cpu->isar.id_mmfr3;
        u = FIELD_DP32(u, ID_MMFR3, PAN, 2); /* ATS1E1 */
        cpu->isar.id_mmfr3 = u;

        u = cpu->isar.id_mmfr4;
        u = FIELD_DP32(u, ID_MMFR4, HPDS, 1); /* AA32HPD */
        u = FIELD_DP32(u, ID_MMFR4, AC2, 1); /* ACTLR2, HACTLR2 */
        u = FIELD_DP32(u, ID_MMFR4, CNP, 1); /* TTCNP */
        u = FIELD_DP32(u, ID_MMFR4, XNX, 1); /* TTS2UXN */
        cpu->isar.id_mmfr4 = u;

        t = cpu->isar.id_aa64dfr0;
        t = FIELD_DP64(t, ID_AA64DFR0, PMUVER, 5); /* v8.4-PMU */
        cpu->isar.id_aa64dfr0 = t;

        u = cpu->isar.id_dfr0;
        u = FIELD_DP32(u, ID_DFR0, PERFMON, 5); /* v8.4-PMU */
        cpu->isar.id_dfr0 = u;

        u = cpu->isar.mvfr1;
        u = FIELD_DP32(u, MVFR1, FPHP, 3);      /* v8.2-FP16 */
        u = FIELD_DP32(u, MVFR1, SIMDHP, 2);    /* v8.2-FP16 */
        cpu->isar.mvfr1 = u;

#ifdef CONFIG_USER_ONLY
        /* For usermode -cpu max we can use a larger and more efficient DCZ
         * blocksize since we don't have to follow what the hardware does.
         */
        cpu->ctr = 0x80038003; /* 32 byte I and D cacheline size, VIPT icache */
        cpu->dcz_blocksize = 7; /*  512 bytes */
#endif

        cpu_pauth_add_props(obj);
    }

    cpu_sve_add_props(obj);
    cpu_sve_add_props_max(obj);
}

static const ARMCPUInfo aarch64_cpus[] = {
    { .name = "cortex-a57",         .initfn = aarch64_a57_initfn },
    { .name = "cortex-a53",         .initfn = aarch64_a53_initfn },
    { .name = "cortex-a72",         .initfn = aarch64_a72_initfn },
    { .name = "max",                .initfn = aarch64_max_initfn },
};

static bool aarch64_cpu_get_aarch64(Object *obj, Error **errp)
{
    ARMCPU *cpu = ARM_CPU(obj);

    return arm_feature(&cpu->env, ARM_FEATURE_AARCH64);
}

static void aarch64_cpu_set_aarch64(Object *obj, bool value, Error **errp)
{
    ARMCPU *cpu = ARM_CPU(obj);

    /* At this time, this property is only allowed if KVM is enabled.  This
     * restriction allows us to avoid fixing up functionality that assumes a
     * uniform execution state like do_interrupt.
     */
    if (value == false) {
        if (!kvm_enabled() || !kvm_arm_aarch32_supported()) {
            error_setg(errp, "'aarch64' feature cannot be disabled "
                             "unless KVM is enabled and 32-bit EL1 "
                             "is supported");
            return;
        }
        unset_feature(&cpu->env, ARM_FEATURE_AARCH64);
    } else {
        set_feature(&cpu->env, ARM_FEATURE_AARCH64);
    }
}

static void aarch64_cpu_finalizefn(Object *obj)
{
}

static gchar *aarch64_gdb_arch_name(CPUState *cs)
{
    return g_strdup("aarch64");
}

static void aarch64_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    ARMCPU *cpu = ARM_CPU(cs);
    CPUARMState *env = &cpu->env;
    uint32_t psr = pstate_read(env);
    int i;
    int el = arm_current_el(env);
    const char *ns_status;

    qemu_fprintf(f, " PC=%016" PRIx64 " ", env->pc);
    for (i = 0; i < 32; i++) {
        if (i == 31) {
            qemu_fprintf(f, " SP=%016" PRIx64 "\n", env->xregs[i]);
        } else {
            qemu_fprintf(f, "X%02d=%016" PRIx64 "%s", i, env->xregs[i],
                         (i + 2) % 3 ? " " : "\n");
        }
    }

    if (arm_feature(env, ARM_FEATURE_EL3) && el != 3) {
        ns_status = env->cp15.scr_el3 & SCR_NS ? "NS " : "S ";
    } else {
        ns_status = "";
    }
    qemu_fprintf(f, "PSTATE=%08x %c%c%c%c %sEL%d%c",
                 psr,
                 psr & PSTATE_N ? 'N' : '-',
                 psr & PSTATE_Z ? 'Z' : '-',
                 psr & PSTATE_C ? 'C' : '-',
                 psr & PSTATE_V ? 'V' : '-',
                 ns_status,
                 el,
                 psr & PSTATE_SP ? 'h' : 't');

    if (cpu_isar_feature(aa64_bti, cpu)) {
        qemu_fprintf(f, "  BTYPE=%d", (psr & PSTATE_BTYPE) >> 10);
    }
    if (!(flags & CPU_DUMP_FPU)) {
        qemu_fprintf(f, "\n");
        return;
    }
    if (fp_exception_el(env, el) != 0) {
        qemu_fprintf(f, "    FPU disabled\n");
        return;
    }
    qemu_fprintf(f, "     FPCR=%08x FPSR=%08x\n",
                 vfp_get_fpcr(env), vfp_get_fpsr(env));

    if (cpu_isar_feature(aa64_sve, cpu) && sve_exception_el(env, el) == 0) {
        int j, zcr_len = cpu_sve_get_zcr_len_for_el(env, el);

        for (i = 0; i <= FFR_PRED_NUM; i++) {
            bool eol;
            if (i == FFR_PRED_NUM) {
                qemu_fprintf(f, "FFR=");
                /* It's last, so end the line.  */
                eol = true;
            } else {
                qemu_fprintf(f, "P%02d=", i);
                switch (zcr_len) {
                case 0:
                    eol = i % 8 == 7;
                    break;
                case 1:
                    eol = i % 6 == 5;
                    break;
                case 2:
                case 3:
                    eol = i % 3 == 2;
                    break;
                default:
                    /* More than one quadword per predicate.  */
                    eol = true;
                    break;
                }
            }
            for (j = zcr_len / 4; j >= 0; j--) {
                int digits;
                if (j * 4 + 4 <= zcr_len + 1) {
                    digits = 16;
                } else {
                    digits = (zcr_len % 4 + 1) * 4;
                }
                qemu_fprintf(f, "%0*" PRIx64 "%s", digits,
                             env->vfp.pregs[i].p[j],
                             j ? ":" : eol ? "\n" : " ");
            }
        }

        for (i = 0; i < 32; i++) {
            if (zcr_len == 0) {
                qemu_fprintf(f, "Z%02d=%016" PRIx64 ":%016" PRIx64 "%s",
                             i, env->vfp.zregs[i].d[1],
                             env->vfp.zregs[i].d[0], i & 1 ? "\n" : " ");
            } else if (zcr_len == 1) {
                qemu_fprintf(f, "Z%02d=%016" PRIx64 ":%016" PRIx64
                             ":%016" PRIx64 ":%016" PRIx64 "\n",
                             i, env->vfp.zregs[i].d[3], env->vfp.zregs[i].d[2],
                             env->vfp.zregs[i].d[1], env->vfp.zregs[i].d[0]);
            } else {
                for (j = zcr_len; j >= 0; j--) {
                    bool odd = (zcr_len - j) % 2 != 0;
                    if (j == zcr_len) {
                        qemu_fprintf(f, "Z%02d[%x-%x]=", i, j, j - 1);
                    } else if (!odd) {
                        if (j > 0) {
                            qemu_fprintf(f, "   [%x-%x]=", j, j - 1);
                        } else {
                            qemu_fprintf(f, "     [%x]=", j);
                        }
                    }
                    qemu_fprintf(f, "%016" PRIx64 ":%016" PRIx64 "%s",
                                 env->vfp.zregs[i].d[j * 2 + 1],
                                 env->vfp.zregs[i].d[j * 2],
                                 odd || j == 0 ? "\n" : ":");
                }
            }
        }
    } else {
        for (i = 0; i < 32; i++) {
            uint64_t *q = aa64_vfp_qreg(env, i);
            qemu_fprintf(f, "Q%02d=%016" PRIx64 ":%016" PRIx64 "%s",
                         i, q[1], q[0], (i & 1 ? "\n" : " "));
        }
    }
}

static void arm_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    ARMCPU *cpu = ARM_CPU(cs);
    CPUARMState *env = &cpu->env;

    if (is_a64(env)) {
        aarch64_cpu_dump_state(cs, f, flags);
    } else {
        arm32_cpu_dump_state(cs, f, flags);
    }
}

static void aarch64_cpu_class_init(ObjectClass *oc, void *data)
{
    CPUClass *cc = CPU_CLASS(oc);

    cc->gdb_read_register = aarch64_cpu_gdb_read_register;
    cc->gdb_write_register = aarch64_cpu_gdb_write_register;
    cc->gdb_num_core_regs = 34;
    cc->gdb_core_xml_file = "aarch64-core.xml";
    cc->gdb_arch_name = aarch64_gdb_arch_name;
    cc->dump_state = arm_cpu_dump_state;

#ifndef CONFIG_USER_ONLY
    cc->write_elf64_note = arm_cpu_write_elf64_note;
#endif /* !CONFIG_USER_ONLY */

    object_class_property_add_bool(oc, "aarch64", aarch64_cpu_get_aarch64,
                                   aarch64_cpu_set_aarch64);
    object_class_property_set_description(oc, "aarch64",
                                          "Set on/off to enable/disable aarch64 "
                                          "execution state ");
}

static void aarch64_cpu_instance_init(Object *obj)
{
    ARMCPUClass *acc = ARM_CPU_GET_CLASS(obj);

    acc->info->initfn(obj);
    arm_cpu_post_init(obj);
}

static void cpu_register_class_init(ObjectClass *oc, void *data)
{
    ARMCPUClass *acc = ARM_CPU_CLASS(oc);

    acc->info = data;
}

static void aarch64_cpu_register(const ARMCPUInfo *info)
{
    TypeInfo type_info = {
        .parent = TYPE_AARCH64_CPU,
        .instance_size = sizeof(ARMCPU),
        .instance_init = aarch64_cpu_instance_init,
        .class_size = sizeof(ARMCPUClass),
        .class_init = info->class_init ?: cpu_register_class_init,
        .class_data = (void *)info,
    };

    type_info.name = g_strdup_printf("%s-" TYPE_ARM_CPU, info->name);
    type_register(&type_info);
    g_free((void *)type_info.name);
}

static const TypeInfo aarch64_cpu_type_info = {
    .name = TYPE_AARCH64_CPU,
    .parent = TYPE_ARM_CPU,
    .instance_size = sizeof(ARMCPU),
    .instance_finalize = aarch64_cpu_finalizefn,
    .abstract = true,
    .class_size = sizeof(AArch64CPUClass),
    .class_init = aarch64_cpu_class_init,
};

static void aarch64_cpu_register_types(void)
{
    size_t i;

    type_register_static(&aarch64_cpu_type_info);

    for (i = 0; i < ARRAY_SIZE(aarch64_cpus); ++i) {
        aarch64_cpu_register(&aarch64_cpus[i]);
    }
}

type_init(aarch64_cpu_register_types)
