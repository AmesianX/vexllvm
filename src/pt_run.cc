#include "Sugar.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#include "elfimg.h"
#include "vexexec.h"
#include "vexcpustate.h"
#include "guestptimg.h"
#include "procargs.h"

static VexExec *vexexec;

void dumpIRSBs(void)
{
	std::cerr << "DUMPING LOGS\n";
	vexexec->dumpLogs(std::cerr);
}

GuestPTImg* createAttached(void)
{
	ProcArgs	*pa;
	int		pid;

	pid = atoi(getenv("VEXLLVM_ATTACH"));
	pa = new ProcArgs(pid);
	return GuestPTImg::createAttached<GuestPTImg>(pid, pa->getArgv());
}

int main(int argc, char* argv[], char* envp[])
{
	GuestPTImg	*gs;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s program_path <args>\n", argv[0]);
		return -1;
	}

	VexCPUState::registerCPUs();

	if (getenv("VEXLLVM_ATTACH") != NULL) {
		gs = createAttached();
	} else {
		gs = GuestPTImg::create<GuestPTImg>(argc - 1, argv + 1, envp);
	}

	gs->patchVDSO();

	vexexec = VexExec::create<VexExec,Guest>(gs);
	assert (vexexec && "Could not create vexexec");
	
	vexexec->run();

	delete vexexec;
	delete gs;

	return 0;
}
