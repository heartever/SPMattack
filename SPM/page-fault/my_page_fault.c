#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/traps.h>
#include <asm/desc_defs.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/context_tracking.h>     /* exception_enter(), ...       */


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

typedef void (*do_page_fault_handler_t)(struct pt_regs *, unsigned long);
extern void default_do_page_fault(struct pt_regs *regs, unsigned long error_code);
extern do_page_fault_handler_t do_page_fault_handler;

unsigned long count = 0;
unsigned long previous_instr_addr = 0;
unsigned long previous_instruction_fault = 0;
unsigned long p_previous_instruction_fault = 0;
unsigned long pp_previous_instruction_fault = 0;
unsigned long ppp_previous_instruction_fault = 0;
unsigned long previous_data_fault = 0;
unsigned long p_previous_data_fault = 0;

unsigned long library_start_address = 0;
bool flag_library_start_address = true;

unsigned long first_address;
bool flag_first_address = false;

int numofpagefault = 0;

unsigned long trace_addr[200];  // to store trace of every insert or lookup
unsigned long dic_num = 0;
int trace_addr_index = 0;

bool add_word_function = false;
bool lookup_function = false;

bool tmp_flag = false;

bool flag_of_first = true;
unsigned long firstEnclaveInstruction = 0;

unsigned long word_count = 0;

bool print_trace = false;

bool flag_lookup = false;
bool flag_add_word = false;
bool flag_spell = false;

static inline void my_flush_tlb_singlepage(unsigned long addr)
{
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static void clear_codepage(void)
{
	pgd_t *pgd;
	pte_t *ptep, pte, temp_pte;
	pud_t *pud;
	pmd_t *pmd;
	
	unsigned long addr;
	struct task_struct * task = current;
	struct page *page = NULL;
	struct mm_struct *mm = task->mm;
	
	for(addr = mm->start_code; addr < mm->end_code; addr += 0x1000)
	//for(addr = 0x00000000LL; addr < 0xffffffffLL; addr += 0x1000)
	{
		pgd = pgd_offset(mm, addr);
		if (pgd_none(*pgd) || pgd_bad(*pgd))
		{
			continue;
		}

		pud = pud_offset(pgd, addr);
		if (pud_none(*pud) || pud_bad(*pud))
		{
			continue;
		}
		
		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd) || pmd_bad(*pmd))
		{
			continue;
		}
		
		ptep = pte_offset_map(pmd, addr);
		if (!ptep)
		{
			continue;
		}
		
		pte = *ptep;
	
		//temp_pte = pte_set_flags(pte, _PAGE_SOFTW1);
		//if(pte_present(pte))
		{
			// down_write(&mm->mmap_sem);
			if((pte_flags(pte) & _PAGE_PRESENT))
			//if(pte_present(pte))
			{
				//printk("Cleared_codepage: %08x\n", addr);
				temp_pte = pte_clear_flags(pte, _PAGE_PRESENT);
				//temp_pte = pte_set_flags(temp_pte, _PAGE_SOFTW1);
				//temp_pte = pte_set_flags(temp_pte, _PAGE_SOFTW2);
				temp_pte = pte_set_flags(temp_pte, _PAGE_PROTNONE);
				set_pte(ptep, temp_pte);
				my_flush_tlb_singlepage(addr);
			}
			// up_write(&mm->mmap_sem);
		}
		if(ptep) pte_unmap(ptep);
	}
}

