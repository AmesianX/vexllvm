#include <stdio.h>
#include <stdlib.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include "ptimgarch.h"
#include "guestcpustate.h"

PTImgArch::PTImgArch(GuestPTImg* in_gs, int in_pid)
: gs(in_gs)
, child_pid(in_pid)
, steps(0)
, blocks(0)
{
	const char* step_gauge;

	log_steps = (getenv("VEXLLVM_LOG_STEPS")) ? true : false;

	step_gauge = getenv("VEXLLVM_STEP_GAUGE");
	if (step_gauge == NULL) {
		log_gauge_overflow = 0;
	} else {
		log_gauge_overflow = atoi(step_gauge);
		fprintf(stderr,
			"STEPS BETWEEN UPDATE: %d.\n",
			log_gauge_overflow);
	}
}

PTImgArch::~PTImgArch() {}

void PTImgArch::waitForSingleStep(void)
{
	int	err, status;

	steps++;
	err = ptrace(PTRACE_SINGLESTEP, child_pid, NULL, NULL);
	if(err < 0) {
		perror("PTImgChk::doStep ptrace single step");
		exit(1);
	}
	wait(&status);

	//TODO: real signal handling needed, but the main process
	//doesn't really have that yet...
	// 1407
	if (WIFSTOPPED(status)) {
		switch (WSTOPSIG(status)) {
		case SIGTRAP:
			/* trap is expected for single-stepping */
			break;
		case SIGFPE:
			fprintf(stderr, "[PTImgArch] FPE!\n");
			break;
		case SIGSEGV: {
			guest_ptr	cur_pc(gs->getCPUState()->getPC());

			fprintf(stderr, "[PTImgArch] SIGSEGV!\n");
			/* "fake progress" */

			gs->getCPUState()->setPC(guest_ptr(0xbadbadbadbad));
			pushRegisters();
			gs->getCPUState()->setPC(cur_pc);
			break;
			}
		default:
			perror("child received a signal or ptrace can't single step");
		}
	}  else {
		perror("Unknown status in waitForSingleStep");
		abort();
	}
	if (log_gauge_overflow && (steps % log_gauge_overflow) == 0) {
		char	c = "/-\\|/-\\|"[(steps / log_gauge_overflow)%8];
		fprintf(stderr,
			//"STEPS %09"PRIu64" %c %09"PRIu64" BLOCKS\r",
			"STEPS %09"PRIu64" %c %09"PRIu64" BLOCKS\n",
			steps, c, blocks);
	}
}

void PTImgArch::copyIn(guest_ptr dst, const void* src, unsigned int bytes) const
{
	char*		in_addr;
	guest_ptr	out_addr, end_addr;

	assert ((bytes % sizeof(long)) == 0);

	in_addr = (char*)src;
	out_addr = dst;
	end_addr = out_addr + bytes;

	while (out_addr < end_addr) {
		long data = *(long*)in_addr;
		int err = ptrace(PTRACE_POKEDATA, child_pid, out_addr.o, data);
		assert(err == 0);
		in_addr += sizeof(long);
		out_addr.o += sizeof(long);
	}
}
