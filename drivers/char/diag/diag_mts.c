/*
 * DIAG MTS for LGE MTS Kernel Driver
 *
 *  lg-msp TEAM <lg-msp@lge.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "diag_mts.h"
#define HDLC_END 0x7E

struct mts_tty *mtsk_tty = NULL;

int mtsk_tty_process(char *buf, int left)
{
	int num_push = 0;
	int total_push = 0;

	struct mts_tty *mtsk_tty_drv = mtsk_tty;

    printk("%s\n", __func__);

	if (mtsk_tty_drv == NULL)  {
		return -1;
	}

	//num_push = tty_insert_flip_string(mtsk_tty_drv->tty_struct,
	num_push = tty_insert_flip_string(mtsk_tty_drv->mts_tty_port,
			buf + total_push, left);
	total_push += num_push;
	left -= num_push;
	//tty_flip_buffer_push(mtsk_tty_drv->tty_struct);
	tty_flip_buffer_push(mtsk_tty_drv->mts_tty_port);

	return total_push;
}

static int mtsk_tty_open(struct tty_struct *tty, struct file *file)
{
	struct mts_tty *mtsk_tty_drv = NULL;

    printk("%s\n", __func__);
	if (!tty)
		return -ENODEV;

	mtsk_tty_drv = mtsk_tty;

	if (!mtsk_tty_drv)
		return -ENODEV;

    tty_port_tty_set(mtsk_tty_drv->mts_tty_port, tty);
    mtsk_tty_drv->mts_tty_port->low_latency = 0;

	tty->driver_data = mtsk_tty_drv;
	mtsk_tty_drv->tty_struct = tty;

	set_bit(TTY_NO_WRITE_SPLIT, &mtsk_tty_drv->tty_struct->flags);

 	diagfwd_connect();
        diagfwd_cancel_hsic(REOPEN_HSIC); // QCT 161032 migration - NEED TO CHECK
	diagfwd_connect_bridge(0);
	pr_debug(KERN_INFO "mtsk_tty_open TTY device open %d,%d\n", 0, 0);
	return 0;
}

static void mtsk_tty_close(struct tty_struct *tty, struct file *file)
{
	struct mts_tty *mtsk_tty_drv = NULL;

    printk("%s\n", __func__);

 	diagfwd_disconnect();
        diagfwd_cancel_hsic(REOPEN_HSIC); // QCT 161032 migration - NEED TO CHECK
	diagfwd_disconnect_bridge(0);


	if (!tty) {
		printk( "mtsk_tty_close FAIL."
				 "tty is Null %d,%d\n", 0, 0);
		return;
	}

	mtsk_tty_drv = tty->driver_data;
    tty_port_tty_set(mtsk_tty_drv->mts_tty_port, NULL);
	printk( "mtsk_tty_close TTY device close %d,%d\n", 0, 0);

	return;
}

static int mtsk_tty_ioctl(struct tty_struct *tty, unsigned int cmd,
			  unsigned long arg)
{
	int ret = 0;

    printk("%s\n", __func__);
	switch (cmd) {
	case MTSK_TTY_MTS_START:
		mtsk_tty->run = 1;
		printk("mtsk_tty->run: 1 (%s)\n", __func__);
		break;
	case MTSK_TTY_MTS_STOP:
		mtsk_tty->run = 0;
		wake_up_interruptible(&mtsk_tty->waitq);
		mtsk_tty->pm_notify_info = 2;
		printk("mtsk_tty->run: 0 (%s)\n", __func__);
		break;
	case MTSK_TTY_MTS_READ:
		wait_event_interruptible(mtsk_tty->waitq,
					mtsk_tty->pm_notify_info != 0);
		printk("MTSK_TTY_MTS_READ (%d)\n", mtsk_tty->pm_notify_info);

		ret = copy_to_user((void *)arg,
				(const void *)&mtsk_tty->pm_notify_info,
				sizeof(int));
		if (ret != 0)
			printk("err: MTSK_TTY_MTS_READ (%s)\n", __func__);
		mtsk_tty->pm_notify_info = 0;
		break;
	default:
		break;
	}
	return ret;
}

static void mtsk_tty_unthrottle(struct tty_struct *tty) {
	return;
}

static int mtsk_tty_write_room(struct tty_struct *tty) {
	return DIAG_MTS_TX_SIZE;
}

static int mtsk_tty_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
	int index=0;
	struct mts_tty *mtsk_tty_drv = NULL;

	mtsk_tty_drv = mtsk_tty;
	tty->driver_data = mtsk_tty_drv;
	mtsk_tty_drv->tty_struct = tty;


		pr_debug(KERN_INFO "mtsk_tty_write TTY device write %d,%d\n", 0, 0);

		diag_bridge[index].usb_read_ptr->context = (void *)index;
		/* check the packet size */
		if (count > DIAG_MTS_RX_MAX_PACKET_SIZE) {
			printk(KERN_INFO "mtsk_tty_write packet size  %d,%d\n",
					 count, DIAG_MTS_RX_MAX_PACKET_SIZE);
			return -EPERM;
		}

		diag_bridge[index].usb_read_ptr->actual = count - 4;