static void clear_heapregion(void)
{
	pgd_t *pgd;
	pte_t *ptep, pte, temp_pte;
	pud_t *pud;
	pmd_t *pmd;
	
	unsigned long addr;
	struct task_struct * task = current;
	struct page *page = NULL;
	struct mm_struct *mm = task->mm;
	
	for(addr = mm->start_brk; addr < mm->brk; addr += 0x1000)
	//for(addr = 0x00000000LL; addr < 0xffffffffLL; addr += 0x1000)
	{
		pgd = pgd_offset(mm, addr);
		if (pgd_none(*pgd) || pgd_bad(*pgd))
		{
			continue;
		}

		pud = pud_offset(pgd, addr);
		if (pud_none(*pud) || pud_bad(*pud))
		{
			continue;
		}
		
		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd) || pmd_bad(*pmd))
		{
			continue;
		}
		
		ptep = pte_offset_map(pmd, addr);
		if (!ptep)
		{
			continue;
		}
		
		pte = *ptep;
	
		//temp_pte = pte_set_flags(pte, _PAGE_SOFTW1);
		//if(pte_present(pte))
		{
			// down_write(&mm->mmap_sem);
			if((pte_flags(pte) & _PAGE_PRESENT))
			//if(pte_present(pte))
			{
				//printk("Cleared_codepage: %08x\n", addr);
				temp_pte = pte_clear_flags(pte, _PAGE_PRESENT);
				//temp_pte = pte_set_flags(temp_pte, _PAGE_SOFTW1);
				//temp_pte = pte_set_flags(temp_pte, _PAGE_SOFTW2);
				temp_pte = pte_set_flags(temp_pte, _PAGE_PROTNONE);
				set_pte(ptep, temp_pte);
				my_flush_tlb_singlepage(addr);
			}
			// up_write(&mm->mmap_sem);
		}
		if(ptep) pte_unmap(ptep);
	}
}

static void clear_targetVM(unsigned long address)
{
	pgd_t *pgd;
	pte_t *ptep, pte, temp_pte;
	pud_t *pud;
	pmd_t *pmd;
	
	unsigned long addr;
	struct task_struct * task = current;
	struct page *page = NULL;
	struct mm_struct *mm = task->mm;
	
	//struct vm_area_struct *vma = mm->mmap;
	struct vm_area_struct *vma = find_vma(mm, address);;
	//while(vma != NULL)
	{
		for(addr = vma->vm_start; addr < vma->vm_end; addr += 0x1000)
		
		//addr = address;
		{
			pgd = pgd_offset(mm, addr);
			if( !(pgd_none(*pgd) || pgd_bad(*pgd)) )
			{
				pud = pud_offset(pgd, addr);
				if( !(pud_none(*pud) || pud_bad(*pud)) )
				{	
					pmd = pmd_offset(pud, addr);
					if( !(pmd_none(*pmd) || pmd_bad(*pmd)) )
					{
						ptep = pte_offset_map(pmd, addr);
						if (ptep)
						{
							pte = *ptep;
			
							//// down_write(&mm->mmap_sem);
							if((pte_flags(pte) & _PAGE_PRESENT))
							//if(pte_present(pte))
							{
								//printk("Cleared_targetVM: %08x\n", addr);
								temp_pte = pte_clear_flags(pte, _PAGE_PRESENT);
								temp_pte = pte_set_flags(temp_pte, _PAGE_PROTNONE);
								set_pte(ptep, temp_pte);
								my_flush_tlb_singlepage(addr);
							}
							if(ptep) pte_unmap(ptep);
							//// up_write(&mm->mmap_sem);
						}
					}
				}
			}
		}
	}
}

static void clear_targetVM2(unsigned long address)
{
	pgd_t *pgd;
	pte_t *ptep, pte, temp_pte;
	pud_t *pud;
	pmd_t *pmd;
	
	unsigned long addr;
	struct task_struct * task = current;
	struct page *page = NULL;
	struct mm_struct *mm = task->mm;
	
	//struct vm_area_struct *vma = mm->mmap;
	struct vm_area_struct *vma = find_vma(mm, address);;
	//while(vma != NULL)
	{
		addr = address;
		{
			pgd = pgd_offset(mm, addr);
			if( !(pgd_none(*pgd) || pgd_bad(*pgd)) )
			{
				pud = pud_offset(pgd, addr);
				if( !(pud_none(*pud) || pud_bad(*pud)) )
				{	
					pmd = pmd_offset(pud, addr);
					if( !(pmd_none(*pmd) || pmd_bad(*pmd)) )
					{
						ptep = pte_offset_map(pmd, addr);
						if (ptep)
						{
							pte = *ptep;
			
							//// down_write(&mm->mmap_sem);
							if((pte_flags(pte) & _PAGE_PRESENT))
							//if(pte_present(pte))
							{
								//printk("Cleared_targetVM: %08x\n", addr);
								temp_pte = pte_clear_flags(pte, _PAGE_PRESENT);
								temp_pte = pte_set_flags(temp_pte, _PAGE_PROTNONE);
								set_pte(ptep, temp_pte);
								my_flush_tlb_singlepage(addr);
							}
							if(ptep) pte_unmap(ptep);
							//// up_write(&mm->mmap_sem);
						}
					}
				}
			}
		}
	}
}

