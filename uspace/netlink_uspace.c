#include <stdio.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define NETLINK_TEST 	17
#define MAX_PAYLOAD 100
int main(void)
{
	//sockaddr_nl represents netlink client in both uspace and kspace
	struct sockaddr_nl sa, da;
	memset(&sa, 0, sizeof(struct sockaddr_nl));
	memset(&da, 0, sizeof(struct sockaddr_nl));
	//uspace
	sa.nl_family = AF_NETLINK;
	sa.nl_pad = 0;
	sa.nl_pid = getpid(); //uspace process

	//kspace
	da.nl_family = AF_NETLINK;
	da.nl_pad = 0;
	da.nl_pid = 0; //for kernel
	
	int fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_TEST);
	if (fd < 0) {
		perror("error in socket create\n");
		exit (errno);
	}
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("bind error\n");
		exit (errno);
	}

	//kernel expects following header in each netlink message
	struct nlmsghdr *nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	if (!nlh) {
		perror("not enough memory");
		exit(errno);
	}
	memset(nlh, 0, sizeof(MAX_PAYLOAD));
	nlh->nlmsg_pid = getpid(); //sending process ID 
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_type = 0;
	strcpy(NLMSG_DATA(nlh),"###string from user space###");

	struct iovec iov;
	memset(&iov, 0, sizeof(struct iovec));
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;

	struct msghdr msg;
	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_name = (void *)&da;
	msg.msg_namelen = sizeof(da);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(fd, &msg, 0);

	/* Close Netlink Socket */
	close(fd);
	return 0;
}

