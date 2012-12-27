#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <net/net_namespace.h> //init_net
#define NETLINK_TEST 	17

MODULE_LICENSE("GPL v2");

static void netlink_test(void);
static void nl_data_ready(struct sk_buff *skb);

static struct sock *nl_sk = NULL;

static struct netlink_kernel_cfg nl_cfg = {
	.input = nl_data_ready,
};

static void nl_data_ready(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	if (skb == NULL) {
		printk("skb is NULL \n");
		return ;
	}
	nlh = (struct nlmsghdr *)skb->data;
	printk(KERN_INFO "%s: received netlink message payload: %s\n", __FUNCTION__, 
			(char *)NLMSG_DATA(nlh));

}

static void netlink_test(void) 
{
	nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST, &nl_cfg);
	if (!nl_sk) {
		printk(KERN_ERR "failed to register receive handler\n");
		return;
	}
}

static int __init nlink_init(void)
{
	printk(KERN_INFO "Starting netlink socket\n");
	netlink_test();
	return 0;
}

static void nlink_exit(void)
{
	sock_release(nl_sk->sk_socket);
	printk(KERN_INFO "goodbye\n");
}
module_init(nlink_init);
module_exit(nlink_exit);
