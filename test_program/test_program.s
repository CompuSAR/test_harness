CARRY    = %00000001
ZERO     = %00000010
INTMASK  = %00000100
DECIMAL  = %00001000
BRK      = %00100000
OVERFLOW = %01000000
NEGATIVE = %10000000

FINISHED_TRIGGER        = $200
READY_TRIGGER_COUNT     = $280
READY_TRIGGER_DELAY     = $281
SO_TRIGGER_COUNT        = $282
SO_TRIGGER_DELAY        = $283
NMI_TRIGGER_COUNT       = $2fa
NMI_TRIGGER_DELAY       = $2fb
RESET_TRIGGER_COUNT     = $2fc
RESET_TRIGGER_DELAY     = $2fd
IRQ_TRIGGER_COUNT       = $2fe
IRQ_TRIGGER_DELAY       = $2ff

value_dump = $ff00

    .org $0000
                .byte $5e       ; eor zp,x and (zp,x) tests (MSB)

    .org $000a
    .byte $cc, $d2              ; cmp zp,x test

    .org $000e
bit_zp_test:    .byte $f3

    .org $0014
                .byte $01       ; dec zp,x test

    .org $0028
                .byte $7a       ; bit zp,x test
                .byte $6e       ; asl zp,x test

sta_zp_test:    .byte 0, 0, $34

    .org $002e
rmb_zp_test:    .byte $65, $65 ^ $ff
smb_zp_test:    .byte $f0, $f0 ^ $ff

    .org $0034
                .byte $a8       ; ldy zp,x
    .org $003f
ldy_zp_test:    .byte $2c

    .org $004e
rol_zp_test:    .byte $41, $9b, $60

    .org $005f
eor_zp_test:
                .byte eor_test_zp_ref % 256, eor_test_zp_ref / 256

    .org $0066
trb_zp_test:    .byte $75, $42
tsb_zp_test:    .byte $a3

    .org $0069
asl_zp_test:    .byte $d3
    .org $0074
dec_zp_test:    .byte $f8

    .org $0078
ldx_zp_test:    .byte $4d

    .org $0092
                .byte $6e       ; lsr zp,x

    .org $0099
branch_bit_test: .byte $50

                .byte $f5       ; ldx zp,y

    .org $009d
lsr_zp_test:    .byte $7e

    .org $00a9
lda_zp_test:    .word lda_indirect_test
    .org $00e9
                .byte $da       ; Shadow of $00a9

    .org $00ec
adc_zp_test:
    .byte $88, $d5, $13
inc_zp_test:
    .byte $00c2

cmp_zp_test:
    .byte $4f, $0a, $8f

    .org $00ff
                .byte $e6       ; eor zp,x and (zp,x) tests (LSB)

    .org $0100
    .dc $ff,$7a         ; Put stack in known state
    .byte $7a           ; Due to a limitation of our lst parser, need to mark the end point

    .org $0300

start:
    nop
    nop
    nop
    jsr flags_dump

    lda #$03
    jsr flags_dump
    lda lda_zp_test     ; Make sure we don't treat the operand as an opcode
    jsr flags_dump

    lda lda_abs_test
    jsr flags_dump

    jsr branch_boundary_test

    ldx #$c0
    ldy #$30
    .if C02
    phx
    phy
    .else
    txa
    pha
    tya
    pha
    .endif

    ; Direct flags manipulation
    lda #$ff
    sta $1fe
    .if C02
    stz $1ff
    .else
    lda #0
    sta $1ff
    .endif

    plp
    jsr flags_dump
    plp
    jsr flags_dump
    sec
    jsr flags_dump
    sed
    jsr flags_dump
    sei
    jsr flags_dump
    clc
    jsr flags_dump
    cld
    jsr flags_dump
    cli
    jsr flags_dump
    clv
    jsr flags_dump

    ; Test addressing modes
    lda lda_abs_test
    lda lda_abs_test,x          ; No page transition
    lda lda_abs_test-$c0,x      ; With page transition
    lda lda_abs_test,y          ; No page transition
    lda lda_abs_test-$30,y      ; With page transition
    lda lda_zp_test
    .if C02
    lda (lda_zp_test+$100-$c0,x)
    .endif
    lda lda_zp_test+$100-$c0,x
    .if C02
    lda (lda_zp_test)
    .endif
    lda (lda_zp_test),y         ; No page transition
    ldy #$f0
    lda (lda_zp_test),y         ; With page transition


    ; ASL test
    lda #1
