#include "Sugar.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#include "elfimg.h"
#include "vexexec.h"
#include "guestcpustate.h"
#include "guestptimg.h"

using namespace llvm;

static VexExec *vexexec;

void dumpIRSBs(void)
{
	std::cerr << "DUMPING LOGS" << std::cerr;
	vexexec->dumpLogs(std::cerr);
}

int main(int argc, char* argv[], char* envp[])
{
	GuestPTImg	*gs;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s program_path <args>\n", argv[0]);
		return -1;
	}

	if (getenv("VEXLLVM_ATTACH") != NULL) {
		gs = GuestPTImg::createAttached<GuestPTImg>(
			atoi(getenv("VEXLLVM_ATTACH")),
			argc - 1, argv + 1, envp);
	} else {
		gs = GuestPTImg::create<GuestPTImg>(argc - 1, argv + 1, envp);
	}
	vexexec = VexExec::create<VexExec,Guest>(gs);
	assert (vexexec && "Could not create vexexec");
	
	vexexec->run();

	delete vexexec;
	delete gs;

	return 0;
}
