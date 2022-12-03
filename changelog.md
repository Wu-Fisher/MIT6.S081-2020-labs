# changelog

## kernel

### sysproc.c

更改了sys_sbrk 增加的时候只是简单的增加sz，并没有实际增加物理有效页，但是减少的时候确实实际减少映射

### trap.c

当trap类型为13或者15的时候认为是缺页异常，调用lazyalloc函数，增加实际物理有效页
### vm.c

lazyalloc写在这里，做一些合法判断之后为其分配页面

其中有很多其他函数进行了修改：

walkaddar 内核态系统调用的时候多用这个函数，所以这里如果发现为空或者无效的话，需要先给他分配一下，然后在继续执行

uvmunmap，uvmcopy的时候如果发现空的则continue就好，因为此时不会保证一定有实际有效映射，并且不会影响正确性。