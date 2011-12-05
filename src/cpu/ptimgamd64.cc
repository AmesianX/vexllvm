#include "cpu/ptimgamd64.h"
extern "C" {
#include <valgrind/libvex_guest_amd64.h>
}

extern "C" {
extern void amd64g_dirtyhelper_CPUID_baseline ( VexGuestAMD64State* st );
}

struct user_regs_desc
{
	const char*	name;
	int		user_off;
	int		vex_off;
};

#define USERREG_ENTRY(x,y)	{ 		\
	#x,					\
	offsetof(struct user_regs_struct, x),	\
	offsetof(VexGuestAMD64State, guest_##y) }


#define REG_COUNT	18
#define GPR_COUNT	16
const struct user_regs_desc user_regs_desc_tab[REG_COUNT] =
{
	USERREG_ENTRY(rip, RIP),
	USERREG_ENTRY(rax, RAX),
	USERREG_ENTRY(rbx, RBX),
	USERREG_ENTRY(rcx, RCX),
	USERREG_ENTRY(rdx, RDX),
	USERREG_ENTRY(rsp, RSP),
	USERREG_ENTRY(rbp, RBP),
	USERREG_ENTRY(rdi, RDI),
	USERREG_ENTRY(rsi, RSI),
	USERREG_ENTRY(r8, R8),
	USERREG_ENTRY(r9, R9),
	USERREG_ENTRY(r10, R10),
	USERREG_ENTRY(r11, R11),
	USERREG_ENTRY(r12, R12),
	USERREG_ENTRY(r13, R13),
	USERREG_ENTRY(r14, R14),
	USERREG_ENTRY(r15, R15),
	USERREG_ENTRY(fs_base, FS_ZERO)
	// TODO: segments?
	// but valgrind/vex seems to not really fully handle them, how sneaky
};

#define get_reg_user(x,y)	*((uint64_t*)&x[user_regs_desc_tab[y].user_off]);
#define get_reg_vex(x,y)	*((uint64_t*)&x[user_regs_desc_tab[y].vex_off])
#define get_reg_name(y)		user_regs_desc_tab[y].name

PTImgAMD64::PTImgAMD64(Guest* gs, int in_pid)
: PTImgArch(gs, in_pid)
, xchk_eflags(getenv("VEXLLVM_XCHK_EFLAGS") ? true : false)
, fixup_eflags(getenv("VEXLLVM_NO_EFLAGS_FIXUP") ? false : true)
{}

bool PTImgAMD64::isRegMismatch(
	const VexGuestAMD64State& state,
	const user_regs_struct& regs) const
{
	uint8_t		*user_regs_ctx;
	uint8_t		*vex_reg_ctx;

	vex_reg_ctx = (uint8_t*)&state;
	user_regs_ctx = (uint8_t*)&regs;

	for (unsigned int i = 0; i < GPR_COUNT; i++) {
		uint64_t	ureg, vreg;

		ureg = get_reg_user(user_regs_ctx, i);
		vreg = get_reg_vex(vex_reg_ctx, i);
		if (ureg != vreg) return true;
	}

	return false;
}


void PTImgAMD64::getRegs(user_regs_struct& regs) const
{
	int	err;

	err = ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
	if (err >= 0) return;

	perror("PTImgAMD64::getRegs");

	/* The politics of failure have failed.
	 * It is time to make them work again.
	 * (this is temporary nonsense to get
	 *  /usr/bin/make xchk tests not to hang). */
	waitpid(child_pid, NULL, 0);
	kill(child_pid, SIGABRT);
	raise(SIGKILL);
	_exit(-1);
	abort();
}

const VexGuestAMD64State& getVexState(void) const
{
	return *((const VexGuestAMD64State*)gs->getCPUState()->getStateData());
}

bool PTImgAMD64::isMatch(void) const
{
	user_regs_struct	regs;
	user_fpregs_struct	fpregs;
	int			err;
	bool			x86_fail, sse_ok, seg_fail,
				x87_ok, mem_ok, stack_ok;a
	const VexGuestAMD64State& state(getVexState());

	getRegs(regs);

	err = ptrace(PTRACE_GETFPREGS, child_pid, NULL, &fpregs);
	assert (err != -1 && "couldn't PTRACE_GETFPREGS");

	x86_fail = isRegMismatch(state, regs);

	// if ptrace fs_base == 0, then we have a fixup but the ptraced
	// process doesn't-- don't flag as an error!
	seg_fail = (regs.fs_base != 0)
		? (regs.fs_base ^ state.guest_FS_ZERO)
		: false;

	//TODO: consider evaluating CC fields, ACFLAG, etc?
	//TODO: segments? shouldn't pop up on user progs..
	//TODO: what is this for? well besides the obvious
	// /* 192 */ULong guest_SSEROUND;
	if (xchk_eflags) {
		uint64_t guest_rflags, eflags;
		eflags = regs.eflags;
		eflags &= FLAGS_MASK;
		guest_rflags = get_rflags(state);
		if (eflags != guest_rflags)
			return false;
	}

	sse_ok = !memcmp(
		&state.guest_XMM0,
		&fpregs.xmm_space[0],
		sizeof(fpregs.xmm_space));

	//TODO: check the top pointer of the floating point stack..
	// /* FPU */
	// /* 456 */UInt  guest_FTOP;
	//FPTAG?

	//what happens if the FP unit is doing long doubles?
	//if VEX supports this, it probably should be filling in
	//the extra 16 bits with the appropriate thing on MMX
	//operations, like a real x86 cpu
	x87_ok = true;
	for(int i = 0; i < 8; ++i) {
		int r  = (state.guest_FTOP + i) & 0x7;
		bool is_ok = fcompare(
			&fpregs.st_space[4 * i],
			state.guest_FPREG[r]);
		if(!is_ok) {
			x87_ok = false;
			break;
		}
	}

	//TODO: what are these?
	// /* 536 */ ULong guest_FPROUND;
	// /* 544 */ ULong guest_FC3210;

	//TODO: more stuff that is likely unneeded
	// other vex internal state (tistart, nraddr, etc)

	mem_ok = isMatchMemLog();
	stack_ok = isStackMatch(regs);

	return 	!x86_fail && x87_ok & sse_ok &&
		!seg_fail && mem_ok && stack_ok;
}

#define MAX_INS_BUF	32
bool PTImgAMD64::canFixup(
	const std::vector<InstExtent>& insts,
	bool has_memlog)
{
	uint64_t	fix_op;
	guest_ptr	fix_addr;
	uint8_t		ins_buf[MAX_INS_BUF];

	foreach (it, insts.begin(), insts.end()) {
		const InstExtent	&inst(*it);
		uint16_t		op16;
		uint8_t			op8;
		int			off;

		assert (inst.second < MAX_INS_BUF);
		/* guest ip's are mapped in our addr space, no need for IPC */
		mem->memcpy(ins_buf, inst.first, inst.second);
		fix_addr = inst.first;

		/* filter out fucking prefix byte */
		off = ( ins_buf[0] == 0x41 ||
			ins_buf[0] == 0x44 ||
			ins_buf[0] == 0x48 ||
			ins_buf[0] == 0x49) ? 1 : 0;
		op8 = ins_buf[off];
		switch (op8) {
		case 0xd3: /* shl    %cl,%rsi */
		case 0xc1: /* shl */
		case 0xf7: /* idiv */
			if (!fixup_eflags)
				break;

		case 0x69: /* IMUL */
		case 0x6b: /* IMUL */
			fix_op = op8;
			goto do_fixup;
		default:
			break;
		}


		op16 = ins_buf[off] | (ins_buf[off+1] << 8);
		switch (op16) {
		case 0x6b4c:
		case 0xaf0f: /* imul   0x24(%r14),%edx */
		case 0xc06b:
		case 0xa30f: /* BT */
			if (!fixup_eflags)
				break;
		case 0xbc0f: /* BSF */
		case 0xbd0f: /* BSR */
			fix_op = op16;
			goto do_fixup;
		default:
			break;
		}

		if (has_memlog) {
			switch(op16) {
			case 0xa30f: /* fucking BT writes to stack! */
				fix_op = op16;
				goto do_fixup;
			default:
				break;
			}
		}
	}

	/* couldn't figure out how to fix */
	return false;

do_fixup:
	fprintf(stderr,
		"[VEXLLVM] fixing up op=%p@IP=%p\n",
		(void*)fix_op,
		(void*)fix_addr.o);
	return true;
}

void PTImgAMD64::printFPRegs(
	std::ostream& os,
	user_fpregs_struct& fpregs,
	const VexGuestAMD64State& ref) const
{
	//TODO: some kind of eflags, checking but i don't yet understand this
	//mess of broken apart state.

	//TODO: what is this for? well besides the obvious
	// /* 192 */ULong guest_SSEROUND;

#define XMM_TO_64(x,i,k) (void*)(x[i*4+k] | (((uint64_t)x[i*4+(k+1)]) << 32L))

	for(int i = 0; i < 16; ++i) {
		if (memcmp(
			&fpregs.xmm_space[i * 4],
			&(&ref.guest_XMM0)[i],
			sizeof(ref.guest_XMM0)))
		{
			os << "***";
		}

		os	<< "xmm" << i << ": "
			<< XMM_TO_64(fpregs.xmm_space, i, 2)
			<< "|"
			<< XMM_TO_64(fpregs.xmm_space, i, 0)
			<< std::endl;
	}

	//TODO: check the top pointer of the floating point stack..
	// /* FPU */
	// /* 456 */UInt  guest_FTOP;
	//FPTAG?

	for(int i = 0; i < 8; ++i) {
		int r  = (ref.guest_FTOP + i) & 0x7;
		if (!fcompare(
			&fpregs.st_space[i * 4],
			ref.guest_FPREG[r]))
		{
			os << "***";
		}
		os << "st" << i << ": "
			<< XMM_TO_64(fpregs.st_space, i, 2) << "|"
			<< XMM_TO_64(fpregs.st_space, i, 0) << std::endl;
	}
}

uintptr_t PTImgAMD64::getSysCallResult() const
{
	user_regs_struct	regs;
	getRegs(regs);
	return regs.rax;
}

static inline bool ldeqd(void* ld, long d)
{
	long double* real = (long double*)ld;
	union {
		double d;
		long l;
	} alias;
	alias.d = *real;
	return alias.l == d;
}

static inline bool fcompare(unsigned int* a, long d)
{
	return (*(long*)&d == *(long*)&a[0] &&
		(a[2] == 0 || a[2] == 0xFFFF) &&
		a[3] == 0) ||
		ldeqd(&a[0], d);
}

void PTImgAMD64::printUserRegs(std::ostream& os) const
{
	const uint8_t	*user_regs_ctx;
	const uint8_t	*vex_reg_ctx;
	const VexGuestAMD64State& ref(getVexState());

	user_regs_ctx = (const uint8_t*)&shadow_reg_cache;
	vex_reg_ctx = (const uint8_t*)&ref;

	for (unsigned int i = 0; i < REG_COUNT; i++) {
		uint64_t	user_reg, vex_reg;

		user_reg = get_reg_user(user_regs_ctx, i);
		vex_reg = get_reg_vex(vex_reg_ctx, i);
		if (user_reg != vex_reg) os << "***";

		os	<< user_regs_desc_tab[i].name << ": "
			<< (void*)user_reg << std::endl;
	}

	uint64_t guest_eflags = get_rflags(ref);
	if ((regs.eflags & FLAGS_MASK) != guest_eflags)
		os << "***";
	os << "EFLAGS: " << (void*)(regs.eflags & FLAGS_MASK);
	if ((regs.eflags & FLAGS_MASK) != guest_eflags)
		os << " vs VEX:" << (void*)guest_eflags;
	os << '\n';
}


#define FLAGS_MASK	(0xff | (1 << 10) | (1 << 11))
static uint64_t get_rflags(const VexGuestAMD64State& state)
{
	uint64_t guest_rflags = LibVEX_GuestAMD64_get_rflags(
		&const_cast<VexGuestAMD64State&>(state));
	guest_rflags &= FLAGS_MASK;
	guest_rflags |= (1 << 1);
	return guest_rflags;
}


#define OPCODE_SYSCALL	0x050f

bool PTImgAMD64::isOnSysCall(const user_regs_struct& regs)
{
	long	cur_opcode;
	bool	is_chk_addr_syscall;

	cur_opcode = getInsOp(regs);

	is_chk_addr_syscall = ((cur_opcode & 0xffff) == OPCODE_SYSCALL);
	return is_chk_addr_syscall;
}

bool PTImgAMD64::isOnRDTSC(const user_regs_struct& regs)
{
	long	cur_opcode;
	cur_opcode = getInsOp(regs);
	return (cur_opcode & 0xffff) == 0x310f;
}

bool PTImgAMD64::isOnCPUID(const user_regs_struct& regs)
{
	long	cur_opcode;
	cur_opcode = getInsOp(regs);
	return (cur_opcode & 0xffff) == 0xA20F;
}

bool PTImgAMD64::isPushF(const user_regs_struct& regs)
{
	long	cur_opcode;
	cur_opcode = getInsOp(regs);
	return (cur_opcode & 0xff) == 0x9C;
}

bool PTImgAMD64::doStep(guest_ptr start, guest_ptr end, bool& hit_syscall)
{
	getRegs(regs);

	/* check rip before executing possibly out of bounds instruction*/
	if (regs.rip < start || regs.rip >= end) {
		if(log_steps) {
			std::cerr << "STOPPING: "
				<< (void*)regs.rip << " not in ["
				<< (void*)start.o << ", "
				<< (void*)end.o << "]"
				<< std::endl;
		}
		/* out of bounds, report back, no more stepping */
		return false;
	}

	/* instruction is in-bounds, run it */
	if (log_steps)
		std::cerr << "STEPPING: " << (void*)regs.rip << std::endl;

	if (isOnSysCall(regs)) {
		/* break on syscall */
		hit_syscall = true;
		return false;
	}

	if (isOnRDTSC(regs)) {
		/* fake rdtsc to match vexhelpers.. */
		regs.rip += 2;
		regs.rax = 1;
		regs.rdx = 0;
		setRegs(regs);
		return true;
	}

	if (isPushF(regs)) {
		/* patch out the single step flag for perfect matching..
		   other flags (IF, reserved bit 1) need vex patch */
		waitForSingleStep();

		long old_v, new_v, err;

		old_v = ptrace(
			PTRACE_PEEKDATA,
			child_pid,
			regs.rsp - sizeof(long),
			NULL);

		new_v = old_v & ~0x100;
		err = ptrace(
			PTRACE_POKEDATA,
			child_pid,
			regs.rsp - sizeof(long),
			new_v);
		assert (err != -1 && "Failed to patch pushed flags");
		return true;
	}

	if (isOnCPUID(regs)) {
		/* fake cpuid to match vexhelpers */
		VexGuestAMD64State	fakeState;
		regs.rip += 2;
		fakeState.guest_RAX = regs.rax;
		fakeState.guest_RBX = regs.rbx;
		fakeState.guest_RCX = regs.rcx;
		fakeState.guest_RDX = regs.rdx;
		amd64g_dirtyhelper_CPUID_baseline(&fakeState);
		regs.rax = fakeState.guest_RAX;
		regs.rbx = fakeState.guest_RBX;
		regs.rcx = fakeState.guest_RCX;
		regs.rdx = fakeState.guest_RDX;
		setRegs(regs);
		return true;
	}

	waitForSingleStep();
	return true;
}

bool PTImgAMD64::filterSysCall(
	const VexGuestAMD64State& state,
	user_regs_struct& regs)
{
	switch (regs.rax) {
	case SYS_brk:
		/* hint a better base for sbrk so that we can hang out
		   with out a rochambeau ensuing */
		if(regs.rdi == 0) {
			regs.rdi = 0x800000;
		}
		break;

	case SYS_exit_group:
		regs.rax = state.guest_RAX;
		return true;

	case SYS_getpid:
		regs.rax = getpid();
		return true;

	case SYS_mmap:
		regs.rdi = state.guest_RAX;
		regs.r10 |= MAP_FIXED;
		setRegs(regs);
		return false;
	}

	return false;
}


void PTImgAMD64::stepSysCall(SyscallsMarshalled* sc_m)
{
	user_regs_struct	regs;
	long			old_rdi, old_r10;
	bool			syscall_restore_rdi_r10;
	int			sys_nr;

	getRegs(regs);

	/* do special syscallhandling if we're on an opcode */
	assert (isOnSysCall(regs));

	syscall_restore_rdi_r10 = false;
	old_rdi = regs.rdi;
	old_r10 = regs.r10;

	if (filterSysCall(state, regs)) {
		regs.rip += 2;
		/* kernel clobbers these */
		regs.rcx = regs.r11 = 0;
		setRegs(regs);
		return;
	}

	sys_nr = regs.rax;
	if (sys_nr == SYS_mmap || sys_nr == SYS_brk) {
		syscall_restore_rdi_r10 = true;
	}

	waitForSingleStep();

	getRegs(regs);
	if (syscall_restore_rdi_r10) {
		regs.r10 = old_r10;
		regs.rdi = old_rdi;
	}

	//kernel clobbers these, assuming that the generated code, causes
	regs.rcx = regs.r11 = 0;
	setRegs(regs);

	/* fixup any calls that affect memory */
	if (sc_m->isSyscallMarshalled(sys_nr)) {
		SyscallPtrBuf	*spb = sc_m->takePtrBuf();
		copyIn(spb->getPtr(), spb->getData(), spb->getLength());
		delete spb;
	}
}

void PTImgAMD64::setRegs(const user_regs_struct& regs)
{
	int	err;

	err = ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);
	if(err < 0) {
		perror("PTImgAMD64::setregs");
		exit(1);
	}
}

