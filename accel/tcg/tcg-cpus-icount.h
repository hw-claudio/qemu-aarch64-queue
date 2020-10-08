/*
 * QEMU TCG Single Threaded vCPUs implementation using instruction counting
 *
 * Copyright 2020 SUSE LLC
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef TCG_CPUS_ICOUNT_H
#define TCG_CPUS_ICOUNT_H

void handle_icount_deadline(void);
void prepare_icount_for_run(CPUState *cpu);
void process_icount_data(CPUState *cpu);

#endif /* TCG_CPUS_ICOUNT_H */
