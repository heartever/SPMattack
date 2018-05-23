#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/traps.h>
#include <asm/desc_defs.h>
#include <linux/moduleparam.h>
#include <linux/context_tracking.h>     /* exception_enter(), ...       */

#include <linux/delay.h>
/*
 *  Copyright (C) 1995  Linus Torvalds
 *  Copyright (C) 2001, 2002 Andi Kleen, SuSE Labs.
 *  Copyright (C) 2008-2009, Red Hat Inc., Ingo Molnar
 */
#include <linux/sched.h>		/* test_thread_flag(), ...	*/
#include <linux/kdebug.h>		/* oops_begin/end, ...		*/
#include <linux/module.h>		/* search_exception_table	*/
#include <linux/bootmem.h>		/* max_low_pfn			*/
#include <linux/kprobes.h>		/* NOKPROBE_SYMBOL, ...		*/
#include <linux/mmiotrace.h>		/* kmmio_handler, ...		*/
#include <linux/perf_event.h>		/* perf_sw_event		*/
#include <linux/hugetlb.h>		/* hstate_index_to_shift	*/
#include <linux/prefetch.h>		/* prefetchw			*/
#include <linux/context_tracking.h>	/* exception_enter(), ...	*/
#include <linux/uaccess.h>		/* faulthandler_disabled()	*/

#include <linux/hugetlb.h>
#include <linux/signal.h>

#include <linux/sched.h>       //wake_up_process()
#include <linux/kthread.h>      //kthread_create()、kthread_run()   

static struct task_struct *task[8];
static struct task_struct *Checktask; 

typedef void (*do_page_fault_handler_t)(struct pt_regs *, unsigned long);
extern void default_do_page_fault(struct pt_regs *regs, unsigned long error_code);
extern do_page_fault_handler_t do_page_fault_handler;

unsigned long count = 0, pfcount = 0, interruptcount = 0;
unsigned long previous_instruction_fault = 0;

struct task_struct *target_task = NULL;
bool flag_get_target_task = false;

bool flag_lookup = false;
bool flag1c = false;
bool flag31 = false;
bool flag2b = false;
bool flag19 = false;
bool flag05 = false;

bool flag_spell = false;
bool lookup = false;
long result[2000000];
int tt = 0;

unsigned long enclave_baseaddr_App = 0;

static inline void nullfunc(void *info)
{
	//msleep(1);
}

void set_affinity(int core_id)
{
    struct cpumask mask;
    cpumask_clear(&mask);
    cpumask_set_cpu(core_id, &mask);
    sched_setaffinity(0, &mask);
}
static inline uint64_t rdtsc(void) {
  uint64_t a, d;
  asm volatile ("mfence");
  asm volatile ("rdtscp" : "=a" (a), "=d" (d) : : "rcx");
  a = (d<<32) | a;
  asm volatile ("mfence");
  return a;
}

static inline void my_flush_tlb_allpages(void *info)
{
    unsigned long val;
   	asm volatile("mov %%cr3, %0" : "=r" (val));
}
static inline void check_accessed(void);

static inline void clear_accessed_thread(void)
{
	pgd_t *pgd;
	pte_t *ptep, temp_pte;
	pud_t *pud;
	pmd_t *pmd;
	
	pte_t pte1a;
	pte_t *ptep1a;
  pte_t *ptep3;
	unsigned long addr, addr2, addr3;
	set_affinity(1);
	bool pagewalk1a = false, pagewalk23 = false, pagewalk16 = false;
	while(1)
	{		
		if(unlikely(!flag_get_target_task) )
		{
			for_each_process(target_task)
			{
				if ( (strcmp(target_task->comm, "pal-Linux-SGX") == 0 ) )      // not our process
				{
					flag_get_target_task = true;
					printk("base address: %p\n", enclave_baseaddr_App);
					//addr = enclave_baseaddr_App+0xb0e7000;
					//addr = enclave_baseaddr_App+0xb0e7000;	
  			  addr = enclave_baseaddr_App+0xb0ef000;
					break;
				}
			}
			msleep(15); 
			continue;
		}
		
		if(!target_task) break;
		
		struct mm_struct *mm = target_task->mm;
		if(!mm) break;
	     
		pgd = pgd_offset(mm, addr);
		if( !(pgd_none(*pgd) || pgd_bad(*pgd)) )
		{
			pud = pud_offset(pgd, addr);
			if( !(pud_none(*pud) || pud_bad(*pud)) )
			{	
				pmd = pmd_offset(pud, addr);
				if( !(pmd_none(*pmd) || pmd_bad(*pmd)) )
				{
					ptep1a = pte_offset_map(pmd, addr);
					if (ptep1a)
					{
						pte1a = *ptep1a;
						
						if(pte_young(pte1a))
						{
               						smp_call_function_single(3, check_accessed, NULL, 1);
               
							//check_accessed();
						
						   //result[tt++] = rdtsc();
							//set_pte(ptep, pte_mkold(*ptep));
							set_pte(ptep1a, pte_mkold(pte1a));
 							
							//    
						  // set_pte(ptep3, pte_mkold(*ptep3));   
								              
					   	       interruptcount++;
						}
					}
				}
			}
		}
	}
}