bool PTImgAMD64::breakpointSysCalls(
	const guest_ptr ip_begin,
	const guest_ptr ip_end)
{
	guest_ptr	rip = ip_begin;
	bool		set_bp = false;

	while (rip != ip_end) {
		if (((getInsOp(rip) & 0xffff) == 0x050f)) {
			gs->setBreakpoint(rip);
			set_bp = true;
		}
		rip.o++;
	}

	return set_bp;
}

guest_ptr PTImgAMD64::undoBreakpoint(void)
{
	struct user_regs_struct	regs;
	int			err;

	/* should be halted on our trapcode. need to set rip prior to
	 * trapcode addr */
	err = ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
	assert (err != -1);

	regs.rip--; /* backtrack before int3 opcode */
	err = ptrace(PTRACE_SETREGS, child_pid, NULL, &regs);

	/* run again w/out reseting BP and you'll end up back here.. */
	return guest_ptr(regs.rip);
}

long PTImgAMD64::setBreakpoint(guest_ptr addr)
{
	uint64_t		old_v, new_v;
	int			err;

	old_v = ptrace(PTRACE_PEEKTEXT, pid, addr.o, NULL);
	new_v = old_v & ~0xff;
	new_v |= 0xcc;

	err = ptrace(PTRACE_POKETEXT, child_pid, addr.o, new_v);
	assert (err != -1 && "Failed to set breakpoint");

	return old_v;
}