static void clear_targetData(void)
{
	pgd_t *pgd;
	pte_t *ptep, pte, temp_pte;
	pud_t *pud;
	pmd_t *pmd;
	
	unsigned long addr;
	struct task_struct * task = current;
	struct page *page = NULL;
	struct mm_struct *mm = task->mm;
	
	{
		for(addr = firstEnclaveInstruction - 0x2d000 + 0x200000; addr < firstEnclaveInstruction - 0x2d000 + 0xffff000; addr += 0x1000)
		
		//addr = address;
		{
			pgd = pgd_offset(mm, addr);
			if( !(pgd_none(*pgd) || pgd_bad(*pgd)) )
			{
				pud = pud_offset(pgd, addr);
				if( !(pud_none(*pud) || pud_bad(*pud)) )
				{	
					pmd = pmd_offset(pud, addr);
					if( !(pmd_none(*pmd) || pmd_bad(*pmd)) )
					{
						ptep = pte_offset_map(pmd, addr);
						if (ptep)
						{
							pte = *ptep;
			
							//// down_write(&mm->mmap_sem);
							if((pte_flags(pte) & _PAGE_PRESENT))
							//if(pte_present(pte))
							{
								//printk("Cleared_targetVM: %08x\n", addr);
								temp_pte = pte_clear_flags(pte, _PAGE_PRESENT);
								temp_pte = pte_set_flags(temp_pte, _PAGE_PROTNONE);
								set_pte(ptep, temp_pte);
								my_flush_tlb_singlepage(addr);
							}
							if(ptep) pte_unmap(ptep);
							//// up_write(&mm->mmap_sem);
						}
					}
				}
			}
		}
	}
}

static void clear_allpages(void)
{
	pgd_t *pgd;
	pte_t *ptep, pte, temp_pte;
	pud_t *pud;
	pmd_t *pmd;
	
	unsigned long addr;
	struct task_struct * task = current;
	struct page *page = NULL;
	struct mm_struct *mm = task->mm;
	
	struct vm_area_struct *vma = mm->mmap;
	
	// addr >= mm->brk && 
	// vma = vma->vm_next; vma = vma->vm_next; skip code and heap
	//struct vm_area_struct *vma = find_vma(mm, address);;
	while(vma != NULL)
	{
		for(addr = vma->vm_start ; addr < vma->vm_end; addr += 0x1000)
		{
			pgd = pgd_offset(mm, addr);
			if( !(pgd_none(*pgd) || pgd_bad(*pgd)) )
			{
				pud = pud_offset(pgd, addr);
				if( !(pud_none(*pud) || pud_bad(*pud)) )
				{	
					pmd = pmd_offset(pud, addr);
					if( !(pmd_none(*pmd) || pmd_bad(*pmd)) )
					{
						ptep = pte_offset_map(pmd, addr);
						if (ptep)
						{
							pte = *ptep;
			
							if(pte_present(pte))
							{
								//printk("clear %08x", addr);
								//if((pte_flags(pte) & _PAGE_PRESENT)) 
								{
									temp_pte = pte_clear_flags(pte, _PAGE_PRESENT);
									temp_pte = pte_set_flags(temp_pte, _PAGE_PROTNONE);
									set_pte(ptep, temp_pte);
									my_flush_tlb_singlepage(addr);
								}
							}
							if(ptep) pte_unmap(ptep);
						}
					}
				}
			}
		}
		vma = vma->vm_next;
	}
	
}