int j = 0;

const int numofaddr = 4;
static unsigned long addrlist[4] = 
{
//	0xb036000, 
//  0xb0e7000,
//  0xb0ef000,  
  0xb0e9000, 
//  0xb0ec000,   
//  0xb0ec000, 
  0xb0f0000,  
//  0xb0f2000   
  
};

static void inline check_accessed(void)
{
	pgd_t *pgd;
	pte_t *ptep, pte, temp_pte;
	pud_t *pud;
	pmd_t *pmd;
	
	unsigned long tmp[100];
	int i, cc = 0;
	unsigned long addr;
 	pte_t pte1a;
	pte_t *ptep1a;
  struct mm_struct *mm = target_task->mm;

  result[tt++] = 0; 
  bool flagtmp = true;
  for( i = 0; i < numofaddr; i++)
 //for(addr = enclave_baseaddr_App+0xb000000; addr <= enclave_baseaddr_App+0xb0ff000; addr += 0x1000)
	{
   		addr = addrlist[i];
		pgd = pgd_offset(mm, addr);
		pud = pud_offset(pgd, addr);
		pmd = pmd_offset(pud, addr);
		ptep = pte_offset_map(pmd, addr);
		if(pte_young(*ptep))
		{	
			result[tt++] = addr;
//			printk("No. %d, addr: %p\n", count++, addr);
			set_pte(ptep, pte_mkold(*ptep));
		}else
       flagtmp = false;
//		pte_unmap(ptep);
	}
  //if(flagtmp) interruptcount++;
//	}
}

static void clear_accessed(void)
{	
	int err;
	int i;
	const char name[6];
	
	for(i = 0; i < 1; i++)
	{
		sprintf(name, "task%d", i);
		task[i] = kthread_create(clear_accessed_thread, NULL, name);  
		if (IS_ERR(task[i])) {  
			printk("Unable to start kernel thread./n");  
			err = PTR_ERR(task[i]);  
			task[i] = NULL;  
			return;  
		}
		wake_up_process(task[i]); 
	}
	
	return 0;
}

void my_do_page_fault(struct pt_regs* regs, unsigned long error_code){
    struct task_struct * task = current;
    struct mm_struct *mm = task->mm;
    
	unsigned long addr = read_cr2();  /* get the page-fault address */
	unsigned long instruct_addr = regs->ip;//GET_IP(regs);  
	struct vm_area_struct *vma = mm->mmap;
	unsigned long virtual_addr;
	
	if ( (strcmp(task->comm, "pal-Linux-SGX") != 0 ) )      // not our process
	{
		default_do_page_fault(regs, error_code);
		return;
	}

	if((error_code & 0x10) != 0)   // instruction pages
	{
		
		if(instruct_addr != addr && ((addr & 0xfff) == 0) )  // instructions in the Enclave
		{
			if( !flag_lookup  &&  (addr == (enclave_baseaddr_App + 0xb0f0000) )     )
			{
				printk("[Spell] functions begins.\n");
				flag_lookup = true;
			}
		}
		
		
	}
	pfcount++;
	default_do_page_fault(regs, error_code);
}

static int __init page_fault_init(void)
{
    printk("page_fault_init module called...\n");
	
	do_page_fault_handler = my_do_page_fault;
	clear_accessed();

    return 0;
}

static void __exit page_fault_exit(void)
{
    printk("page_fault_exit module called...\n");
    do_page_fault_handler = default_do_page_fault;
	int i; count = 0; int j = 0;
  uint64_t tmp = 0; int forzero = 0;
	for(i = 0; i < tt; i++)
	{
  
  
       if(result[i] == 0) printk("Word %d: ===============\n", ++j);
       
       else 
   		    printk("No. %d, addr: %p\n", count++, result[i] - tmp);
      // tmp = result[i];
       
  //     break;
   // }
  }
  count = 0; j = 0;
  for(i = 0; i < tt; i++)
   {
  		if(result[i] == 0xb0e9000) count++;
  		else if(result[i] == 0xb0f0000)
  		{
  			if(count > 20) 
        {
          printk("No. %d, count: %d\n", j++, count);
          count = 0;
          forzero = 0;
        }
        else
        {
          forzero += count;
          while(forzero >= 14) 
          {
            printk("No. %d, count: %d\n", j++, forzero);
            forzero -= 14;
          }
          count = 0;
        }	
  		}
  
   
	}
	printk("page fault: %d, interrupt: %d\n", pfcount, interruptcount);
}

module_init(page_fault_init);
module_exit(page_fault_exit);
MODULE_LICENSE("Dual BSD/GPL");