void PTImgAMD64::slurpRegisters(void)
{
	AMD64CPUState			*amd64_cpu_state;
	int				err;
	struct user_regs_struct		regs;
	struct user_fpregs_struct	fpregs;

	err = ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
	assert(err != -1);
	err = ptrace(PTRACE_GETFPREGS, child_pid, NULL, &fpregs);
	assert(err != -1);

	amd64_cpu_state = (AMD64CPUState*)gs->getCPUState();

	/* linux is busted, quirks ahead */
	if (regs.fs_base == 0) {
		/* if it's static, it'll probably be using
		 * some native register bullshit for the TLS and it'll read 0.
		 *
		 * Patch this transgression up by allocating some pages
		 * to do the work.
		 * (if N/A, the show goes on as normal)
		 */

		/* Yes, I tried ptrace/ARCH_GET_FS. NO DICE. */
		//err = ptrace(
		//	PTRACE_ARCH_PRCTL, pid, &regs.fs_base, ARCH_GET_FS);
		// fprintf(stderr, "%d %p\n",  err, regs.fs_base);

		int		res;
		guest_ptr	base_addr;

		/* I saw some negative offsets when grabbing errno,
		 * so allow for at least 4KB in either direction */
		res = mem->mmap(
			base_addr,
			guest_ptr(0),
			4096*2,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			-1,
			0);
		assert (res == 0);
		regs.fs_base = base_addr.o + 4096;
	}
	amd64_cpu_state->setRegs(regs, fpregs);
}
