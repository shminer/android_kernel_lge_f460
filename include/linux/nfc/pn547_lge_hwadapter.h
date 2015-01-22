#ifndef _PN547_LGE_HWADAPTER_H_
#define _PN547_LGE_HWADAPTER_H_

#include <linux/nfc/pn547_lge.h>

#include <linux/of_gpio.h>
#include <linux/clk.h>

int pn547_get_hw_revision(void);
unsigned int pn547_get_irq_pin(struct pn547_dev *dev);
int pn547_gpio_to_irq(struct pn547_dev *dev);
void pn547_gpio_enable(struct pn547_dev *pn547_dev);
void pn547_shutdown_cb(struct pn547_dev *pn547_dev);
void pn547_parse_dt(struct device *dev, struct pn547_dev *pn547_dev);
#endif /*                         */