asl_loop:
    asl asl_abs_test
    jsr flags_dump
    asl asl_abs_test,x
    jsr flags_dump
    asl asl_zp_test
    jsr flags_dump
    asl asl_zp_test,x
    jsr flags_dump
    asl
    jsr flags_dump
    bne asl_loop

    sta sta_zp_test     ; Just somewhere to keep A's initial value
    ldx #(adc_tests_ret1 & 0xff)
    stx stored_ret
    ldx #(adc_tests_ret1 >> 8)
    stx stored_ret+1

    jmp adc_tests
adc_tests_ret1:

    sed
    ldx #(adc_tests_ret2 & 0xff)
    stx stored_ret
    ldx #(adc_tests_ret2 >> 8)
    stx stored_ret+1
    lda sta_zp_test

    jmp adc_tests
adc_tests_ret2:
    cld


    ; BIT test
    lda #$4f
    php
    ldx #$1a
    php
    ldy #$22
    php

    bit bit_abs_test
    php
    .if C02
    bit bit_abs_test,x
    php
    bit #$b0
    php
    bit bit_zp_test,x
    php
    .endif
    bit bit_zp_test
    php

    ; BRK test
    sed
    cli
    brk
    .byte $1
    cld
    sei
    brk
    .byte $2

    ; CMP test
    cmp cmp_abs_test
    php
    cmp cmp_abs_test,x
    php
    cmp cmp_abs_test,y
    php
    cmp #$db
    php
    cmp cmp_zp_test
    php
    cmp (cmp_zp_test,x)
    php
    cmp cmp_zp_test,x
    php
    .if C02
    cmp (cmp_zp_test)
    php
    .endif
    cmp (cmp_zp_test),y
    php
    cmp #$4e
    php
    cmp #$4f
    php
    cmp #$50
    php

    ; CPX test
    cpx cmp_abs_test
    php
    cpx #$db
    php
    cpx cmp_zp_test
    php
    cpx #$19
    php
    cpx #$1a
    php
    cpx #$1b
    php
    ldx #$a0
    cpx #$0
    php

    ; CPY test
    cpy cmp_abs_test
    php
    cpy #$db
    php
    cpy cmp_zp_test
    php
    cpy #$19
    php
    cpy #$1a
    php
    cpy #$1b
    php
    ldx #$a0
    cpy #$0
    php

    ; DEC test
    dec dec_abs_test
    php
    dec dec_abs_test,x
    php
    dec dec_zp_test
    php
    dec dec_zp_test,x
    php

    ldx #$12
    dec dec_abs_test,x
    php

    ldx #$a0

dec_loop:
    .if C02
    dec
    php
    bne dec_loop
    dec
    php
    .endif

    jsr bb_test
    lda branch_bit_test
    eor #$ff
    sta branch_bit_test
    jsr bb_test

    ; EOR test
    php
    eor eor_abs_test
    pha
    php
    eor eor_abs_test,x
    pha
    php
    eor eor_abs_test,y
    pha
    php
    eor #$f4
    pha
    php
    eor eor_zp_test
    pha
    php
    eor (eor_zp_test,x)
    pha
    php
    eor eor_zp_test,x
    pha
    php
    .if C02
    eor (eor_zp_test)
    pha
    php
    .else
    eor eor_test_zp_ref
    pha
    php
    .endif
    eor (eor_zp_test),y
    pha
    php
    eor #$3c
    pha
    php


    ; inc tests
    .if C02
    dec
    dec
    .else
    clc
    sbc #2
    .endif
    ldx #$3

inc_loop:
    inc inc_abs_test
    php
    inc inc_abs_test,x
    php
    .if C02
    inc
    php
    .endif
    inc inc_zp_test
    php
    inc inc_zp_test,x
    php

    dex
    bne inc_loop

    .if C02
    jmp jmp_tests_c02
    .else
    jmp jmp_tests_mos
    .endif
    brk                 ; Unreachable
