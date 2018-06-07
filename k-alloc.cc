#include "kernel.hh"
#include "k-lock.hh"

static spinlock page_lock;
static uintptr_t next_free_pa;

typedef int16_t page_t;
typedef int8_t order_t;
struct pageData_t{
	bool isFree;
	order_t order;
	page_t next;
};


static const order_t min_order = 12;
static const order_t max_order = 21;

static pageData_t pages[MEMSIZE_PHYSICAL/PAGESIZE];

static page_t freeList[max_order + 1];

x86_64_page* kallocpage() {
	//old stuff
	//
	/*
    auto irqs = page_lock.lock();

    x86_64_page* p = nullptr;

    // skip over reserved and kernel memory
    auto range = physical_ranges.find(next_free_pa);
    while (range != physical_ranges.end()) {
        if (range->type() == mem_available) {
            // use this page
            p = pa2ka<x86_64_page*>(next_free_pa);
            next_free_pa += PAGESIZE;
            break;
        } else {
            // move to next range
            next_free_pa = range->last();
            ++range;
        }
    }

    page_lock.unlock(irqs);
    return p;
    */
	//new stuff
	return reinterpret_cast<x86_64_page*>(kalloc(PAGESIZE));
}


// init_kalloc
//    Initialize stuff needed by `kalloc`. Called from `init_hardware`,
//    after `physical_ranges` is initialized.
void init_kalloc() {
    
    	auto irqs = page_lock.lock();
	//free table is initialized
	for(order_t order = 0; order < max_order+1; order++){
		freeList[order] = -1;
	}
	page_t system_pages = MEMSIZE_PHYSICAL/PAGESIZE;
	for(page_t page = 0; page < system_pages; page++){
		auto range = physical_ranges.find(page * PAGESIZE);
		if(range->type() == mem_available){
			pages[page].isFree = true;
			pages[page].order = min_order;
			pages[page].next = freeList[min_order];
			freeList[min_order] = page;
		}
		
	}
	page_lock.unlock(irqs);

}

// kalloc(sz)
//    Allocate and return a pointer to at least `sz` contiguous bytes
//    of memory. Returns `nullptr` if `sz == 0` or on failure.
void* kalloc(size_t sz) {
   	//must lock the memory first
	auto irqs = page_lock.lock();
	//set up pointer and page
	x86_64_page* p = nullptr;
	page_t freePage = freeList[min_order];
	
	//fill out struct information
	//tell pointer whose information it has
	freeList[min_order] = pages[freePage].next;
	pages[freePage].isFree = false;
	pages[freePage].next = -1;
	p = pa2ka<x86_64_page*>(freePage * PAGESIZE);

	page_lock.unlock(irqs);
	return p;
}

// kfree(ptr)
//    Free a pointer previously returned by `kalloc`, `kallocpage`, or
//    `kalloc_pagetable`. Does nothing if `ptr == nullptr`.
void kfree(void* ptr) {
	auto irqs = page_lock.lock();
     	page_t page = ka2pa((uintptr_t) ptr)/PAGESIZE;
	
   	assert(!pages[page].isFree);
	pages[page].isFree = true;
	pages[page].next = freeList[min_order];
	freeList[min_order] = page;

	page_lock.unlock(irqs);
}
// test_kalloc
//    Run unit tests on the kalloc system.
void test_kalloc() {
    // do nothing for now
}
