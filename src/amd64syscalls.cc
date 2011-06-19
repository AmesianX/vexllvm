#include "syscalls.h"
#include "guest.h"
#include <sys/syscall.h>
#include "amd64cpustate.h"
#include <sys/errno.h>
#include <sys/prctl.h>
#include <asm/prctl.h> 
#include "guestmem.h"
#include <vector>
#include "Sugar.h"

/* this header loads all of the system headers outside of the namespace */
#include "translatedsyscall.h"

static GuestMem::Mapping* g_syscall_last_mapping = NULL;

namespace AMD64 {
	/* this will hold the last mapping that was changed during the syscall
	   ... hopefully there is only one... */

	/* we have to define a few platform specific types, non platform
	   specific interface code goes in the translatedutil.h */
	#define TARGET_X86_64
	#define TARGET_I386
	#define TARGET_ABI_BITS 64
	#define TARGET_ABI64
	#define abi_long 	long
	#define abi_ulong	unsigned long
	#define target_ulong 	abi_ulong
	#define target_long 	abi_long
	#define HOST_PAGE_ALIGN(x) \
		((uintptr_t)x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)
	#define cpu_to_uname_machine(...)	"x86_64"
	/* Maximum number of LDT entries supported. */
	#define TARGET_LDT_ENTRIES	8192
	/* The size of each LDT entry. */
	#define TARGET_LDT_ENTRY_SIZE	8
		
	/* memory mapping requires wrappers based on the host address
	   space limitations */
	/* oh poo, this needs to record mappings... they don't right now */
	#define target_mmap(a, l, p, f, fd, o) \
		(abi_long)(uintptr_t)mmap((void*)a, l, p, f, fd, o)
	#define mmap_find_vma(s, l)	mmap_find_vma_flags(s, l, MAP_32BIT)

	/* our implementations of stuff ripped out of the qemu files */
	#include "translatedutil.h"
	/* generic type conversion utility routines */
	#include "translatedthunk.h"
	/* terminal io definitions that are plaform dependent */
	#include "amd64termbits.h"
	/* platform dependent signals */
	#include "amd64signal.h"
	/* structure and flag definitions that are platform independent */
	#include "translatedsyscalldefs.h"
	/* we need load our platform specific stuff as well */
	#include "amd64syscallnumbers.h"
	/* this constructs all of our syscall translation code */
	#include "translatedsyscall.c"
	/* this constructs all of our syscall translation code */
	#include "translatedsignal.c"
	/* this has some tables and such used for type conversion */
	#include "translatedthunk.c"
	
	std::vector<int> g_host_to_guest_syscalls(512);
	std::vector<int> g_guest_to_host_syscalls(512);
	bool syscall_mapping_init() {
		foreach(it, g_host_to_guest_syscalls.begin(),
			g_host_to_guest_syscalls.end()) *it = -1;
		foreach(it, g_guest_to_host_syscalls.begin(),
			g_guest_to_host_syscalls.end()) *it = -1;
		#define SYSCALL_RELATION(name, host, guest) 	\
			g_host_to_guest_syscalls[host] = guest;	\
			g_guest_to_host_syscalls[guest] = host;
		#include "syscallsmapping.h"
		return true;
	}
	bool dummy_syscall_mapping = syscall_mapping_init();
}


int Syscalls::translateAMD64Syscall(int sys_nr) const {
	if(sys_nr > AMD64::g_guest_to_host_syscalls.size())
		return -1;

	int host_sys_nr = AMD64::g_guest_to_host_syscalls[sys_nr];
	if(!host_sys_nr) {
		return -1;
	}
	return host_sys_nr;
}

uintptr_t Syscalls::applyAMD64Syscall(
	SyscallParams& args,
	GuestMem::Mapping& m)
{
	uintptr_t sc_ret = ~0ULL;

	/* special syscalls that we handle per arch, these
	   generally supersede any pass through or translated 
	   behaviors, but they can just alter the args and let
	   the other mechanisms finish the job */
	switch (args.getSyscall()) {
	case SYS_arch_prctl:
		if(AMD64_arch_prctl(args, m, sc_ret))
			return sc_ret;
		break;
	default:
		break;
	}
		
	/* if the host and guest are identical, then just pass through */
	if(!force_translation && guest->getArch() == Arch::getHostArch()) {
		return passthroughSyscall(args, m);
	}
	
	sc_ret = AMD64::do_syscall(NULL,
		args.getSyscall(),
		args.getArg(0),
		args.getArg(1),
		args.getArg(2),
		args.getArg(3),
		args.getArg(4),
		args.getArg(5));
		
	if(g_syscall_last_mapping) {
		m = *g_syscall_last_mapping;
		g_syscall_last_mapping = NULL;
	}
	
	return sc_ret;
}

SYSCALL_BODY(AMD64, arch_prctl) {
	AMD64CPUState* cpu_state = (AMD64CPUState*)this->cpu_state;
	if(args.getArg(0) == ARCH_GET_FS) {
		sc_ret = cpu_state->getFSBase();
	} else if(args.getArg(0) == ARCH_SET_FS) {
		cpu_state->setFSBase(args.getArg(1));
		sc_ret = 0;
	} else {
		/* nothing else is supported by VEX */
		sc_ret = -EPERM;
	}
	return true;
}