#include "condor_dev.h"
#include "condor_hw_regs.h"

/**
 * @brief 소켓 버퍼에 있는 데이터를 모두 출력한다.(Hex dump)
 * @param skb 출력할 소켓 버퍼
 * @return
 * */
 void print_packet_dump(struct sk_buff *skb)
{
    /* Debug for sk_buff */
    struct platform_device *pdev = g_pdev;
    uint32_t i, j, linelen, remaining;
    uint32_t line_idx = 0;
    uint32_t rowsize = 16;
    size_t len;
    BYTE *data, ch;

    data = (BYTE *)skb->data;

    if (skb_is_nonlinear(skb))
        len = skb->data_len;
    else
        len = skb->len;
    // dev_info(&pdev->dev, "Packet hex dump %d-bytes\n", len);
    remaining = len;
    printk("Packet length: %d\n", skb->len);
    for (i = 0; i < len; i += rowsize)
    {
        printk("%05x0\t", line_idx);

        linelen = remaining < rowsize ? remaining : rowsize;
        remaining -= rowsize;

        for (j = 0; j < linelen; j++)
        {
            ch = data[j];
            printk(KERN_CONT "%02x ", ch);
        }

        data += linelen;
        line_idx += 1;

        printk(KERN_CONT "\n");
    }
}