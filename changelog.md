# changelog

## kernel

### defs.h

加了一些函数方便使用

### riscv.h

加入了cow标识位

### trap.c

70: 当遇到 页面读fault的时候 调用cowcopy，将虚拟地址找到的物理地址复制一份给当前的va

### vm.c

copyout 里面需要重新分配物理页，调用完cowcopy之后，记得重新walk一下，找到新的物理页

mappages 的时候注释panic，因为会重映射

uvmcopy 不要来新的物理页分配，将原来的pte和flags修改后，直接新的va重新映射到原来的物理地址即可（PTE_COW 和 PTE_W）

### kalloc.c

13: 加入了了refcnt数据结构，包含了(PHYSTOP-KERNBASE)/PGSIZE 个计数器和锁

79：kfree的时候，先看引用数是否>1,如果是，引用数-1就好。如果引用数为1，则需要正常释放物理页，并且改引用为0

113: kalloc 当r不为空的时候引用数改为1

131：cowcopy 核心复制函数，如果va的pa pte为cow

如果引用数大于1则需要复制一个新的页面，并且引用到该虚拟地址上

如果引用数就是1，则去掉cow和加上w标志位，作为一个普通的可写页面返回

这里考虑使用的页表，就加了一个页表参数hhh