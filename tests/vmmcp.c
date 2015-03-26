#include <stdio.h> 
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arch/arch.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ros/syscall.h>

unsigned long long stack[1024];
volatile int shared = 0;
int mcp = 1;
#define V(x, t) (*((volatile t*)(x)))

static void *fail(void*arg)
{

	if (mcp)
	while (V(&shared, int) < 31) {
		if (! (V(&shared, int) & 1))
			V(&shared, int) = V(&shared, int) + 1;
//		cpu_relax();
	}
	V(&shared, int) = 55;

	__asm__ __volatile__("vmcall\n");
	__asm__ __volatile__("mov $0xdeadbeef, %rbx; mov 5, %rax\n");	
}

unsigned long long *p512, *p1, *p2m;

void *talk_thread(void *arg)
{
	printf("talk thread ..\n");
	for(; V(&shared, int) < 32; ){
		if (V(&shared, int) & 1) {
			printf("shared %d\n", V(&shared, int) );
			V(&shared, int) = V(&shared, int) + 1;
		}
		cpu_relax();
	}
	printf("All done\n");
	return NULL;
}

pthread_t *my_threads;
void **my_retvals;
int nr_threads = 2;

int main(int argc, char **argv)
{
	int nr_gpcs = 1;
	int fd = open("#c/sysctl", O_RDWR), ret;
	void * x;
	static char cmd[512];
	if (fd < 0) {
		perror("#c/sysctl");
		exit(1);
	}
	if (ros_syscall(SYS_setup_vmm, nr_gpcs, 0, 0, 0, 0, 0) != nr_gpcs) {
		perror("Guest pcore setup failed");
		exit(1);
	}

	mcp = 1; //argc - 1;
	if (mcp) {
		my_threads = malloc(sizeof(pthread_t) * nr_threads);
		my_retvals = malloc(sizeof(void*) * nr_threads);
		if (!(my_retvals && my_threads))
			perror("Init threads/malloc");

		pthread_can_vcore_request(FALSE);	/* 2LS won't manage vcores */
		pthread_need_tls(FALSE);
		pthread_lib_init();					/* gives us one vcore */
		vcore_request(nr_threads - 1);		/* ghetto incremental interface */
		for (int i = 0; i < nr_threads; i++) {
			x = __procinfo.vcoremap;
			printf("%p\n", __procinfo.vcoremap);
			printf("Vcore %d mapped to pcore %d\n", i,
			    	__procinfo.vcoremap[i].pcoreid);
		}
	}

	if (mcp) {
		if (pthread_create(&my_threads[0], NULL, &talk_thread, NULL))
			perror("pth_create failed");
//		if (pthread_create(&my_threads[1], NULL, &fail, NULL))
//			perror("pth_create failed");
	}
	printf("threads started\n");

	if (0) for (int i = 0; i < nr_threads-1; i++) {
		int ret;
		if (pthread_join(my_threads[i], &my_retvals[i]))
			perror("pth_join failed");
		printf("%d %d\n", i, ret);
	}
	

	ret = syscall(33, 1);
	if (ret < 0) {
		perror("vm setup");
		exit(1);
	}
	ret = posix_memalign((void **)&p512, 4096, 3*4096);
	if (ret) {
		perror("ptp alloc");
		exit(1);
	}
	p1 = &p512[512];
	p2m = &p512[1024];
	p512[0] = (unsigned long long)p1 | 7;
	p1[0] = /*0x87; */(unsigned long long)p2m | 7;
	p2m[0] = 0x87;
	p2m[1] = 0x200000 | 0x87;
	p2m[2] = 0x400000 | 0x87;
	p2m[3] = 0x600000 | 0x87;

	printf("p512 %p p512[0] is 0x%lx p1 %p p1[0] is 0x%x\n", p512, p512[0], p1, p1[0]);
	sprintf(cmd, "V 0x%x 0x%x 0x%x", (unsigned long long)fail, (unsigned long long) &stack[1024], (unsigned long long) p512);
	printf("Writing command :%s:\n", cmd);
	ret = write(fd, cmd, strlen(cmd));
	if (ret != strlen(cmd)) {
		perror(cmd);
	}
	printf("shared is %d\n", shared);

	return 0;
}
