#include <linux/module.h>
#include <linux/kprobes.h>

int pre_handler(struct kprobe *p, struct pt_regs *regs) {
  printk(KERN_INFO "kprobe hit\n");
  return 0;
}

void post_handler(struct kprobe *p, struct pt_regs *regs,
                  unsigned long flags) {
  printk(KERN_INFO "kprobe post hit\n");
}

static struct kprobe kp = {
  .symbol_name = "dh_set_secret",
  .pre_handler = pre_handler,
  .post_handler = post_handler,
};

int init_module(void)
{ 
    int ret;
    ret = register_kprobe(&kp);
    if (ret < 0) {
      printk(KERN_ERR "failed to register kprobe: %d\n", ret);
      return ret;
    }
    printk(KERN_INFO "successfully registerd kprobe\n");
    return 0; 
} 

void cleanup_module(void)
{ 
  unregister_kprobe(&kp);
} 

MODULE_LICENSE("GPL");
