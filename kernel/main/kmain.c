/******************************************************************************/
/* Important Spring 2015 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "kernel.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "errno.h"

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

static void hard_shutdown(void);
static void *bootstrap(int arg1, void *arg2);
static void *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void *initproc_run(int arg1, void *arg2);
extern void vfs_flush_vnodes();
static context_t bootstrap_context;
static int gdb_wait = GDBWAIT;
/**
 * This is the first real C function ever called. It performs a lot of
 * hardware-specific initialization, then creates a pseudo-context to
 * execute the bootstrap function in.
 */
void kmain() {
	GDB_CALL_HOOK(boot);

	dbg_init();
	dbgq(DBG_CORE, "Kernel binary:\n");
	dbgq(DBG_CORE, "  text: 0x%p-0x%p\n", &kernel_start_text, &kernel_end_text);
	dbgq(DBG_CORE, "  data: 0x%p-0x%p\n", &kernel_start_data, &kernel_end_data);
	dbgq(DBG_CORE, "  bss:  0x%p-0x%p\n", &kernel_start_bss, &kernel_end_bss);

	page_init();

	pt_init();
	slab_init();
	pframe_init();

	acpi_init();
	apic_init();
	pci_init();
	intr_init();

	gdt_init();

	/* initialize slab allocators */
#ifdef __VM__
	anon_init();
	shadow_init();
#endif
	vmmap_init();
	proc_init();
	kthread_init();

#ifdef __DRIVERS__
	bytedev_init();
	blockdev_init();
#endif

	void *bstack = page_alloc();
	pagedir_t *bpdir = pt_get();
	KASSERT(NULL != bstack && "Ran out of memory while booting.");
	/* This little loop gives gdb a place to synch up with weenix.  In the
	 * past the weenix command started qemu was started with -S which
	 * allowed gdb to connect and start before the boot loader ran, but
	 * since then a bug has appeared where breakpoints fail if gdb connects
	 * before the boot loader runs.  See
	 *
	 * https://bugs.launchpad.net/qemu/+bug/526653
	 *
	 * This loop (along with an additional command in init.gdb setting
	 * gdb_wait to 0) sticks weenix at a known place so gdb can join a
	 * running weenix, set gdb_wait to zero  and catch the breakpoint in
	 * bootstrap below.  See Config.mk for how to set GDBWAIT correctly.
	 *
	 * DANGER: if GDBWAIT != 0, and gdb is not running, this loop will never
	 * exit and weenix will not run.  Make SURE the GDBWAIT is set the way
	 * you expect.
	 */
	while (gdb_wait)
		;
	context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE,
			bpdir);
	context_make_active(&bootstrap_context);

	panic("\nReturned to kmain()!!!\n");
}

/**
 * Clears all interrupts and halts, meaning that we will never run
 * again.
 */
