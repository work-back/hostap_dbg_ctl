#include "dbg_ctl.h"


int main(void)
{
    char cmd[1024] = {0};
    char buf[1024] = {0};

    dbg_ctl_init();

    snprintf(cmd, sizeof(cmd), "%s", "pmkid_lost ADD 11:22:33:44:55:66");
    dbg_ctl_run(cmd, buf, sizeof(buf));
    wpa_printf(MSG_ERROR, "RESP ====> %s", buf);

    snprintf(cmd, sizeof(cmd), "%s", "pmkid_lost");
    dbg_ctl_run(cmd, buf, sizeof(buf));
    wpa_printf(MSG_ERROR, "RESP ====> %s", buf);

    return 0;
}
