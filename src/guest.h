#ifndef GUEST_H
#define GUEST_H

#include <iostream>
#include <vector>
#include <stdint.h>
#include <list>
#include "arch.h"
#include "guestmem.h"
#include "syscall/syscallparams.h"

class GuestCPUState;
class GuestSnapshot;
class GuestPTImg;
class GuestABI;
class Symbols;

/* ties together all state information for guest.
 * 1. register state
 * 2. memory mappings
 * 3. syscall param/result access
 *
 * This is basically an anemic operating system process structure.
 */
class Guest
{
public:
	virtual ~Guest();
	virtual guest_ptr getEntryPoint(void) const = 0;
	std::list<GuestMem::Mapping> getMemoryMap(void) const;

	const GuestCPUState* getCPUState(void) const { return cpu_state; }
	GuestCPUState* getCPUState(void) { return cpu_state; }

	SyscallParams getSyscallParams(void) const;
	void setSyscallResult(uint64_t ret);

	virtual std::string getName(guest_ptr) const;
	virtual const Symbols* getSymbols(void) const { return NULL; }
	virtual Symbols* getSymbols(void) { return NULL; }
	virtual const Symbols* getDynSymbols(void) const { return NULL; }
	void addLibrarySyms(const char* path, guest_ptr base);
	static void addLibrarySyms(const char* path, guest_ptr base, Symbols* s);

	uint64_t getExitCode(void) const;
	void print(std::ostream& os) const;
	virtual Arch::Arch getArch() const = 0;

	const char* getBinaryPath(void) const { return bin_path; }
	const GuestMem* getMem(void) const { assert (mem != NULL); return mem; }
	GuestMem* getMem(void) { assert (mem != NULL); return mem; }

	/* this is for swapping out the memory layer with
	 * a ptrace memory layer, mainly */
	void setMem(GuestMem* in_mem) { mem = in_mem; }

	/* guest has an interface to saving/loading, but all the legwork
	 * is done by guestsnapshot to keep things tidy */
	void save(const char* dirpath = NULL) const;
	static Guest* load(const char* dirpath = NULL);

	/* saves the guest snapshot to a 'core' file which
	 * is meant to be loadable by the *guest* platform's debugger */
	void toCore(const char* path = NULL) const;

	/* This might have to eventually morph into some 'get parameters'
	 * type function. Since we just do executables right now, limit
	 * the interface to argv's */
	virtual std::vector<guest_ptr> getArgvPtrs(void) const
	{ return std::vector<guest_ptr>(); }

	const GuestABI* getABI(void) const { return abi; }
protected:
	friend class GuestPTImg;
	void setBinPath(const char* b);
	Guest(const char* bin_path);

	GuestCPUState	*cpu_state;
	GuestMem	*mem;
	char		*bin_path;
	GuestABI	*abi;
};

#endif