int myfunc(int arg) {
    printf("hello:arguments is %d\n",arg);
    int pid=getpid();
    printf("pid:%d\n",pid);
}

int main()
{
    myfunc(0);
    exit();
}