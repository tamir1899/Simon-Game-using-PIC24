.include "xc.inc"

.text

.global _thh_wait_100us, _thh_wait_1ms

_thh_wait_1ms:
    nop
    repeat  #3982
    nop
    return

_thh_wait_100us:
    nop
    repeat  #398
    nop
    return
