#include "dbg_ctl.h"

struct dl_list pmkid_lost_list;

#define STA_FLAG_PMKID_LOST   (1 << 0)
#define STA_FLAG_EAPOL_DROP   (1 << 1)
#define STA_FLAG_EAPOL_2_DROP (1 << 2)
#define STA_FLAG_ASSOC_REQ_DROP (1 << 3)

typedef struct dbg_ctl_sta {
    struct dl_list node;
    u8 mac_addr[ETH_ALEN]; 
    u32 flags;
} dbg_ctl_sta_t;

static int compare_ether_addr(const u8 *addr1, const u8 *addr2)
{
	const u16 *a = (const u16 *) addr1;
	const u16 *b = (const u16 *) addr2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) != 0;
}

static void sta_list_add(u8 *mac_addr, u32 f)
{
    dbg_ctl_sta_t *p = NULL, *n = NULL;

    dl_list_for_each(p, &pmkid_lost_list, dbg_ctl_sta_t, node) {
        if (!compare_ether_addr(mac_addr, p->mac_addr)) {
            p->flags |= f;
            goto exit_add;
        }
    }

    n = os_malloc(sizeof(dbg_ctl_sta_t));
    if (!n) {
        goto exit_add;
    }
    memset(n, 0, sizeof(dbg_ctl_sta_t));
    memcpy(n->mac_addr, mac_addr, ETH_ALEN);
    n->flags |= f;

    dl_list_add(&pmkid_lost_list, &(n->node));

exit_add:

    return;
}

static void sta_list_del(u8 *mac_addr, u32 f)
{
    dbg_ctl_sta_t *p = NULL, *n = NULL;

    dl_list_for_each_safe(p, n, &pmkid_lost_list, dbg_ctl_sta_t, node) {
        if (!compare_ether_addr(mac_addr, p->mac_addr)) {
            p->flags &= ~f;
            if (p->flags == 0x0) {
                dl_list_del(&(p->node));
            }
            break;
        }
    }

    return;
}

static int sta_list_dump2buf(char *buf, int buf_len, u32 f)
{
    int ret = 0;
    dbg_ctl_sta_t *p = NULL;

    dl_list_for_each(p, &pmkid_lost_list, dbg_ctl_sta_t, node) {
        if (p->flags & f) {
            ret += os_snprintf(buf + ret, buf_len - ret, L_MAC_FMT"\n", L_MAC2STR(p->mac_addr)); 
        }
    }

    return ret;
}

static int sta_list_check(u8 *mac_addr, u32 f)
{
    int ret = 0;
    dbg_ctl_sta_t *p = NULL;

    dl_list_for_each(p, &pmkid_lost_list, dbg_ctl_sta_t, node) {
        if (!compare_ether_addr(mac_addr, p->mac_addr) && (p->flags & f)) {
            ret = 1;
            break;
        }
    }

    return ret;
}

int pmkid_lost_list_check(u8 *mac_addr)
{
    return sta_list_check(mac_addr, STA_FLAG_PMKID_LOST);
}

int eapol2_drop_check(u8 *mac_addr)
{
    return sta_list_check(mac_addr, STA_FLAG_EAPOL_2_DROP);
}

int assoc_req_drop_check(u8 *mac_addr)
{
    return sta_list_check(mac_addr, STA_FLAG_ASSOC_REQ_DROP);
}

typedef int (*dbg_ctl_cmd_set_cb_t)(void *it, char *value);
typedef int (*dbg_ctl_cmd_get_cb_t)(void *it, char *buf, int buf_len);

typedef struct dbg_ctl_cmd_info {
    const char *name;
    dbg_ctl_cmd_set_cb_t set;
    dbg_ctl_cmd_get_cb_t get;
    u32 flag;
}dbg_ctl_cmd_it;


int skip_wpa_auth_test = 0;

static int dbg_ctl_sta_op(dbg_ctl_cmd_it *it, char *value)
{
    char *op = value;
    char *mac = NULL;

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

        if (!os_strncmp(op, "ADD", 3)) {
            sta_list_add(m2, it->flag);
        } else if (!os_strncmp(op, "DEL", 3)) {
            sta_list_del(m2, it->flag);
        }
    } while(0);

    return 0;
}

int dbg_ctl_list_dump2buf(dbg_ctl_cmd_it *it, char *buf, int buf_len)
{
    int ret = 0;

    ret += os_snprintf(buf + ret, buf_len - ret, "%s sta list:\n", it->name); 

    ret += sta_list_dump2buf(buf + ret, buf_len - ret, it->flag);

    return ret;
}

int dbg_ctl_cmd_skip_wpa_set(void *it, char *value)
{
    skip_wpa_auth_test = atoi(value);

    return 0;
}

int dbg_ctl_cmd_skip_wpa_get(void *it, char *buf, int buf_len)
{
    return os_snprintf(buf, buf_len, "[%s] => [%d]\n", "skip_wpa", skip_wpa_auth_test);
}

int dbg_ctl_cmd_sta_set(void *it, char *value)
{
    return dbg_ctl_sta_op(it, value);
}

int dbg_ctl_cmd_sta_get(void *it, char *buf, int buf_len)
{
    return dbg_ctl_list_dump2buf(it, buf, buf_len);
}

static dbg_ctl_cmd_it cmd_list[] = {
   {"skip_wpa",    dbg_ctl_cmd_skip_wpa_set, dbg_ctl_cmd_skip_wpa_get, 0},
   {"pmkid_lost",  dbg_ctl_cmd_sta_set, dbg_ctl_cmd_sta_get, STA_FLAG_PMKID_LOST},
   {"eapol_drop",  dbg_ctl_cmd_sta_set, dbg_ctl_cmd_sta_get, STA_FLAG_EAPOL_DROP},
   {"eapol2_drop", dbg_ctl_cmd_sta_set, dbg_ctl_cmd_sta_get, STA_FLAG_EAPOL_2_DROP},
   {"assoc_req_drop", dbg_ctl_cmd_sta_set, dbg_ctl_cmd_sta_get, STA_FLAG_ASSOC_REQ_DROP},
};

static dbg_ctl_cmd_it* dbg_ctl_get_cmd(const char *cmd)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(cmd_list); i++) {
        if (!os_strncmp(cmd, cmd_list[i].name, os_strlen(cmd) + 1)) {
            return &(cmd_list[i]);
        }
    }

    return NULL;
}

int dbg_ctl_run(char *cmd, char *buf, size_t buflen)
{
	char *value = NULL;

	/* cmd: DBG_CTL [CMD] [<val>]" */
	if (*cmd == '\0') {
        return os_snprintf(buf, buflen, "Error: %s\n", "cmd is NULL.");
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

    if (!os_strlen(cmd)) {
        return os_snprintf(buf, buflen, "Error: %s\n", "cmd is NULL.");
    }

    dbg_ctl_cmd_it *cmd_it = NULL;
    cmd_it = dbg_ctl_get_cmd(cmd);
    if (!cmd_it) {
        return os_snprintf(buf, buflen, "Error: cmd %s is not support.\n", cmd);
    }

	/* value: [<val>]" */

    if (value && os_strlen(value)) {
        if (cmd_it->set) {
            if (cmd_it->set(cmd_it, value)) {
                return os_snprintf(buf, buflen, "Error: run cmd %s set failed.\n", cmd);
            } else {
                return os_snprintf(buf, buflen, "%s", "OK\n");
            }
        }
    } else {
        if (cmd_it->get) {
            return cmd_it->get(cmd_it, buf, buflen);
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

