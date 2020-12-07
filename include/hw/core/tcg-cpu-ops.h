/*
 * TCG-Specific operations that are not meaningful for hardware accelerators
 *
 * Copyright 2020 SUSE LLC
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef TCG_CPU_OPS_H
#define TCG_CPU_OPS_H

#include "hw/core/cpu.h"
#include "exec/memattrs.h"

/**
 * struct TcgCpuOperations: TCG operations specific to a CPU class
 */
typedef struct TcgCpuOperations {
    /**
     * @initialize: Initalize TCG state
     *
     * Called when the first CPU is realized.
     */
    void (*initialize)(void);
    /**
     * @synchronize_from_tb: Synchronize state from a TCG #TranslationBlock
     *
     * This is called when we abandon execution of a TB before
     * starting it, and must set all parts of the CPU state which
     * the previous TB in the chain may not have updated. This
     * will need to do more. If this hook is not implemented then
     * the default is to call @set_pc(tb->pc).
     */
    void (*synchronize_from_tb)(CPUState *cpu, struct TranslationBlock *tb);
    /** @cpu_exec_enter: Callback for cpu_exec preparation */
    void (*cpu_exec_enter)(CPUState *cpu);
    /** @cpu_exec_exit: Callback for cpu_exec cleanup */
    void (*cpu_exec_exit)(CPUState *cpu);
    /** @cpu_exec_interrupt: Callback for processing interrupts in cpu_exec */
    bool (*cpu_exec_interrupt)(CPUState *cpu, int interrupt_request);
    /** @do_interrupt: Callback for interrupt handling. */
    void (*do_interrupt)(CPUState *cpu);

    /**
     * @tlb_fill: Handle a softmmu tlb miss or user-only address fault
     *
     * For system mode, if the access is valid, call tlb_set_page
     * and return true; if the access is invalid, and probe is
     * true, return false; otherwise raise an exception and do
     * not return.  For user-only mode, always raise an exception
     * and do not return.
     */
    bool (*tlb_fill)(CPUState *cpu, vaddr address, int size,
                     MMUAccessType access_type, int mmu_idx,
                     bool probe, uintptr_t retaddr);
    /** @debug_excp_handler: Callback for handling debug exceptions */
    void (*debug_excp_handler)(CPUState *cpu);

#ifndef CONFIG_USER_ONLY
    /**
     * @do_transaction_failed: Callback for handling failed memory transactions
     * (ie bus faults or external aborts; not MMU faults)
     */
    void (*do_transaction_failed)(CPUState *cpu, hwaddr physaddr, vaddr addr,
                                  unsigned size, MMUAccessType access_type,
                                  int mmu_idx, MemTxAttrs attrs,
                                  MemTxResult response, uintptr_t retaddr);

#endif /* !CONFIG_USER_ONLY */

} TcgCpuOperations;

#endif /* TCG_CPU_OPS_H */
