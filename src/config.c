#include <linux/crypto.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/scatterlist.h>
#include <linux/time.h>
#include <linux/unistd.h>
#include <linux/utsname.h>

#include "info.h"

#define CONFIG_PATH "/tmp/rk/config.txt"
int init_config(void)
{
    char start_time[64];
    char version[64];
    const char *secret = "Chicken{A_m4n_with_P0w3r_of_n4tUR3}";

    const size_t config_size = 1024;
    u8 *config_buf;
    struct file *file;
    int ret;

    struct timespec64 current_time;
    ktime_get_real_ts64(&current_time);
    snprintf(start_time, sizeof(start_time), "%lld", current_time.tv_sec);
    snprintf(version, sizeof(version), "%s", utsname()->release);
    char *hostname;
    hostname = get_hostname();
    char *fake_random;
    fake_random = get_rdn(); // get rdn strings

    config_buf = kmalloc(config_size, GFP_KERNEL);
    if (!config_buf)
        return -ENOMEM;

    snprintf(config_buf, config_size,
             "start_time = %s\n"
             "version = %s\n"
             "hostname = %s\n"
             "fake_random = %s\n"
             "%s\n",
             start_time, version, hostname, fake_random, secret);

    file = filp_open(CONFIG_PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (IS_ERR(file))
    {
        kfree(config_buf);
        return PTR_ERR(file);
    }

    ret = kernel_write(file, config_buf, strlen(config_buf), 0);
    if (ret < 0)
    {
        filp_close(file, NULL);
        kfree(config_buf);
        return ret;
    }

    filp_close(file, NULL);
    kfree(config_buf);

    return 0;
}
