﻿# LAB3 EXERCISE

标签（空格分隔）： UCORE_REPORT

叶雨菲 2013011325 计32

---

练习

对实验报告的要求：

基于markdown格式来完成，以文本方式为主
填写各个基本练习中要求完成的报告内容
完成实验后，请分析ucore_lab中提供的参考答案，并请在实验报告中说明你的实现与参考答案的区别
列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）
列出你认为OS原理中很重要，但在实验中没有对应上的知识点
练习0：填写已有实验

本实验依赖实验1/2。请把你做的实验1/2的代码填入本实验中代码中有“LAB1”,“LAB2”的注释相应部分。

> done

###练习1：给未被映射的地址映射上物理页（需要编程）

完成do_pgfault（mm/vmm.c）函数，给未被映射的地址映射上物理页。设置访问权限 的时候需要参考页面所在 VMA 的权限，同时需要注意映射物理页时需要操作内存控制 结构所指定的页表，而不是内核的页表。注意：在LAB3 EXERCISE 1处填写代码。执行

make　qemu
后，如果通过check_pgfault函数的测试后，会有“check_pgfault() succeeded!”的输出，表示练习1基本正确。

请在实验报告中简要说明你的设计实现过程。请回答如下问题：

1. 请描述页目录项（Pag Director Entry）和页表（Page Table Entry）中组成部分对ucore实现页替换算法的潜在用处。
2. 如果ucore的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？

**实现**

```
    ptep = get_pte(mm->pgdir, addr, 1);           //(1) try to find a pte, if pte's PT(Page Table) isn't existed, then create a PT.
    if (ptep == NULL) {
    	cprintf("fail to get pte\n");
    	goto failed;
    }
    if (*ptep == 0) {
                            //(2) if the phy addr isn't exist, then alloc a page & map the phy addr with logical addr
    	if (pgdir_alloc_page(mm->pgdir, addr, perm) == NULL) {
    		cprintf("fail to alloc page\n");
    		goto failed;
    	}
    }
```

**1.1**
当一个PTE用来描述一般意义上的物理页时，显然它应该维护各种权限和映射关系，以及应该有PTE_ P标记；但当它用来描述一个被置换出去的物理页时，它被用来维护该物理页与 swap 磁盘上扇区的映射关系，并且该 PTE 不应该由 MMU 将它解释成物理页映射(即没有 PTE_ P 标记)，与此同时对应的权限则交由 mm_struct 来维护，当对位于该页的内存地址进行访问的时候，必然导致 page fault，然后ucore能够根据 PTE 描述的 swap 项将相应的物理页重新建立起来，并根据虚存所描述的权限重新设置好 PTE 使得内存访问能够继续正常进行。该PTE的最低位--present位应该为0 （即 PTE _P 标记为空，表示虚实地址映射关系不存在），接下来的7位暂时保留，可以用作各种扩展；而原来用来表示页帧号的高24位地址，恰好可以用来表示此页在硬盘上的起始扇区的位置（其从第几个扇区开始）。为了在页表项中区别 0 和 swap 分区的映射，将 swap 分区的一个 page 空出来不用，也就是说一个高24位不为0，而最低位为0的PTE表示了一个放在硬盘上的页的起始扇区号。

**1.2**
产生页访问异常后，CPU硬件和软件都会做一些事情来应对此事。首先页访问异常也是一种异常，所以针对一般异常的硬件处理操作是必须要做的，即CPU在当前内核栈保存当前被打断的程序现场，即依次压入当前被打断程序使用的EFLAGS，CS，EIP，errorCode；由于页访问异常的中断号是0xE，CPU把异常中断号0xE对应的中断服务例程的地址（vectors.S中的标号vector14处）加载到CS和EIP寄存器中，开始执行中断服务例程。

###练习2：补充完成基于FIFO的页面替换算法（需要编程）

完成vmm.c中的do_pgfault函数，并且在实现FIFO算法的swap_fifo.c中完成map_swappable和swap_out_vistim函数。通过对swap的测试。注意：在LAB2 EXERCISE 2处填写代码。执行

make　qemu
后，如果通过check_swap函数的测试后，会有“check_swap() succeeded!”的输出，表示练习2基本正确。

请在实验报告中简要说明你的设计实现过程。

请在实验报告中回答如下问题：

1. 如果要在ucore上实现"extended clock页替换算法"请给你的设计方案，现有的swap_manager框架是否足以支持在ucore中实现此算法？如果是，请给你的设计方案。如果不是，请给出你的新的扩展和基此扩展的设计方案。并需要回答如下问题
1.1 需要被换出的页的特征是什么？
1.2 在ucore中如何判断具有这样特征的页？
1.3 何时进行换入和换出操作？

设计：
使用循环列表，并且加入两个标志位PTE_A | PTE_W, 前者表示该页最近是否被访问，后者表示该页最近是否被改写。维护一个指针，当发生缺页异常的时候，从指针位置开始查起，访问位是1，改成0，如果改写为是1，则写回到disk中，然后改写成0。遇到的第一个00的页，就是要换出的页。

**2.1.1**
被换出的页的特征：指针经过的第一个not dirty 且 not access的页
**2.1.2**
如何判断具有这样特征的页：在页表的格式中，在PTE_P, PTE_W等之后，加两个bit标志位，一个表示近期被访问，一个表示被改写。
**2.1.3**
换出操作：当发生缺页的时候，且指针经过00时，换出该页；或者，当方向该页被访问过时，会向磁盘中写回该页
换入操作：当发生缺页时，找到应该符合2.1.1中特征的页时，执行换入操作
