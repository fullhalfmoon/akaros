// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef ROS_INC_LIB_H
#define ROS_INC_LIB_H 1

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/string.h>
#include <inc/error.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/memlayout.h>
#include <inc/syscall.h>

#define USED(x)		(void)(x)

// libos.c or entry.S
extern char *binaryname;
extern volatile env_t *env;
// will need to change these types when we have real structs
// seems like they need to be either arrays [] or functions () for it to work
extern volatile uint8_t (COUNT(PGSIZE * UINFO_PAGES) procinfo)[];
extern volatile uint8_t (COUNT(PGSIZE * UDATA_PAGES) procdata)[];
extern syscall_front_ring_t sysfrontring;
extern volatile page_t pages[];
void	exit(void);

// readline.c
char*	readline(const char *buf);

// syscall.c
void sys_null();
void sys_cputs(const char *string, size_t len);
void sys_cputs_async(const char *string, size_t len, syscall_desc_t* desc);
int	sys_cgetc(void);
envid_t	sys_getenvid(void);
int	sys_env_destroy(envid_t);
error_t waiton_syscall(syscall_desc_t* desc, syscall_rsp_t* rsp);

// async callback
typedef uint32_t async_desc;

extern syscall_desc_t ALL_ASYNC_CALLS[][];
error_t waiton_async_call(async_desc desc);
//get_free_async_desc

/*
typedef syscall_waiter_t;
typedef struct syscall_waiter {
	syscall_desc_t desc;	
	LIST_ENTRY(syscall_waiter_t) next;
} syscall_waiter_t;
LIST_HEAD(syscall_waiter_list_t, syscall_waiter_t); 

syscall_waiter_list_t ALL_ASYNC_CALLS[256];
sys_cpit_async (desc)
put_syscalls_on_a_fucking_async_callback(async_desc, desc);
*/

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if already exists */
#define O_MKDIR		0x0800		/* create directory, not regular file */

#endif	// !ROS_INC_LIB_H
