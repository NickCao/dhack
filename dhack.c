#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/fips.h>
#include <crypto/internal/kpp.h>
#include <crypto/kpp.h>
#include <crypto/dh.h>
#include <crypto/rng.h>
#include <linux/mpi.h>

// begin copied code

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

// end copied code

struct data {
  struct crypto_kpp *tfm;
  const void *buf;
	unsigned int len;
};

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
  int res;
  struct data* dt = (struct data*) ri->data;
  dt->tfm = regs->di;
  dt->buf = regs->si;
  dt->len = regs->dx;
  printk(KERN_INFO "dhack: kretprobe entry, tfm: %p, buf: %p, len: %d\n", regs->di, regs->si, regs->dx);
  return 0;
}

static int handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
  struct data* dt = (struct data*) ri->data;
  printk(KERN_INFO "dhack: kretprobe exit, rax: %d\n", regs->ax);
  if (regs->ax != 0) {
    regs->ax = dh_set_secret(dt->tfm, dt->buf, dt->len);
    printk(KERN_INFO "dhack: kretprobe exit, called modified function, rax: %d\n", regs->ax);
  }
  return 0;
}

static struct kretprobe krp = {
  .handler = handler,
  .entry_handler = entry_handler,
  .data_size = sizeof(struct data),
  .maxactive = 20,
};

int init_module(void)
{ 
    int ret;
    krp.kp.symbol_name = "dh_set_secret";
    ret = register_kretprobe(&krp);
    if (ret < 0) {
      printk(KERN_ERR "dhack: failed to register kretprobe: %d\n", ret);
      return ret;
    }
    printk(KERN_INFO "dhack: successfully registerd kretprobe at %p\n", krp.kp.addr);
    return 0; 
} 

void cleanup_module(void)
{ 
  unregister_kretprobe(&krp);
  printk(KERN_INFO "dhack: unregistered kretprobe\n");
} 

MODULE_LICENSE("GPL");
