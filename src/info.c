#include <linux/file.h>
#include <linux/fs.h>
#include <linux/inet.h>
#include <linux/inetdevice.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/string.h>

#define MAX_IP_ADDR_LEN 16

#include <linux/random.h>

#define MAX_RANDOM_STRING_LENGTH 32

char *get_hostname(void)
{
    struct file *file;
    char *hostname = NULL;
    char *line_buf = NULL;
    int ret;

    file = filp_open("/etc/hostname", O_RDONLY, 0);
    if (IS_ERR(file))
        return NULL;

    // Allocate buffer for reading the username
    line_buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!line_buf)
    {
        filp_close(file, NULL);
        return NULL;
    }

    ret = kernel_read(file, line_buf, PAGE_SIZE - 1, 0);
    if (ret > 0)
    {
        line_buf[ret] = '\0';

        if (line_buf[ret - 1] == '\n')
            line_buf[ret - 1] = '\0';

        hostname = kmalloc(ret, GFP_KERNEL);
        if (hostname)
            strcpy(hostname, line_buf);
    }

    filp_close(file, NULL);
    kfree(line_buf);

    return hostname;
}

char *get_rdn(void)
{
    static char random_string[MAX_RANDOM_STRING_LENGTH];
    int i;

    for (i = 0; i < MAX_RANDOM_STRING_LENGTH - 1; ++i)
    {
        unsigned char random_char;
        get_random_bytes(&random_char, sizeof(random_char));

        random_string[i] = (random_char % 95) + 32;
    }

    random_string[MAX_RANDOM_STRING_LENGTH - 1] = '\0';
    return random_string;
}