jmp_test_continues:
    php


    ; LDX test
    ldx ldx_abs_test
    php
    stx value_dump
    ldx ldx_abs_test,y
    php
    stx value_dump
    ldx ldx_abs_test-$22,y
    php
    stx value_dump
    ldx #$04
    php
    stx value_dump
    ldx ldx_zp_test
    php
    stx value_dump
    ldx ldx_zp_test,y
    php
    stx value_dump


    ; LDY test
    ldy ldy_abs_test
    php
    sty value_dump
    ldy ldy_abs_test,x
    php
    sty value_dump
    ldy #$6c
    php
    sty value_dump
    ldy ldy_zp_test
    php
    sty value_dump
    ldy ldy_zp_test,x
    php
    sty value_dump


    ; LSR test
    lsr lsr_abs_test
    php
    lsr lsr_abs_test,x
    php
    lsr
    php
    sta value_dump
    lsr lsr_zp_test
    php
    lsr lsr_zp_test,x
    php


    ; Stack pull tests
    ldy #0
    lda #$fc
    sta $1a3    ; A
    sty $1a4
    lda #$55
    sta $1a5    ; A
    lda #$dd
    sta $1a6    ; Y
    sty $1a7    ; A
    lda #$03
    sta $1a8    ; Y
    lda #$9b
    sta $1a9    ; X
    lda #$2d
    sta $1aa    ; X
    sty $1ab    ; X

    ldx #$03
pull_test_loop1:
    pla
    sta value_dump
    php
    plp
    .if C02
    ply
    .else
    ; Kinda pointless, as it doesn't test anything, but let's keep things consistent
    pla
    tay
    .endif
    sty value_dump
    php
    plp

    dex
    bne pull_test_loop1

