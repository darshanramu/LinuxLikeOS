Documentation for Kernel Assignment 2
=====================================

+-------------+
| BUILD & RUN |
+-------------+

Comments: 
1. Use make command to compile the kernel
2. Command to run: ./weenix -n

+-----------------+
| SKIP (Optional) |
+-----------------+

Is there are any tests in the standard test suite that you know that it's not
working and you don't want the grader to run it at all so you won't get extra
deductions, please list them here.  (Of course, if the grader won't run these
tests, you will not get plus points for them.)

+---------+
| GRADING |
+---------+

(A.1) In fs/vnode.c:
    (a) In special_file_read(): 6 out of 6 pts
    (b) In special_file_write(): 6 out of 6 pts

(A.2) In fs/namev.c:
    (a) In lookup(): 6 out of 6 pts
    (b) In dir_namev(): 10 out of 10 pts
    (c) In open_namev(): 2 out of 2 pts

(3) In fs/vfs_syscall.c:
    (a) In do_write(): 6 out of 6 pts
    (b) In do_mknod(): 2 out of 2 pts
    (c) In do_mkdir(): 2 out of 2 pts
    (d) In do_rmdir(): 2 out of 2 pts
    (e) In do_unlink(): 2 out of 2 pts
    (f) In do_stat(): 2 out of 2 pts

(B) vfstest: 39 out of 39 pts
    Comments: Command to run in kshell: vfs_test 

(C.1) faber_fs_thread_test (3 out of 3 pts)
    Comments: Command to run in kshell: fs_thread_test 
    
	
(C.2) faber_directory_test (2 out of 2 pts)
    Comments: Command to run in kshell: dir_test 

(D) Self-checks: (10 out of 10 pts)
    Comments: By running all the tests in the above sections all the code paths are excercised.
    Also lines with dbg(DBG_PRINT,"(GRADING2D)\n"); will be excercised when kernel starts up and kshell comes.		

Missing required section(s) in README file (vfs-README.txt): None 
Submitted binary file : No 
Submitted extra (unmodified) file : No 
Wrong file location in submission : No 
Use dbg_print(...) instead of dbg(DBG_PRINT, ...) : No 
Not properly indentify which dbg() printout is for which item in the grading guidelines : Properly identified 
Cannot compile : Can compile 
Compiler warnings : None 
"make clean" : cleans up required files 
Useless KASSERT : None 
Insufficient/Confusing dbg : None 
Kernel panic : No 
Cannot halt kernel cleanly : halts cleanly 

+------+
| BUGS |
+------+

Comments: none identified 

+---------------------------+
| CONTRIBUTION FROM MEMBERS |
+---------------------------+

Equal share

+------------------+
| OTHER (Optional) |
+------------------+

Special DBG setting in Config.mk for certain tests: No 
Comments on deviation from spec (you will still lose points, but it's better to let the grader know): No deviation 
General comments on design decisions: None 

