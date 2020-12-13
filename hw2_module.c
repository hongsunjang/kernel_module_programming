
// header files
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>

#include <linux/moduleparam.h>
#include <linux/timer.h>


#include <linux/init.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched/signal.h>


MODULE_AUTHOR("Jang, HongSun");
MODULE_LICENSE("GPL");

// Constant value for 'ns' to 'ms'
#define NS_BY_MS 1000000
//#define PAGE_SIZE 4096

//module parameters
static int period = 10;
module_param(period, int, 0);
static int pid = -1;
module_param(pid,int,0);

struct vm_area_info {
    int vm_area_type;
    int page_counts;
    bool readable;
    bool writable;
    bool executable;
    unsigned long vm_start;
    unsigned long vm_end;
    unsigned long pm_start;
    unsigned long pm_end;
};

//tasklet function and declaration
struct mem_info{
    bool task_exist;
    char* task_name;
    u64 init_time;
    u64 update_time;
    unsigned long pgd_base;
    struct vm_area_info list[255];
    int list_length;
}my_tasklet_data;



static void print_bar(struct seq_file *s){
    int i;
    for (i=0;i<80;++i)seq_printf(s, "-");
    seq_printf(s, "\n");
}


// sequence operation function for 'hw2' file
static void *hw2_seq_start(struct seq_file *s, loff_t *pos)
{   
    
    if(*pos==1){
        return NULL;
    }
    return pos;
}

static void *hw2_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    return NULL;
}

static void hw2_seq_stop(struct seq_file *s, void *v)
{
    //nothing to do
}

static int hw2_seq_show(struct seq_file *s, void *v)
{
    print_bar(s);
    seq_printf(s, "[System Programming Assignment 2]\n");
    seq_printf(s, "ID: 2017147510, Name: Jang, Hongsun\n");
    if(!(my_tasklet_data.task_exist)){
        seq_printf(s, "There is no information - PID: %d\n", pid);
        print_bar(s);
    } else{
        seq_printf(s, "Command: %s, PID: %d\n",my_tasklet_data.task_name, pid);
        print_bar(s);
        seq_printf(s, "Last update time: %llu ms\n", my_tasklet_data.update_time);
        seq_printf(s, "Page Size: ?KB]\n");
        seq_printf(s, "PGD Base Address: 0x%08lx\n", my_tasklet_data.pgd_base);
        print_bar(s);
    }
    

    return 0;
}

//hw2의 seq 관련 함수들을 가지는 구조체
static struct seq_operations hw2_seq_ops = {
    .start = hw2_seq_start,
    .next = hw2_seq_next,
    .stop = hw2_seq_stop,
    .show = hw2_seq_show
};

static int hw2_proc_open(struct inode *inode, struct file *file) {
    return seq_open(file, &hw2_seq_ops);
}


static const struct file_operations hw2_file_ops = {
    .owner = THIS_MODULE,
    .open = hw2_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release
};



void my_tasklet_function(unsigned long data){
    struct task_struct* task;
    struct vm_area_struct *iter;
    struct mem_info *info = (struct mem_info *)data;
    int index = 0;
    info->task_exist = false;
    
    //마지막에 Null 추가하기
    
    for_each_process(task){
        if((task->pid == pid) && (task->mm != NULL)){
            info->task_exist = true;
            printk("target proc exists");
            info->task_name = task->comm;
            info-> update_time = jiffies - info->init_time;
        
            
            info-> pgd_base = (task->mm->pgd->pgd);
            //printk("pgd_base:0x%08lx\n", task->mm->pgd->pgd);
            //printk("mmap base:0x%08lx\n", task->mm->mmap_base);
            //printk( "task start code: 0x%08lx\n", task->mm->start_code);
            //printk( "task end code: 0x%08lx\n", task->mm->end_code);
            //printk( "task start data: 0x%08lx\n", task->mm->start_data);
            //printk( "task end data: 0x%08lx\n", task->mm->end_data);
            //printk( "task start heap: 0x%08lx\n", task->mm->start_brk);
            //printk( "task end heap: 0x%08lx\n", task->mm->brk);
            //printk( "task start stack: 0x%08lx\n", task->mm->start_stack);
            //printk( "task end stack: 0x%08lx\n", task->mm->arg_start);
            //printk("page size:%ld",PAGE_SIZE);
            for(iter = task->mm->mmap; iter!=NULL; iter = iter->vm_next){
                if(iter->vm_start < task->mm->start_code)continue;
                if(iter->vm_start >= task->mm->arg_start)break;
                if(iter->vm_start< task->mm->end_code){
                    //code
                    info->list[index].vm_area_type = 0;
                }else if(iter->vm_start< task->mm->end_data){
                    //data
                    info->list[index].vm_area_type = 1;
                }else if(iter->vm_start< task->mm->start_brk){
                    //bss
                    info->list[index].vm_area_type = 2;
                }else if(iter->vm_start< task->mm->brk){
                    //heap
                    info->list[index].vm_area_type = 3;
                }else if(iter->vm_start< task->mm->start_stack){
                    //Shared Libraries
                    info->list[index].vm_area_type = 4;
                }else if(iter->vm_start< task->mm->arg_start){
                    //stack
                    info->list[index].vm_area_type = 5;
                }
                printk("vm_area type: %d\n",info->list[index].vm_area_type);
                
                info->list[index].page_counts = (iter->vm_end - iter->vm_start)/4096;

                printk("vm_area flag: 0x%08lx\n", iter->vm_flags);
                

                info->list[index].vm_start =iter->vm_start;
                info->list[index].vm_end =iter->vm_end;
                
                
                index++;
            }
            info->list_length=index;
            break;
        }
    }

    if(info->task_exist == false){
        printk("target proc does not exist!");
    }

    return;
}

DECLARE_TASKLET(my_tasklet, my_tasklet_function,(unsigned long) &my_tasklet_data );

//timer callback function

static struct timer_list my_timer;

void my_timer_callback( struct timer_list *timer){
    printk("my_timer_callback called (%ld).\n",jiffies);

    tasklet_schedule(&my_tasklet);
    
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(period*1000));
}

// init function when files and directory generated
static int __init hw2_init(void) {
    int ret;
     
    timer_setup(&my_timer, my_timer_callback, 0);
    my_tasklet_data.init_time=jiffies;
    proc_create("hw2", 0, NULL, &hw2_file_ops);
    ret = mod_timer(&my_timer, jiffies);

    if (ret) printk("Error in mod_timer\n");

    

    return 0; 
}


static void __exit hw2_exit(void) {
    int ret;
    ret = del_timer(&my_timer);
    if(ret) printk("The timer is still in use...\n");

    tasklet_kill(&my_tasklet);
    //remove 'hw2' file
    remove_proc_entry("hw2", NULL);
}

module_init(hw2_init);
module_exit(hw2_exit);