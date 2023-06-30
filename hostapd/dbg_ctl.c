#include "dbg_ctl.h"

struct dl_list pmkid_lost_list;

typedef struct dbg_ctl_sta {
    struct dl_list node;
    u8 mac_addr[ETH_ALEN]; 
} dbg_ctl_sta_t;

static int compare_ether_addr(const u8 *addr1, const u8 *addr2)
{
	const u16 *a = (const u16 *) addr1;
	const u16 *b = (const u16 *) addr2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) != 0;
}


void pmkid_lost_list_add(u8 *mac_addr)
{
    dbg_ctl_sta_t *p = NULL, *n = NULL;

    dl_list_for_each(p, &pmkid_lost_list, dbg_ctl_sta_t, node) {
        if (!compare_ether_addr(mac_addr, p->mac_addr)) {
            goto exit_add;
        }
    }

    n = os_malloc(sizeof(dbg_ctl_sta_t));
    if (!n) {
        goto exit_add;
    }
    memcpy(n->mac_addr, mac_addr, ETH_ALEN);

    dl_list_add(&pmkid_lost_list, &(n->node));

exit_add:
    return;
}

void pmkid_lost_list_del(u8 *mac_addr)
{
    dbg_ctl_sta_t *p = NULL, *n = NULL;

    dl_list_for_each_safe(p, n, &pmkid_lost_list, dbg_ctl_sta_t, node) {
        if (!compare_ether_addr(mac_addr, p->mac_addr)) {
            dl_list_del(&(p->node));
            break;
        }
    }

    return;
}

int pmkid_lost_list_dump2buf(char *buf, int buf_len)
{
    int ret = 0;
    dbg_ctl_sta_t *p = NULL;

    wpa_printf(MSG_ERROR, "%s", "pmkid lost sta list"); 
    ret += os_snprintf(buf + ret, buf_len - ret, "%s", "pmkid lost sta list:\n"); 

    dl_list_for_each(p, &pmkid_lost_list, dbg_ctl_sta_t, node) {
        wpa_printf(MSG_ERROR, L_MAC_FMT, L_MAC2STR(p->mac_addr));
        ret += os_snprintf(buf + ret, buf_len - ret, L_MAC_FMT"\n", L_MAC2STR(p->mac_addr)); 
    }

    return ret;
}

int pmkid_lost_list_check(u8 *mac_addr)
{
    int ret = 0;
    dbg_ctl_sta_t *p = NULL;

    dl_list_for_each(p, &pmkid_lost_list, dbg_ctl_sta_t, node) {
        if (!compare_ether_addr(mac_addr, p->mac_addr)) {
            ret = 1;
            break;
        }
    }

    return ret;
}

int skip_wpa_auth_test = 0;

static int dbg_ctl_pmkid_lost_op(char *value)
{
    char *op = value;
    char *mac = NULL;

    wpa_printf(MSG_ERROR, "op buf ====> [%s]", value);
    
    /* value: [OP MAC]" */
    mac = os_strchr(op, ' ');
	if (mac) {
		*mac++ = '\0';
		while (*mac == ' ') {
			mac++;
		}
	}

    wpa_printf(MSG_ERROR, "op: [%s], mac:[%s]", op, mac);

    if (!mac || !os_strlen(mac)) {
        return -1;
    }

    do {
        int rc = -1;
        unsigned int m[6] = {0};
        u8 m2[6] = {0};

        rc = sscanf(mac, L_MAC_FMT, &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
        if (rc != 6) {
            break;
        }

        m2[0] = (u8)m[0]; m2[1] = (u8)m[1]; m2[2] = (u8)m[2];
        m2[3] = (u8)m[3]; m2[4] = (u8)m[4]; m2[5] = (u8)m[5];

        wpa_printf(MSG_ERROR, "%s ====> "L_MAC_FMT, op, L_MAC2STR(m2));

        if (!os_strncmp(op, "ADD", 3)) {
            pmkid_lost_list_add(m2);
        } else if (!os_strncmp(op, "DEL", 3)) {
            pmkid_lost_list_del(m2);
        }
    } while(0);

    return 0;
}

int dbg_ctl_run(char *cmd, char *buf, size_t buflen)
{
	char *pos, *end, *value;
	int ret = 0;

	/* cmd: DBG_CTL [CMD] [<val>]" */
	if (*cmd == '\0') {
		return ret;
	}

    wpa_printf(MSG_ERROR, "cmd buf @ [%s]", cmd);

	while (*cmd == ' ')
		cmd++;

	value = os_strchr(cmd, ' ');
	if (value) {
		*value++ = '\0';
		while (*value == ' ') {
			value++;
		}
	}
	/* value: [<val>]" */

    wpa_printf(MSG_ERROR, "cmd ====> [%s]", cmd);

    if (os_strlen(cmd)) {
        if (os_strncmp(cmd, "skip_wpa", 8) == 0) {
            wpa_printf(MSG_ERROR, "run ====> skip_wpa");
            if (value && os_strlen(value)) {
                skip_wpa_auth_test = atoi(value);
            } else {
                pos = buf;
                end = buf + buflen;
                ret = os_snprintf(pos, end - pos, "[%s] => [%d]\n", "skip_wpa", skip_wpa_auth_test);
                if (os_snprintf_error(end - pos, ret))
                    ret = 0;

                return ret;
            }
        } else if (os_strncmp(cmd, "pmkid_lost", 10) == 0) {
            wpa_printf(MSG_ERROR, "run ====> pmkid_lost");
            if (value && os_strlen(value)) {
                dbg_ctl_pmkid_lost_op(value);
            } else {
                pos = buf;
                end = buf + buflen;
                ret = pmkid_lost_list_dump2buf(pos, end - pos);
                if (os_snprintf_error(end - pos, ret))
                    ret = 0;
                return ret;
            }
        } else {
            pos = buf;
            end = buf + buflen;
            ret = os_snprintf(pos, end - pos, "Unknown dbg_ctl cmd: [%s]", cmd);
            if (os_snprintf_error(end - pos, ret))
                ret = 0;
            return ret;
        }
    }

	os_memcpy(buf, "OK\n", 3);
	return 3;
}

int dbg_ctl_init(void)
{
    dl_list_init(&pmkid_lost_list);

    return 0;
}