#ifdef DIAG_DEBUG
	print_hex_dump(KERN_DEBUG, "MASK DATA ", DUMP_PREFIX_OFFSET, 16, 1,
                             buf + 4, count - 4, 1);
#endif

		memcpy(diag_bridge[index].usb_buf_out, (char *)buf + 4,
			   diag_bridge[index].usb_read_ptr->actual);

		diag_bridge[index].usb_read_ptr->buf =
				 diag_bridge[index].usb_buf_out;
		diag_bridge[index].usb_read_ptr->length = USB_MAX_OUT_BUF;
	
 		mtsk_tty_send_mask(diag_bridge[index].usb_read_ptr);
		while (diag_hsic[index].in_busy_hsic_write == 1) {
			msleep(25);
		}
	
	queue_work(diag_bridge[index].wq,
		 &diag_hsic[index].diag_read_hsic_work);
    printk("%s - %d\n", __func__, count);

	return count;
}

static const struct tty_operations mtsk_tty_ops = {
	.open = mtsk_tty_open,
	.close = mtsk_tty_close,
	.write = mtsk_tty_write,
	.write_room = mtsk_tty_write_room,
	.unthrottle = mtsk_tty_unthrottle,
	.ioctl = mtsk_tty_ioctl,
};

static int mts_pm_notify(struct notifier_block *b, unsigned long event, void *p)
{
	wake_up_interruptible(&mtsk_tty->waitq);

	switch (event) {
        case PM_SUSPEND_PREPARE:
		mtsk_tty->pm_notify_info = 3;
		printk("mts_pm_notify: PM_SUSPEND_PREPARE\n");
                break;
        case PM_POST_SUSPEND:
		mtsk_tty->pm_notify_info = 4;
		printk("mts_pm_notify: PM_POST_SUSPEND\n");
                break;
	default:
		break;
        }

        return 0;
}

