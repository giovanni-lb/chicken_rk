#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

int is_running_in_vm(void)
{
    const char *cpuinfo_file = "/proc/cpuinfo";
    const char *vendor_id = "vendor_id";
    const char *model_name = "model name";
    const char *qemu_vendor_id = "QEMU";
    const char *qemu_model_name = "QEMU Virtual CPU";
    const char *hypervisor_vendor = "Hypervisor detected";

    struct file *file;
    char line[256];
    bool is_qemu = false;
    bool is_vm = false;

    // Open /proc/cpuinfo file
    file = filp_open(cpuinfo_file, O_RDONLY, 0);
    if (IS_ERR(file))
    {
#ifndef DEBUG
        printk(KERN_ERR "Failed to open /proc/cpuinfo\n");
#endif
        return 1;
    }

    // Read /proc/cpuinfo line by line to find some info
    while (kernel_read(file, line, sizeof(line) - 1, &file->f_pos) > 0)
    {
        line[sizeof(line) - 1] = '\0';
        if (strncmp(line, vendor_id, strlen(vendor_id)) == 0)
        {
            if (strstr(line, qemu_vendor_id) != NULL)
                is_qemu = true;
        }
        else if (strncmp(line, model_name, strlen(model_name)) == 0)
        {
            if (strstr(line, qemu_model_name) != NULL)
                is_qemu = true;
        }
        else if (strncmp(line, hypervisor_vendor, strlen(hypervisor_vendor))
                 == 0)
        {
            is_vm = true;
        }
    }

    filp_close(file, NULL);

    // Check if the system is running in a virtualized environment
    if (is_qemu || is_vm)
        return 0;

    return 1;
}
