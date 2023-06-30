#ifndef __DBG_CTL_H__
#define __DBG_CTL_H__

#include "utils/includes.h"
#include "utils/common.h"
#include "utils/list.h"

#define L_MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define L_MAC2STR(m) (m)[0],(m)[1],(m)[2],(m)[3],(m)[4],(m)[5]

void pmkid_lost_list_add(u8 *mac_addr);
void pmkid_lost_list_del(u8 *mac_addr);
void pmkid_lost_list_check(u8 *mac_addr);
int pmkid_lost_list_dump2buf(char *buf, int buf_len);

int dbg_ctl_run(char *cmd, char *buf, size_t buflen);

int dbg_ctl_init(void);

#endif //__DBG_CTL_H__

