#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/fips.h>
#include <crypto/internal/kpp.h>
#include <crypto/kpp.h>
#include <crypto/dh.h>
#include <crypto/rng.h>
#include <linux/mpi.h>

struct dh_ctx {
	MPI p;	/* Value is guaranteed to be set. */
	MPI g;	/* Value is guaranteed to be set. */
	MPI xa;	/* Value is guaranteed to be set. */
};

static void dh_clear_ctx(struct dh_ctx *ctx)
{
	mpi_free(ctx->p);
	mpi_free(ctx->g);
	mpi_free(ctx->xa);
	memset(ctx, 0, sizeof(*ctx));
}

static inline struct dh_ctx *dh_get_ctx(struct crypto_kpp *tfm)
{
	return kpp_tfm_ctx(tfm);
}

static int dh_check_params_length(unsigned int p_len)
{
	if (fips_enabled)
		return (p_len < 2048) ? -EINVAL : 0;

	return (p_len < 1024) ? -EINVAL : 0;
}

static int dh_set_params(struct dh_ctx *ctx, struct dh *params)
{
	if (dh_check_params_length(params->p_size << 3))
		return -EINVAL;

	ctx->p = mpi_read_raw_data(params->p, params->p_size);
	if (!ctx->p)
		return -EINVAL;

	ctx->g = mpi_read_raw_data(params->g, params->g_size);
	if (!ctx->g)
		return -EINVAL;

	return 0;
}

static int dh_set_secret(struct crypto_kpp *tfm, const void *buf,
			 unsigned int len)
{
	struct dh_ctx *ctx = dh_get_ctx(tfm);
	struct dh params;

	/* Free the old MPI key if any */
	dh_clear_ctx(ctx);

	if (crypto_dh_decode_key(buf, len, &params) < 0)
		goto err_clear_ctx;

	if (dh_set_params(ctx, &params) < 0)
		goto err_clear_ctx;

	ctx->xa = mpi_read_raw_data(params.key, params.key_size);
	if (!ctx->xa)
		goto err_clear_ctx;

	return 0;

err_clear_ctx:
	dh_clear_ctx(ctx);
	return -EINVAL;
}

struct data {
  int res;
};

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
  int res;
  // res = dh_set_secret(regs->di, regs->si, regs->dx);
  ((struct data*) ri->data)->res = res;
  printk(KERN_INFO "kprobe hit, res: %d\n", res);
  return 0;
}

static int handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
  printk(KERN_INFO "kprobe post hit, rax: %d\n", regs->ax);
  // regs->ax = ((struct data*) ri->data)->res;
  return 0;
}

static struct kretprobe krp = {
  .handler = handler,
  .entry_handler = entry_handler,
  .maxactive = 20,
  .data_size = sizeof(struct data),
};

int init_module(void)
{ 
    int ret;
    krp.kp.symbol_name = "dh_set_secret";
    ret = register_kretprobe(&krp);
    if (ret < 0) {
      printk(KERN_ERR "failed to register kprobe: %d\n", ret);
      return ret;
    }
    printk(KERN_INFO "successfully registerd kprobe at %p\n", krp.kp.addr);
    return 0; 
} 

void cleanup_module(void)
{ 
  unregister_kretprobe(&krp);
  printk(KERN_INFO "unregistered kprobe\n");
} 

MODULE_LICENSE("GPL");