pull_test_loop2:
    .if C02
    plx
    .else
    ; Also kinda pointless
    pla
    tax
    .endif
    stx value_dump
    php
    plp

    dey
    bne pull_test_loop2


    .if CPU_WDC
    ; RMB/SMB test
    rmb 0,rmb_zp_test
    rmb 0,rmb_zp_test+1
    rmb 1,rmb_zp_test
    rmb 1,rmb_zp_test+1
    rmb 2,rmb_zp_test
    rmb 2,rmb_zp_test+1
    rmb 3,rmb_zp_test
    rmb 3,rmb_zp_test+1
    rmb 4,rmb_zp_test
    rmb 4,rmb_zp_test+1
    rmb 5,rmb_zp_test
    rmb 5,rmb_zp_test+1
    rmb 6,rmb_zp_test
    rmb 6,rmb_zp_test+1
    rmb 7,rmb_zp_test
    rmb 7,rmb_zp_test+1

    smb 0,smb_zp_test
    smb 0,smb_zp_test+1
    smb 1,smb_zp_test
    smb 1,smb_zp_test+1
    smb 2,smb_zp_test
    smb 2,smb_zp_test+1
    smb 3,smb_zp_test
    smb 3,smb_zp_test+1
    smb 4,smb_zp_test
    smb 4,smb_zp_test+1
    smb 5,smb_zp_test
    smb 5,smb_zp_test+1
    smb 6,smb_zp_test
    smb 6,smb_zp_test+1
    smb 7,smb_zp_test
    smb 7,smb_zp_test+1
    .endif


    ; ROL/ROR test
    ldx #1
    lda #$af
    rol rol_abs_test
    php
    rol
    php
    sta value_dump
    ror rol_abs_test+1
    php
    rol rol_abs_test,x
    php
    ror rol_abs_test+1,x
    php
    ror
    php
    sta value_dump
    rol rol_zp_test
    php
    ror rol_zp_test+1
    php
    rol rol_zp_test,x
    php
    ror rol_zp_test+1,x
    php


    ; STA test
    ldy #$02
    sta sta_abs_test
    .if C02
    inc
    .else
    adc #1
    .endif
    sta sta_abs_test,x
    .if C02
    inc
    .else
    adc #1
    .endif
    sta sta_abs_test,y
    .if C02
    inc
    .else
    adc #1
    .endif
    sta sta_zp_test
    .if C02
    inc
    .else
    adc #1
    .endif
    sta sta_zp_test,x
    .if C02
    inc
    .else
    adc #1
    .endif
    sta (sta_zp_test,x)
    .if C02
    inc
    .else
    adc #1
    .endif
    .if C02
    sta (sta_zp_test)
    inc
    .endif
    sta (sta_zp_test),y
    .if C02
    inc
    .else
    adc #1
    .endif
    ldx #$80
    sta sta_abs_test,x
    .if C02
    inc
    .else
    adc #1
    .endif
    ldy #$ff
    sta sta_abs_test,y
    .if C02
    inc
    .else
    adc #1
    .endif
    sta (sta_zp_test),y
    php


    ; STX test
    stx sta_abs_test
    inx
    stx sta_zp_test+1
    inx
    stx sta_zp_test+1,y
    php


    ; STY test
    ldx #$2
    sty sta_abs_test
    iny
    sty sta_zp_test
    iny
    sty sta_zp_test,x
    php


    ; STZ test
    .if C02
    stz sta_abs_test
    stz sta_abs_test,x
    stz sta_zp_test
    stz sta_zp_test,x
    php
    .endif


    ; Transfer test
    lda #$85
    jsr transfer_tests
    lda #$00
    jsr transfer_tests

    tsx
    jsr dump_state
    ldx #$ff
    lda #$00
    txs
    jsr dump_state


    ; TSB/TRB test
    .if C02
    lda #$89
    trb trb_abs_test
    jsr dump_state
    trb trb_abs_test+1
    jsr dump_state

    tsb tsb_abs_test
    jsr dump_state

    trb trb_zp_test
    jsr dump_state
    trb trb_zp_test+1
    jsr dump_state

    tsb tsb_zp_test
    jsr dump_state

    lda #$00
    tsb trb_abs_test+1
    jsr dump_state
    tsb tsb_zp_test+1
    jsr dump_state
    .endif


    jsr regression1_apple2_disassembly


    ; IRQ test
    lda #$6
    sta IRQ_TRIGGER_COUNT
    sta IRQ_TRIGGER_DELAY
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    lda #30
    sta IRQ_TRIGGER_COUNT
    ldx #4
    stx IRQ_TRIGGER_DELAY
    nop
    nop
    nop
    nop
    cli
    nop
    nop
    nop
    nop
    nop
    nop


    ; NMI test
    lda #40
    sta NMI_TRIGGER_COUNT
    sta IRQ_TRIGGER_COUNT
    ldx #15
    ldy #11
    stx IRQ_TRIGGER_DELAY
    sty NMI_TRIGGER_DELAY

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop


    ; Ready and SO tests
    clv

    lda #10
    sta READY_TRIGGER_COUNT
    lda #2
    sta SO_TRIGGER_COUNT
    ldx #9
    ldy #30
    stx READY_TRIGGER_DELAY
    sty SO_TRIGGER_DELAY

so_test_loop:
    bvc so_test_loop
    

    ; STP test
    lda #(stp_test_cont1 % 256)
    sta reset_vector
    lda #(stp_test_cont1 / 256)
    sta reset_vector+1
    .if C02
    lda #$04
    sta RESET_TRIGGER_COUNT
    lda #$10
    sta RESET_TRIGGER_DELAY

    stp
    .endif

stp_test_cont1:
    jsr dump_state

    lda #(stp_test_cont2 % 256)
    sta reset_vector
    lda #(stp_test_cont2 / 256)
    sta reset_vector+1

    lda #$ff
    pha
    plp
    jsr dump_state

    lda #$04
    sta RESET_TRIGGER_COUNT
    sta RESET_TRIGGER_DELAY

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

stp_test_cont2:
    ; We don't care about the status flags, only D and I
    lda #$00
    clv
    clc
    jsr dump_state

    lda #(stp_test_cont3 % 256)
    sta reset_vector
    lda #(stp_test_cont3 / 256)
    sta reset_vector+1

    lda #$00
    pha
    plp
    jsr dump_state

    lda #$04
    sta RESET_TRIGGER_COUNT
    sta RESET_TRIGGER_DELAY

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

