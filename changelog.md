# log

## user

user.h usys.pl 常见系统调用添加

## kernel

trap.c 75 添加缺页的文件写入

sysfile.c 489 两个核心系统调用的写入

syscall.h syscall.c 两个核心系统调用的声明

proc.h 添加VMA进入内存的结构体

proc.c 修改exit 和 fork 以及一开始的alloc

memlayout.h 添加最大VMA地址 MAXMMAP

file.c 183 为了偏移的写入，这里稍微改了一下filewrite

defs.h 添加了一些hanshu1

fcntl.h 为了不报错，把选择编译给噶掉了