static int __init mtsk_tty_init(void)
{
	int ret = 0;
	struct device *tty_dev =  NULL;
	struct mts_tty *mtsk_tty_drv = NULL;

	printk("%s\n", __func__);

	mtsk_tty_drv = kzalloc(sizeof(struct mts_tty), GFP_KERNEL);
    mtsk_tty_drv->mts_tty_port = kzalloc(sizeof(struct tty_port), GFP_KERNEL);
	tty_port_init(mtsk_tty_drv->mts_tty_port);

	if (mtsk_tty_drv == NULL) {
		printk( "mtsk_tty_init: memory alloc fail %d - %d\n", 0, 0);
		return 0;
	}

	mtsk_tty = mtsk_tty_drv;
	mtsk_tty_drv->tty_drv = alloc_tty_driver(MAX_DIAG_MTS_DRV);

	if (!mtsk_tty_drv->tty_drv) {
		printk( "mtsk_tty_init: tty alloc driver fail %d - %d\n", 1, 0);
		kfree(mtsk_tty_drv);
		return 0;
	}

	mtsk_tty_drv->tty_drv->name = "diag_mts";
	mtsk_tty_drv->tty_drv->owner = THIS_MODULE;
	mtsk_tty_drv->tty_drv->driver_name = "diag_mts";

	/* uses dynamically assigned dev_t values */
	mtsk_tty_drv->tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	mtsk_tty_drv->tty_drv->subtype = SERIAL_TYPE_NORMAL;
	mtsk_tty_drv->tty_drv->flags = TTY_DRIVER_REAL_RAW |
				       TTY_DRIVER_DYNAMIC_DEV |
				       TTY_DRIVER_RESET_TERMIOS;

	/* initializing the mts driver */
	mtsk_tty_drv->tty_drv->init_termios = tty_std_termios;
	mtsk_tty_drv->tty_drv->init_termios.c_iflag = IGNBRK | IGNPAR;
	mtsk_tty_drv->tty_drv->init_termios.c_oflag = 0;
	mtsk_tty_drv->tty_drv->init_termios.c_cflag =
        B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	mtsk_tty_drv->tty_drv->init_termios.c_lflag = 0;

	tty_set_operations(mtsk_tty_drv->tty_drv, &mtsk_tty_ops);
	tty_port_link_device(mtsk_tty_drv->mts_tty_port, mtsk_tty_drv->tty_drv, 0);
	ret = tty_register_driver(mtsk_tty_drv->tty_drv);

	if (ret) {
        printk("fail to mts tty_register_driver\n");
		put_tty_driver(mtsk_tty_drv->tty_drv);
		tty_port_destroy(mtsk_tty_drv->mts_tty_port);	
		mtsk_tty_drv->tty_drv = NULL;
        kfree(mtsk_tty_drv->mts_tty_port);
		kfree(mtsk_tty_drv);
		return 0;
	}

	tty_dev = tty_register_device(mtsk_tty_drv->tty_drv, 0, NULL);

	if (IS_ERR(tty_dev)) {
        printk("fail to mts tty_register_device\n");
		tty_unregister_driver(mtsk_tty_drv->tty_drv);
		put_tty_driver(mtsk_tty_drv->tty_drv);
		tty_port_destroy(mtsk_tty_drv->mts_tty_port);	
        kfree(mtsk_tty_drv->mts_tty_port);
		kfree(mtsk_tty_drv);
		return 0;
	}

	mtsk_tty->pm_notify.notifier_call = mts_pm_notify;
	register_pm_notifier(&mtsk_tty->pm_notify);
	init_waitqueue_head(&mtsk_tty->waitq);

	mtsk_tty->run = 0;
	mtsk_tty->pm_notify_info = 0;

	printk( "mtsk_tty_init success\n");
	return 0;
}

static void __exit mtsk_tty_exit(void)
{
	int ret = 0;
	struct mts_tty *mtsk_tty_drv = NULL;

	mtsk_tty_drv = mtsk_tty;

	if (!mtsk_tty_drv) {
		printk(": %s:" "NULL mtsk_tty_drv", __func__);
		return;
	}
	tty_port_destroy(mtsk_tty_drv->mts_tty_port);	
	mdelay(20);
	tty_unregister_device(mtsk_tty_drv->tty_drv, 0);
	ret = tty_unregister_driver(mtsk_tty_drv->tty_drv);
	put_tty_driver(mtsk_tty_drv->tty_drv);
	mtsk_tty_drv->tty_drv = NULL;
    kfree(mtsk_tty_drv->mts_tty_port);
	kfree(mtsk_tty_drv);
	mtsk_tty = NULL;

	unregister_pm_notifier(&mtsk_tty->pm_notify);

	printk( "mtsk_tty_exit  SUCESS %d - %d\n", 0, 0);
	return;
}

module_init(mtsk_tty_init);
module_exit(mtsk_tty_exit);

MODULE_DESCRIPTION("LGE MTS TTY");
MODULE_LICENSE("GPL");
MODULE_AUTHOR(" lg-msp TEAM <lg-msp@lge.com>");