stp_test_cont3:
    ; We don't care about the status flags, only D and I
    lda #$00
    clv
    clc
    jsr dump_state


    ; WAI tests
    .if CPU_WDC
    cli
    lda #6
    sta IRQ_TRIGGER_COUNT
    lda #10
    sta IRQ_TRIGGER_DELAY
    wai

    sei
    lda #6
    sta IRQ_TRIGGER_COUNT
    lda #10
    sta IRQ_TRIGGER_DELAY
    wai

    lda #6
    sta NMI_TRIGGER_COUNT
    lda #10
    sta NMI_TRIGGER_DELAY
    wai
    .endif


    sta FINISHED_TRIGGER
    .byte 00

transfer_tests:
    jsr dump_state
    ldy #$01
    tay
    jsr dump_state
    ldx #$02
    tax
    jsr dump_state
    eor #$ff
    ldy #$01
    tya
    jsr dump_state
    ldx #$01
    txa
    jsr dump_state
    rts


dump_state:
    php
    sta value_dump
    stx value_dump
    sty value_dump
    plp
    rts
    .db $22

    .org $0800
adc_tests:
    ; ADC tests
    ldx #2
    ldy #1
adc_loop:
    adc adc_abs_test
    pha
    php
    and adc_abs_test
    pha
    php
    adc adc_abs_test,x
    pha
    php
    and adc_abs_test,x
    pha
    php
    adc adc_abs_test,y
    pha
    php
    and adc_abs_test,y
    pha
    php
    adc #$cd
    pha
    php
    and #$a7
    pha
    php
    adc adc_zp_test
    pha
    php
    and adc_zp_test
    pha
    php
    adc (adc_zp_test,x)
    pha
    php
    and (adc_zp_test,x)
    pha
    php
    adc adc_zp_test,x
    pha
    php
    and adc_zp_test,x
    pha
    php
    .if C02
    adc (adc_zp_test)
    pha
    php
    and (adc_zp_test)
    pha
    php
    .endif
    adc (adc_zp_test),y
    pha
    php
    and (adc_zp_test),y
    pha
    php
    iny
    dex
    bne adc_loop

sbc_loop:
    stx value_dump
    sty value_dump
    sta value_dump
    php

    sbc adc_abs_test
    pha
    php
    ora adc_abs_test
    pha
    php
    sbc adc_abs_test,x
    pha
    php
    ora adc_abs_test,x
    pha
    php
    sbc adc_abs_test,y
    pha
    php
    ora adc_abs_test,y
    pha
    php
    sbc #$cd
    pha
    php
    ora #$cd
    pha
    php
    sbc adc_zp_test
    pha
    php
    ora adc_zp_test
    pha
    php
    sbc (adc_zp_test,x)
    pha
    php
    ora (adc_zp_test,x)
    pha
    php
    sbc adc_zp_test,x
    pha
    php
    ora adc_zp_test,x
    pha
    php
    .if C02
    sbc (adc_zp_test)
    pha
    php
    ora (adc_zp_test)
    pha
    php
    .endif
    sbc (adc_zp_test),y
    pha
    php
    ora (adc_zp_test),y
    pha
    php
    inx
    dey
    bne sbc_loop

    jmp (stored_ret)

stored_ret: dw 0

reset_handler:
    ldx #$ff
    txs
    lda #start/256
    pha
    lda #start%256
    pha
    lda #INTMASK+OVERFLOW
    pha
    rti
    brk         ; Unreachable


    .org $0a4f
    .byte $8f           ; cmp (zp) test

    .org $0a71
    .byte $25           ; cmp (zp),y test

    .org $13d5
    .byte $dd           ; adc (zp,x) test

    .org $1690
bit_abs_test:
    .byte $6b

    .org $16aa
    .byte $03           ; bit abs,x

    .org $1910
    .byte $66           ; Shadow of 1a10

    .org $1a10
ldx_abs_test:
    .byte $6d

    .org $1a32
    .byte $d7           ; ldx abs,y

    .org $26ff
