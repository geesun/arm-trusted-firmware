#include <arch_helpers.h>
#include <assert.h>
#include <cci.h>
#include <console.h>
#include <debug.h>
#include <gicv2.h>
#include <mmio.h>
#include <psci.h>
#include <platform_def.h>

static uintptr_t hikey960_sec_entrypoint;
static const plat_psci_ops_t hikey960_psci_ops = {
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	hikey960_sec_entrypoint = sec_entrypoint;

	INFO("%s: sec_entrypoint=0x%lx\n", __func__,
	     (unsigned long)hikey960_sec_entrypoint);

	/*
	 * Initialize PSCI ops struct
	 */
	*psci_ops = &hikey960_psci_ops;

	return 0;
}
