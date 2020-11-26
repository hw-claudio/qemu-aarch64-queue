/*
 * Accelerator interface, specializes CPUClass
 *
 * Copyright 2020 SUSE LLC
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef ACCEL_CPU_H
#define ACCEL_CPU_H

struct AccelCPUClass {
    /*< private >*/
    ObjectClass parent_class;
    /*< public >*/

    void (*cpu_class_init)(CPUClass *cc);
    void (*cpu_instance_init)(CPUState *cpu);
    void (*cpu_realizefn)(CPUState *cpu, Error **errp);
};

#define TYPE_ACCEL_CPU "accel-" CPU_RESOLVING_TYPE
#define ACCEL_CPU_NAME(name) (name "-" TYPE_ACCEL_CPU)
typedef struct AccelCPUClass AccelCPUClass;
DECLARE_CLASS_CHECKERS(AccelCPUClass, ACCEL_CPU, TYPE_ACCEL_CPU)

#endif /* ACCEL_CPU_H */
