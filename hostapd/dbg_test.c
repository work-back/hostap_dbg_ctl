#include "dbg_ctl.h"

int dbg_ctl_run_fmt(const char *format, ...)
{
    char cmd[1024];
    char buf[1024];
    int len = 0;
    va_list vg;

    va_start(vg, format);
    vsnprintf(cmd, sizeof(cmd), format, vg);
    va_end(vg);

    dbg_ctl_run(cmd, buf, sizeof(buf));
    wpa_printf(MSG_ERROR, "====> %s", buf);

    return 0;
}


int main(void)
{
    char cmd[1024] = {0};
    char buf[1024] = {0};

    dbg_ctl_init();

    dbg_ctl_run_fmt("pmkid_lost ADD 11:22:33:44:55:66");
    dbg_ctl_run_fmt("pmkid_lost ADD 11:22:33:44:55:77");
    dbg_ctl_run_fmt("pmkid_lost TMP 11:22:33:44:55:88 10");

    dbg_ctl_run_fmt("pmkid_lost");

    int i = 0;
    u8 ck_mac[ETH_ALEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x88};
    for (i = 0; i < 15; i++) {
        if(pmkid_lost_list_check(ck_mac)) {
            wpa_printf(MSG_ERROR, "[%d]: IN LIST", i);
        } else {
            wpa_printf(MSG_ERROR, "[%d]: NOT IN LIST", i);
        }
    }

    dbg_ctl_run_fmt("pmkid_lost");
    dbg_ctl_run_fmt("pmkid_lost TMP 11:22:33:44:55:88 10");
    dbg_ctl_run_fmt("pmkid_lost DEL 11:22:33:44:55:66");
    dbg_ctl_run_fmt("pmkid_lost");

    dbg_ctl_run_fmt("pmkid_lost ADD 11:22:33:44:55:66");
    dbg_ctl_run_fmt("pmkid_lost ADD 11:22:33:44:55:77");
    dbg_ctl_run_fmt("pmkid_lost TMP 11:22:33:44:55:88 10");
    dbg_ctl_run_fmt("pmkid_lost");
    dbg_ctl_run_fmt("pmkid_lost CLR");
    dbg_ctl_run_fmt("pmkid_lost");

    dbg_ctl_run_fmt("");
    dbg_ctl_run_fmt("fku");

    dbg_ctl_run_fmt("skip_wpa 1");
    dbg_ctl_run_fmt("skip_wpa");

    return 0;
}
