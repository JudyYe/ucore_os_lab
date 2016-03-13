#lab1 report

叶雨菲 2013011325 计32
---

##练习1：理解通过make生成执行文件的过程。（要求在报告中写出对下述问题的回答）

列出本实验各练习中对应的OS原理的知识点，并说明本实验中的实现部分如何对应和体现了原理中的基本概念和关键知识点。

在此练习中，大家需要通过静态分析代码来了解：

1.    操作系统镜像文件ucore.img是如何一步一步生成的？(需要比较详细地解释Makefile中每一条相关命令和命令参数的含义，以及说明命令导致的结果)
2.    一个被系统认为是符合规范的硬盘主引导扇区的特征是什么？
    
 
**1.1**
> 1. 操作系统镜像文件ucore.img是如何一步一步生成的

STEP1 首先在makefile 中找到create ucore.img的代码
```
UCOREIMG	:= $(call totarget,ucore.img)

$(UCOREIMG): $(kernel) $(bootblock)
	$(V)dd if=/dev/zero of=$@ count=10000
	$(V)dd if=$(bootblock) of=$@ conv=notrunc
	$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc

$(call create_target,ucore.img)
```
 
STEP2 发现ucore img依赖bootblock 和kernel两个模块
```create kernel```

```create bootblock```阅读makefile，大致流程是：把booting需要用到的代码放到0x7c00处，输出中间的汇编结果。

使用make V=命令，观察每一步make命令
```
+ cc kern/init/init.c  //编译 init.c
+ cc kern/libs/readline.c //编译 readline.c
+ cc kern/libs/stdio.c //编译 stdio.c
+ cc kern/debug/kdebug.c//编译 kdebug.c 
+ cc kern/debug/kmonitor.c //编译 kmonitor
+ cc kern/debug/panic.c//编译 panic.c
+ cc kern/driver/clock.c //编译 clock.c
+ cc kern/driver/console.c //编译 console.c
+ cc kern/driver/intr.c//编译 intr.c
+ cc kern/driver/picirq.c //编译 picirq.c
+ cc kern/trap/trap.c //编译 trap.c
+ cc kern/trap/trapentry.S //编译 trapentry.S
+ cc kern/trap/vectors.S //编译 vector.S
+ cc kern/mm/pmm.c//编译 pmm.c
+ cc libs/printfmt.c // printgmt.c
+ cc libs/string.c //编译 string.c
``` 
参数意义

- -Iboot/ -Ilibs/: Include 目录boot/, libs/ 
- -fno-builtin: 禁止使用gcc的built in函数进行优化。
- -fno-stack-protector: 不产生多余的代码来检查栈溢出。
- -Wall: 打开所有警告。 -ggdb: 添加供gdb调试的调试信息。
- -m32: 产生32位代码。
- -nostdinc:不检查系统默认的目录以获取头文件。
- -Os:为了目标文件大小而优化。有利于生成较小的目标文件。
- -c boot/bootasm.S: 指定源文件名。
- -o obj/bootasm.o: 指定目标文件名。

```
+ ld bin/kernel//接下来用ld合并目标文件(object) 和 库文件(archive),生成kernel程序
+ cc boot/bootasm.S //编译 bootasm.S
+ cc boot/bootmain.c //编译 bootmain.c
+ cc tools/sign.c //编译 sign.c
+ ld bin/bootblock//接下来连接源文件与目标文件，生成bootblock程序
+ //最后将bootloader放入虚拟硬盘ucore.img中去。
dd if=/dev/zero of=bin/ucore.img count=10000 // 生成一个大小为5.1MB的空白磁盘镜像。
dd if=bin/bootblock of=bin/ucore.img conv=notrunc //将编译好的bootblock写进刚才生成的磁盘镜像中，conv=notrunc表示若 bootblock大小小于ucore.img则不截断ucore.img。 这样一个bootloader就做好了。
dd if=bin/kernel of=bin/ucore.img seek=1 conv=notrunc
```
**1.2**
查看```tools/sign.c```函数，用于生成符合规范的硬盘主引导扇区，阅读代码，发现要求：
>1. bootloader代码量不能超过510B
>2. 结尾的511，512B是55AA

##练习2：使用qemu执行并调试lab1中的软件。（要求在报告中简要写出练习过程）

为了熟悉使用qemu和gdb进行的调试工作，我们进行如下的小练习：

1. 从CPU加电后执行的第一条指令开始，单步跟踪BIOS的执行。
2. 在初始化位置0x7c00设置实地址断点,测试断点正常。
3. 从0x7c00开始跟踪代码运行,将单步跟踪反汇编得到的代码与bootasm.S和 bootblock.asm进行比较。
4. 自己找一个bootloader或内核中的代码位置，设置断点并进行测试。

**2.1**从加电后执行的第一条指令开始，单步调试
输入命令```make lab1-mon```，或者将gdbinit改成
```
set architecture i8086
target remote :1234
```
调试运行，看到有
```
0x0000fff0 in ?? ()
warning: A handler for the OS ABI "GNU/Linux" is not built into this configuration
of GDB.  Attempting to continue with the default i8086 settings.
```

