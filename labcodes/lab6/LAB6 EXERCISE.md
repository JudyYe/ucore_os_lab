# LAB6 EXERCISE

标签（空格分隔）： UCORE_REPORT

---

###练习0：填写已有实验

本实验依赖实验1/2/3/4/5。请把你做的实验2/3/4/5的代码填入本实验中代码中有“LAB1”/“LAB2”/“LAB3”/“LAB4”“LAB5”的注释相应部分。并确保编译通过。注意：为了能够正确执行lab6的测试应用程序，可能需对已完成的实验1/2/3/4/5的代码进行进一步改进。

> done

###练习1: 使用 Round Robin 调度算法（不需要编码）

完成练习0后，建议大家比较一下（可用kdiff3等文件比较软件）个人完成的lab5和练习0完成后的刚修改的lab6之间的区别，分析了解lab6采用RR调度算法后的执行过程。执行make grade，大部分测试用例应该通过。但执行priority.c应该过不去。

请在实验报告中完成：

- 请理解并分析sched_calss中各个函数指针的用法，并接合Round Robin 调度算法描ucore的调度执行过程
- 请在实验报告中简要说明如何设计实现”多级反馈队列调度算法“，给出概要设计，鼓励给出详细设计

> 让所有runnable态的进程分时轮流使用CPU时间。RR调度器维护当前runnable进程的有序运行队列。当前进程的时间片用完之后，调度器将当前进程放置到运行队列的尾部，再从其头部取出进程进行调度。RR调度算法的就绪队列在组织结构上也是一个双向链表，只是增加了一个成员变量，表明在此就绪进程队列中的最大执行时间片。而且在进程控制块proc_struct中增加了一个成员变量time_slice，用来记录进程当前的可运行时间片段。这是由于RR调度算法需要考虑执行进程的运行时间不能太长。在每个timer到时的时候，操作系统会递减当前执行进程的time_slice，当time_slice为0时，就意味着这个进程运行了一段时间（这个时间片段称为进程的时间片），需要把CPU让给其他进程执行，于是操作系统就需要让此进程重新回到rq的队列尾，且重置此进程的时间片为就绪队列的成员变量最大时间片max_time_slice值，然后再从rq的队列头取出一个新的进程执行。下面来分析一下其调度算法的实现。

 RR_init完成了对进程队列的初始化
```
static void  
RR_init(struct run_queue *rq) {  
    list_init(&(rq->run_list));  
    rq->proc_num = 0;  
}  
```
RR_enqueue的函数实现即把某进程的进程控制块指针放入到rq队列末尾，且如果进程控制块的时间片为0，则需要把它重置为rq成员变量max_time_slice。这表示如果进程在当前的执行时间片已经用完，需要等到下一次有机会运行时，才能再执行一段时间。
```
static void  
RR_enqueue(struct run_queue *rq, struct proc_struct *proc) {  
    assert(list_empty(&(proc->run_link)));  
    list_add_before(&(rq->run_list), &(proc->run_link));  
    if (proc->time_slice == 0 || proc->time_slice > rq->max_time_slice) {  
        proc->time_slice = rq->max_time_slice;  
    }  
    proc->rq = rq;  
    rq->proc_num ++;  
}  
```
RR_dequeue的函数实现如下表所示。即把就绪进程队列rq的进程控制块指针的队列元素删除，并把表示就绪进程个数的proc_num减一。
```
static void  
RR_dequeue(struct run_queue *rq, struct proc_struct *proc) {  
    assert(!list_empty(&(proc->run_link)) && proc->rq == rq);  
    list_del_init(&(proc->run_link));  
    rq->proc_num --;  
}  
```
RR_pick_next的函数实现如下表所示。即选取就绪进程队列rq中的队头队列元素，并把队列元素转换成进程控制块指针。
```
static struct proc_struct *  
RR_pick_next(struct run_queue *rq) {  
    list_entry_t *le = list_next(&(rq->run_list));  
    if (le != &(rq->run_list)) {  
        return le2proc(le, run_link);  
    }  
    return NULL;  
}  
```
RR_proc_tick的函数实现如下表所示。即每次timer到时后，trap函数将会间接调用此函数来把当前执行进程的时间片time_slice减一。如果time_slice降到零，则设置此进程成员变量need_resched标识为1，这样在下一次中断来后执行trap函数时，会由于当前进程程成员变量need_resched标识为1而执行schedule函数，从而把当前执行进程放回就绪队列末尾，而从就绪队列头取出在就绪队列上等待时间最久的那个就绪进程执行。
```
static void  
RR_proc_tick(struct run_queue *rq, struct proc_struct *proc) {  
    if (proc->time_slice > 0) {  
        proc->time_slice --;  
    }  
    if (proc->time_slice == 0) {  
        proc->need_resched = 1;  
    }  
}  
```

###练习2: 实现 Stride Scheduling 调度算法（需要编码）

首先需要换掉RR调度器的实现，即用default_sched_stride_c覆盖default_sched.c。然后根据此文件和后续文档对Stride度器的相关描述，完成Stride调度算法的实现。

后面的实验文档部分给出了Stride调度算法的大体描述。这里给出Stride调度算法的一些相关的资料（目前网上中文的资料比较欠缺）。

- strid-shed paper location1
- strid-shed paper location2
- 也可GOOGLE “Stride Scheduling” 来查找相关资料

执行：make grade。如果所显示的应用程序检测都输出ok，则基本正确。如果只是priority.c过不去，可执行 make run-priority 命令来单独调试它。大致执行结果可看附录。（ 使用的是 qemu-1.0.1 ）。

请在实验报告中简要说明你的设计实现过程。

