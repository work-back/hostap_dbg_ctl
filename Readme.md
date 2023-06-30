# test build 

gcc hostapd/dbg_test.c hostapd/dbg_ctl.c -I src/ -D DBG_CTL_TEST -Wall -o dbg_ctl
