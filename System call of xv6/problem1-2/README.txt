@Huawei Wang 09/13/2017
Authorship is closely linked to work
1.Add functionality of sys_getppid() in /kernel/sysproc.c
2.register the function in /user/usys.S, /user/user.h in order to let the gcc know where is the function
3.assign the syscall macro number in /include/syscall.h
4.add array of function pointers in /kernel/syscall.c
5.write testgetppid.c for testing.