ucore找不到“GNU/LINUX”的设置，接着尝试从i8086的设置启动，即尝试读入 0x7c00处的地址，即ucore加电后运行的第一条指令，使用ni跟踪
```
   0x7c00:	cli    
   0x7c01:	cld    
=> 0x7c02:	xor    %ax,%ax
   0x7c04:	mov    %ax,%ds
   0x7c06:	mov    %ax,%es
   0x7c08:	mov    %ax,%ss
   0x7c0a:	in     $0x64,%al
   0x7c0c:	test   $0x2,%al
   0x7c0e:	jne    0x7c0a
   0x7c10:	mov    $0xd1,%al
```
**2.2**设置实地址断点
在gdb中输入```b *0x7c10```然后continue，发现程序在实地址 ```0x7c10```处停了下来，断点正常

**2.3** bootasm.S和 bootblock.asm进行比较
不同点：
- bootblock.asm中有标注w，b等表示地址长度的标记
- bootblock.asm中只有代码段的符号，具体地址位置没有填到代码中，比如```jne    0x7c14```与```jnz seta20.1```

**2.4**见2.2

##练习3：分析bootloader进入保护模式的过程。（要求在报告中写出分析）

BIOS将通过读取硬盘主引导扇区到内存，并转跳到对应内存中的位置执行bootloader。请分析bootloader是如何完成从实模式进入保护模式的。

1. 为何开启A20，以及如何开启A20
2. 如何初始化GDT表
3. 如何使能和进入保护模式

**3.1**    为了与最早期的PC向下兼容。早期PC只有20位地址空间，当高于1MB(20bits)的时候，会自动从0开始。开启A20后不会抹掉高20位。
依据，有注释
```
    #  For backwards compatibility with the earliest PCs, physical
    #  address line 20 is tied low, so that addresses higher than
    #  1MB wrap around to zero by default. This code undoes this.
```

开启A20的流程
```
1. 监听8042端口，直到设备空闲。
2. 向8042端口发送写命令
3. 向8042端口发送A20开启的命令字。(0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1)
```

**3.2**GDT表
把GDT的入口地址load到gdtr中。
```
lgdt gdtdesc
```

**3.3**
进入保护模式：设置cr0 = 1，并跳转到代码段，重新设置段寄存器。
```
    movl %cr0, %eax
    orl $CR0_PE_ON, %eax
    movl %eax, %cr0
```
> 使能？？？

##练习4：分析bootloader加载ELF格式的OS的过程。（要求在报告中写出分析）

通过阅读bootmain.c，了解bootloader如何加载ELF文件。通过分析源代码和通过qemu来运行并调试bootloader&OS，

1. bootloader如何读取硬盘扇区的？
2. bootloader是如何加载ELF格式的OS？

**4.1**读一个扇区的流程（boot/bootmain.c中的readsect函数实现）大致如下：

1. 等待磁盘准备好
2. 发出读取扇区的命令
3. 等待磁盘准备好
4. 把磁盘扇区数据读到指定内存

**4.2**
在bootmain.c中，找到读取os的代码
```
    // read the 1st page off disk
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);

    // is this a valid ELF?
    if (ELFHDR->e_magic != ELF_MAGIC) {
        goto bad;
    }

    struct proghdr *ph, *eph;

    // load each program segment (ignores ph flags)
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    eph = ph + ELFHDR->e_phnum;
    for (; ph < eph; ph ++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }
```
根据```elfhdr```和```proghdr```的结构描述，先按照elf header的格式，读取程序段数，再把每段程序一次加载进内存的指定位置。

##练习5：实现函数调用堆栈跟踪函数 （需要编程）
我们需要在lab1中完成kdebug.c中函数print_stackframe的实现.

请完成实验，看看输出是否与上述显示大致一致，并解释最后一行各个数值的含义。

提示：可阅读小节“函数堆栈”，了解编译器如何建立函数调用关系的。在完成lab1编译后，查看lab1/obj/bootblock.asm，了解bootloader源码与机器码的语句和地址等的对应关系；查看lab1/obj/kernel.asm，了解 ucore OS源码与机器码的语句和地址等的对应关系。

要求完成函数kern/debug/kdebug.c::print_stackframe的实现，提交改进后源代码包（可以编译执行），并在实验报告中简要说明实现过程，并写出对上述问题的回答。


**实现过程**
在print_stackframe中已经有了伪代码，只需要实现即可。实验过程中，比较困扰的是指针问题，有些绕。但是想到*uint32_t和uint32_t的区别只是寻址方式不一样，而且可以通过
```
    *(uint32_t*)some_int
```
来强制转换，其余的就容易多了。

##练习6：完善中断初始化和处理 （需要编程）

请完成编码工作和回答如下问题：

1. 中断描述符表（也可简称为保护模式下的中断向量表）中一个表项占多少字节？其中哪几位代表中断处理代码的入口？
2. 请编程完善kern/trap/trap.c中对中断向量表进行初始化的函数idt_init。在idt_init函数中，依次对所有中断入口进行初始化。使用mmu.h中的SETGATE宏，填充idt数组内容。每个中断的入口由tools/vectors.c生成，使用trap.c中声明的vectors数组即可。
3. 请编程完善trap.c中的中断处理函数trap，在对时钟中断进行处理的部分填写trap函数中处理时钟中断的部分，使操作系统每遇到100次时钟中断后，调用print_ticks子程序，向屏幕上打印一行文字”100 ticks”。

**6.1**
IDT一个表项占8字节，中断处理代码入口在selector.base + offset, offset存在0-15位，selector存在16-31位。

**6.2**见代码