bool flag = false;
int calledfunc = 0;
void my_do_page_fault(struct pt_regs* regs, unsigned long error_code){
    struct task_struct * task = current;
    struct mm_struct *mm = task->mm;
    
	unsigned long addr = read_cr2();  /* get the page-fault address */
	unsigned long instruct_addr = regs->ip;//GET_IP(regs);  
	struct vm_area_struct *vma = mm->mmap;
	unsigned long virtual_addr;
	
	pgd_t *pgd;
    pte_t *ptep, pte, temp_pte;
    pud_t *pud;
    pmd_t *pmd;
	
	if ( (strcmp(task->comm, "pal-Linux-SGX") != 0 ) )      // not our process
	{
		default_do_page_fault(regs, error_code);
		return;
	}
	clear_targetVM2(0xb0f0000);
	clear_targetVM2(0xb0f1000);		
    clear_targetVM2(0xb0e7000);	
    clear_targetVM2(0xb0e8000);

	if((error_code & 0x10) != 0)   // instruction pages
	{	
		if(instruct_addr != addr && ((addr & 0xfff) == 0) )  // instructions in the Enclave
		{
       //if(p_previous_instruction_fault == 0xb0f1000 && previous_instruction_fault == 0xb0f0000 && addr == 0xb0f1000)
       //  flag = true;
      //  if(flag)
      if(addr == 0xb0f1000 && previous_instruction_fault == 0xb0e7000 && pp_previous_instruction_fault == 0xb0f0000 && ppp_previous_instruction_fault == 0xb0e8000)  //
      //  if( pp_previous_instruction_fault == 0xb0f0000)
        {
           printk("No. %d, Instruction: %p, %p, %p, %p, %p, functionscalled: %d\n", count++, ppp_previous_instruction_fault, pp_previous_instruction_fault, p_previous_instruction_fault, previous_instruction_fault, addr, calledfunc);
           calledfunc = 0;
        }
        else
          calledfunc++;
			// clear_targetVM(previous_instruction_fault);
      
			if(p_previous_instruction_fault == addr)
			{
				pgd = pgd_offset(mm, previous_instruction_fault);
				if( !(pgd_none(*pgd) || pgd_bad(*pgd)) )
				{
					pud = pud_offset(pgd, previous_instruction_fault);
					if( !(pud_none(*pud) || pud_bad(*pud)) )
					{	
						pmd = pmd_offset(pud, previous_instruction_fault);
						if( !(pmd_none(*pmd) || pmd_bad(*pmd)) )
						{
							ptep = pte_offset_map(pmd, previous_instruction_fault);
							if (ptep)
							{
								pte = *ptep;
						
								//// down_write(&mm->mmap_sem);
								if(!(pte_flags(pte) & _PAGE_PRESENT) && (pte_flags(pte) & _PAGE_PROTNONE))
								{
									temp_pte = pte_set_flags(pte, _PAGE_PRESENT);
									//temp_pte = pte_clear_flags(temp_pte, _PAGE_PROTNONE);
									set_pte(ptep, temp_pte);
								}
								//// up_write(&mm->mmap_sem);
								if(ptep) pte_unmap(ptep);
							}
						}
					}
				}
			}
		   
			pgd = pgd_offset(mm, addr);
			if( !(pgd_none(*pgd) || pgd_bad(*pgd)) )
			{
				pud = pud_offset(pgd, addr);
				if( !(pud_none(*pud) || pud_bad(*pud)) )
				{	
					pmd = pmd_offset(pud, addr);
					if( !(pmd_none(*pmd) || pmd_bad(*pmd)) )
					{
						ptep = pte_offset_map(pmd, addr);
						if (ptep)
						{
							pte = *ptep;
						
							//// down_write(&mm->mmap_sem);
							if(!(pte_flags(pte) & _PAGE_PRESENT) && (pte_flags(pte) & _PAGE_PROTNONE))
							{
								temp_pte = pte_set_flags(pte, _PAGE_PRESENT);
								//temp_pte = pte_clear_flags(temp_pte, _PAGE_PROTNONE);
								set_pte(ptep, temp_pte);
							}
							//// up_write(&mm->mmap_sem);
							if(ptep) pte_unmap(ptep);
						}
					}
				}
			}
			ppp_previous_instruction_fault = pp_previous_instruction_fault;
			pp_previous_instruction_fault = p_previous_instruction_fault;
			p_previous_instruction_fault = previous_instruction_fault;
			previous_instruction_fault = addr;
		}
	}
	numofpagefault++;
	default_do_page_fault(regs, error_code);
}

static int __init page_fault_init(void)
{
    printk("page_fault_init module called...\n");
	  do_page_fault_handler = my_do_page_fault;
    return 0;
}

static void __exit page_fault_exit(void)
{
    printk("page_fault_exit module called...\n");
    printk("Number of page faults: %d\n", numofpagefault);
    do_page_fault_handler = default_do_page_fault;
}

module_init(page_fault_init);
module_exit(page_fault_exit);
MODULE_LICENSE("Dual BSD/GPL");