static void hard_shutdown() {
#ifdef __DRIVERS__
	vt_print_shutdown();
#endif
	__asm__ volatile("cli; hlt");
}

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
bootstrap(int arg1, void *arg2) {
	/* necessary to finalize page table information */
	pt_template_init();

	/* NOT_YET_IMPLEMENTED("PROCS: bootstrap"); */
	pid_t child;
	int status;

	proc_t *cur_pcb = proc_create("idle_proc");
	KASSERT(cur_pcb != NULL);
	dbg(DBG_PRINT, "(GRADING1A 1.a)\n");

	KASSERT(cur_pcb->p_pid == PID_IDLE);
	dbg(DBG_PRINT, "(GRADING1A 1.a)\n");

	curproc = cur_pcb;
	kthread_t *idlthr = kthread_create(cur_pcb, idleproc_run, 0, NULL);
	KASSERT(idlthr != NULL);
	dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
	curthr = idlthr;

	context_make_active(&idlthr->kt_ctx);

	panic("weenix returned to bootstrap()!!! BAD!!!\n");
	return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
idleproc_run(int arg1, void *arg2) {
	int status;
	pid_t child;

	/* create init proc */
	kthread_t *initthr = initproc_create();
	init_call_all();
	GDB_CALL_HOOK(initialized);

	/* Create other kernel threads (in order) */

#ifdef __VFS__
	/* Once you have VFS remember to set the current working directory
	 * of the idle and init processes */
	/* NOT_YET_IMPLEMENTED("VFS: idleproc_run"); */
	dbg(DBG_PRINT,"(GRADING2D)\n");
	curproc->p_cwd = vfs_root_vn;
	vref(vfs_root_vn);
	if(initthr!=NULL)
	{
		dbg(DBG_PRINT,"(GRADING2D)\n");
		initthr->kt_proc->p_cwd = vfs_root_vn;
		vref(vfs_root_vn);
	}
	/* Here you need to make the null, zero, and tty devices using mknod */
	/* You can't do this until you have VFS, check the include/drivers/dev.h
	 * file for macros with the device ID's you will need to pass to mknod */
	/* NOT_YET_IMPLEMENTED("VFS: idleproc_run"); */
	do_mkdir("/dev");

	do_mknod("/dev/null",S_IFCHR,MKDEVID(1, 0));
	do_mknod("/dev/zero",S_IFCHR,MKDEVID(1, 1));
	do_mknod("/dev/tty0",S_IFCHR,MKDEVID(2, 0));
	do_mknod("/dev/tty1",S_IFCHR,MKDEVID(2, 1));
#endif

	/* Finally, enable interrupts (we want to make sure interrupts
	 * are enabled AFTER all drivers are initialized) */
	intr_enable();

	/* Run initproc */
	sched_make_runnable(initthr);
	/* Now wait for it */
	child = do_waitpid(-1, 0, &status);
	KASSERT(PID_INIT == child);

#ifdef __MTP__
	kthread_reapd_shutdown();
#endif

#ifdef __SHADOWD__
	/* wait for shadowd to shutdown */
	shadowd_shutdown();
#endif

#ifdef __VFS__
	/* Shutdown the vfs: */
	dbg_print("weenix: vfs shutdown...\n");
	do_unlink("/dev/null");
	do_unlink("/dev/zero");
	do_unlink("/dev/tty1");
	do_unlink("/dev/tty0");
	do_rmdir("/dev");
	vput(curproc->p_cwd);
	vfs_flush_vnodes();
	if (vfs_shutdown())
	panic("vfs shutdown FAILED!!\n");

#endif

	/* Shutdown the pframe system */
#ifdef __S5FS__
	pframe_shutdown();
#endif

	dbg_print("\nweenix: halted cleanly!\n");
	GDB_CALL_HOOK(shutdown);
	hard_shutdown();
	return NULL;
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
static kthread_t *
initproc_create(void) {
	/* NOT_YET_IMPLEMENTED("PROCS: initproc_create"); */
	proc_t *init_pcb;
	init_pcb = proc_create("init_proc");
	KASSERT(init_pcb != NULL);
	dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

	KASSERT(init_pcb->p_pid == PID_INIT);
	dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

	kthread_t *init_thr;
	init_thr = kthread_create(init_pcb, initproc_run, 0, NULL);

	KASSERT(init_thr != NULL);
	dbg(DBG_PRINT, "(GRADING1A 1.b)\n");

	return init_thr;
}

extern void* faber_thread_test(int, int);
extern void* sunghan_deadlock_test(int, int);
extern void* sunghan_test(int, int);
extern int vfstest_main(int, char**);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);

int call_faber(kshell_t *kshell, int argc, char **argv) {
	dbg(DBG_PRINT, "(GRADING1B)\n");
	proc_t *fabpcb = proc_create("Faber Test");
	kthread_t *fabtcb = kthread_create(fabpcb,
			(kthread_func_t) faber_thread_test, 1, 0);
	sched_make_runnable(fabtcb);
	int status;
	do_waitpid(fabtcb->kt_proc->p_pid, 0, &status);
	dbg(DBG_PRINT, "(GRADING1C)\n");
	return 0;
}

int call_sunghan_deadlock_test(kshell_t *kshell, int argc, char **argv) {
	dbg(DBG_PRINT, "(GRADING1B)\n");
	proc_t *fabpcb = proc_create("Sunghan Deadlock Test");
	kthread_t *fabtcb = kthread_create(fabpcb,
			(kthread_func_t) sunghan_deadlock_test, 1, 0);
	sched_make_runnable(fabtcb);
	int status;
	do_waitpid(fabtcb->kt_proc->p_pid, 0, &status);
	dbg(DBG_PRINT, "(GRADING1D)\n");
	return 0;
}

int call_sunghan_test(kshell_t *kshell, int argc, char **argv) {
	dbg(DBG_PRINT, "(GRADING1B)\n");
	proc_t *fabpcb = proc_create("Sunghan Test");
	kthread_t *fabtcb = kthread_create(fabpcb, (kthread_func_t) sunghan_test, 1,
			0);
	sched_make_runnable(fabtcb);
	int status;
	do_waitpid(fabtcb->kt_proc->p_pid, 0, &status);
	dbg(DBG_PRINT, "(GRADING1D)\n");
	return 0;
}

int call_vfstest_main(kshell_t *kshell, int argc, char **argv) {
	dbg(DBG_PRINT, "(GRADING2B) start vfs test\n");
	proc_t *vfspcb = proc_create("VFS Test");
	kthread_t *vfstcb = kthread_create(vfspcb, (kthread_func_t) vfstest_main, 1,
			0);
	sched_make_runnable(vfstcb);
	int status;
	do_waitpid(vfstcb->kt_proc->p_pid, 0, &status);
	dbg(DBG_PRINT, "(GRADING2B) done with vfs test\n");
	return 0;
}

void* create_kshell() {
	kshell_t *new_shell;
	int i;
	kshell_add_command("faber_test", call_faber, "invoke faber test");
	kshell_add_command("sunghan_test", call_sunghan_test,
			"invoke sunghan test");
	kshell_add_command("deadlock_test", call_sunghan_deadlock_test,
			"invoke sunghan deadlock test");
	kshell_add_command("vfs_test", call_vfstest_main, "invoke VFS test");
	kshell_add_command("fs_thread_test", faber_fs_thread_test,
			"invoke faber fs thread test");
	kshell_add_command("dir_test", faber_directory_test,
			"invoke faber directory test");

	dbg(DBG_PRINT, "(GRADING1B)\n");
	new_shell = kshell_create(0);
	while (1) {
		i = kshell_execute_next(new_shell);
		dbg(DBG_PRINT, "(GRADING1B)\n");
		if (i <= 0) {
			dbg(DBG_PRINT, "(GRADING1B)\n");
			break;
		}
	}
	kshell_destroy(new_shell);
	/*
	 proc_t *child;
	 if (!list_empty(&curproc->p_children)) {
	 dbg(DBG_PRINT, "Displaying children of pid=%d\n",curproc->p_pid);
	 list_iterate_begin(&curproc->p_children, child, proc_t, p_child_link)
	 {
	 dbg(DBG_PRINT, "child pid=%d \n",child->p_pid);

	 }list_iterate_end();
	 }
	 else
	 dbg(DBG_PRINT,"No children exists");
	 */
	return NULL;
}

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
initproc_run(int arg1, void *arg2) {
	/* NOT_YET_IMPLEMENTED("PROCS: initproc_run"); */
	dbg(DBG_PRINT, "(GRADING1B)\n");
	/*create_kshell();*/
	char *argv[] = { "init", NULL };
	char *envp[] = { NULL };
	/*kernel_execve("/usr/bin/hello", argv, envp);*/
	/*kernel_execve("/usr/bin/fork-and-wait", argv, envp);*/
	/*char *argv[]={"args","ab","cde","fghi","j",NULL};*/
	/*kernel_execve("/usr/bin/args", argv, envp);*/
	/*char *argv[]={"uname","-a",NULL};*/
	/*Uname Works after open of tty
	 * do_close(0);
	 do_close(1);

	 do_open("/dev/tty1", O_RDONLY);
	 do_open("/dev/tty1", O_WRONLY);*/
	/*kernel_execve("/bin/uname", argv, envp);*/
	/*kernel_execve("/usr/bin/kshell", argv, envp);*/
	dbg(DBG_PRINT, "(GRADING3F)\n");
	kernel_execve("/sbin/init", argv, envp);
	return NULL;
}