jmp_ind_test2:
    .word jmp_dest2
    .byte 0
    .word jmp_dest4

    .org $2746
jmp_ind_cont:
    .word jmp_test_continues

    .org $274f
    .word jmp_dest5

    .org $27b8
jmp_ind_test1:
    .word jmp_dest1
    .byte 0
    .word jmp_dest3

    .org $27ff
    .word jmp_dest3

    .org $2808
    .word jmp_dest3

    .org $29ff
    .word jmp_dest_mos

    .org $2aff
jmp_ind_test_mos:
    .word jmp_dest_mos_error

    .org $3001
jmp_dest_mos:
    jmp (jmp_ind_cont)
    brk                 ; Unreachable

    .org $3201
jmp_dest_mos_error:
    brk                 ; Should never reach here

    .org $3331
    .byte $a9, $f2, $fc, $ae ; Dummy read in zp indirect test

    .org $3630
branch_boundary_test:
    sec
    bcs .1
.2  clc
    bcc .3
.4  clv
    bvc .5
.7  .db $7f
.6  lda #2
    adc .7
    bvs .8
.9  bmi .10
.11 bne .12
.13 lda #0
    beq .14
.15 bpl .16
.17 rts
    .db $00

    .org $3682
.1  bcs .2
.3  bcc .4
.5  bvc .6
.8  bvs .9
.10 bmi .11
.12 bne .13
.14 beq .15
.16 bpl .17
    .db $00

    .org $3787
cmp_abs_test:
    .byte $b8

    .org $37a1
    .byte $6f

    .org $37a9
    .byte $0e

    .org $40ef
flags_dump:
    php
    bcs .1
    bcc .1

.2  bvs .3
    bvc .3

    .if C02
.4  bra .5
    .else
.4  jmp .5
    .endif

.3  bne .4
    beq .4

.1  bmi .2
    bpl .2

.5
    plp
    rts
    .byte 00

    .org $41ef
bb_test:
    ; Branch bit tests
    .if CPU_WDC
    bbr 0, branch_bit_test, .1
    bbs 0, branch_bit_test, .1

.2  bbr 2, branch_bit_test, .3
    bbs 2, branch_bit_test, .3

.4  bbr 4, branch_bit_test, .5
    bbs 4, branch_bit_test, .5

.6  bbr 6, branch_bit_test, .7
    bbs 6, branch_bit_test, .7

.8  rts

.7  bbr 7, branch_bit_test, .8
    bbs 7, branch_bit_test, .8

.5  bbr 5, branch_bit_test, .6
    bbs 5, branch_bit_test, .6

.3  bbr 3, branch_bit_test, .4
    bbs 3, branch_bit_test, .4

.1  bbr 1, branch_bit_test, .2
    bbs 1, branch_bit_test, .2
    .else
    rts
    .endif

    brk         ; Unreachable

int_handler:
    php
    plp
    rti
    brk         ; Unreachable

nmi_handler:
    jmp int_handler
    brk         ; Unreachable

    .org $4614
                .byte $ad                       ; Shadow of $4714 for sta_abs_test,x

    .org $4693
                .byte $bd                       ; Shadow of 4794 for sta_abs_test,y
sta_abs_test:   .byte $87, $91, $20

    .org $55aa
jmp_tests_mos:
    jmp (jmp_ind_test_mos)
    brk         ; Unreachable


jmp_tests_c02:
    ldx #$3

    jmp (jmp_ind_test1)
    brk         ; Unreachable

    .org $5ee6
                .byte $3f                       ; eor (zp,x) test

    .org $60ff
                .byte $6d, $bc, $8e, $87        ; Shadow of 61ff

    .org $617a
                .byte $7d                       ; Shadow of $627a

    .org $61da
dec_abs_test    .byte $7b

    .org $61ec
                .byte $90 ; Second dec abs,x test

    .org $61ff
inc_abs_test    .byte $fe, $30, $22, $48        ; inc abs, int abs,x tests

    .org $627a
                .byte $01 ; dec abs,x test

    .org $6c21
                .byte $a8       ; Shadow of $6d21

    .org $6d21
lda_abs_test    .byte $74
    .org $6d51
                .byte $c7       ; lda abs,y
    .org $6de1
                .byte $08       ; lda abs,x

    .org $7088
