#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("ddp_pass")
int  ddp_pass_func(struct ddp_md *ctx)
{
	return 20;
}