eor_test_zp_ref
                .byte $29       ; eor (zp)
    .org $70aa
                .byte $50       ; eor (zp),y

    .org $7ace
adc_abs_test:   .byte $65, $ca, $26, $6b

    .org $7d15
trb_abs_test:   .byte $d7, $36
tsb_abs_test:   .byte $a5, $00

    .org $7e9d
    .byte $98                   ; Shadow of 7f9d

    .org $7ea8
lsr_abs_test:
    .byte $9b

    .org $7f9d
    .byte $49                   ; lsr abs,x

    .org $8510
asl_abs_test:   .byte $56
    .org $85d0
                .byte $40       ; asl abs,x test

    .org $a303
jmp_dest2:
    .if C02
                jmp (jmp_ind_test1,x)
    .endif
                brk             ; Unreachable

jmp_dest3:
    .if C02
                jmp (jmp_ind_test2,x)
    .endif
                brk

jmp_dest5:
                ldx #$47
                jmp jmp_dest1
                brk             ; Unreachable

jmp_dest4:
                ldx #$50

jmp_dest1:
                jmp (jmp_ind_test2)
                brk             ; Unreachable

    .org $ae28
                .byte $2b       ; Shadow of $af28

    .org $ae38
lda_indirect_test .byte $bf
    .org $ae68
                .byte $20       ; lda (zp),y test
    .org $af28
                .byte $22       ; lda (zp),y test

    .org $c213
    .byte $25                   ; adc (zp,x) test

    .org $d014
    .byte $01                   ; Shadow of $d114

    .org $d074
eor_abs_test    .byte $17

    .org $d096
                .byte $11        ; eor abs,y

    .org $d114
                .byte $49       ; eor abs,x test

    .org $d2cc
    .byte $38                   ; cmp (zp,x) test

    .org $d588
    .byte $15                   ; adc (zp,x) test
    .byte $8e, $9b, $f5         ; adc (zp),y test

    .org $e308
ldy_abs_test:
    .byte $ff

    .org $e3fd
    .byte $93                   ; ldy abs,x


    .org $f800
regression1_apple2_disassembly:
    lda #$a9    ; Should have registered as LDA immediate, registers as ???
    jsr .0
    lda #$85    ; Should have registered as STA zp, registers as ???
    jsr .0
    lda #$ad    ; Should have registered as LDA abs, registers as LDA zp
    jsr .0

    rts
    brk         ; Unreachable

    .org $f882
    ; Apple 2 autostart ROM INSDS1 routine
.0
    tay
    lsr
    bcc .1
    ror
    bcs .2
    cmp #$A2
    beq .2
    and #$87
.1  ; IEVEN
    lsr
    tax
    lda FMT1,x
    jsr .8
    bne .5
.2  ; ERR
    ldy #$80
    lda #$00
.5  ; GETFMT
    tax
    lda FMT2,x
    sta $2e ; F8.MASK
    and #$03
    sta $2f ; LENGTH

    rts

.8       bcc     .9
            lsr     A
            lsr     A
            lsr     A
            lsr     A
.9      and     #$0f
            rts

FMT1        .byte   $04,$20,$54,$30,$0d,$80,$04,$90,$03,$22,$54,$33,$0d,$80,$04,$90
            .byte   $04,$20,$54,$33,$0d,$80,$04,$90,$04,$20,$54,$3b,$0d,$80,$04,$90
            .byte   $00,$22,$44,$33,$0d,$c8,$44,$00,$11,$22,$44,$33,$0d,$c8,$44,$a9
            .byte   $01,$22,$44,$33,$0d,$80,$04,$90,$01,$22,$44,$33,$0d,$80,$04,$90
            .byte   $26,$31,$87,$9a
FMT2        .data   $00,$21,$81,$82,$00,$00,$59,$4d,$91,$92,$86,$4a,$85,$9d

    .org $f9f8
rol_abs_test:
    .byte $cd, $71, $e4

    .org $fffa
nmi_vector:     .word nmi_handler
reset_vector:   .word reset_handler
irq_vector:     .word int_handler
