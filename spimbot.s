##############################################################
# Main SPIMbot Code
#
# Taylor Fairbank | Vinay Hiremath
##############################################################



#============================================================
# Main Functionality
#============================================================

.data
    num_puzzles:    .word 0
    ball_to_kick:   .word -1

main:

    # set our interrupts
    li   $t4, 0x6001                  # enable suduko and kick
    mtc0 $t4, $12

    # set up some of our variables
    sw $t1, num_puzzles
    sw $t2, ball_to_kick

main_while:
    # 1.) check if num_puzzles is zero
    #
    #     if (yes):
    #       request X suduko puzzles

    # 2.) check if ball_to_kick is greater than equal to zero
    #
    #     if (yes):
    #       check if ball exists
    #       select ball
    #       check if ball is kickable
    #       if (yes):
    #           kick ball

    # 3.) check closest ball that exists on field
    #
    #     calculate arc tan to ball
    #     run towards ball

    # 4.) jump to top of loop
    j main_while




#============================================================
# Interrupt Handler
#============================================================

.kdata
    chunkIH:            .space 8
    non_intrpt_str:     .asciiz "Non-interrupt exception\n"
    unhandled_str:      .asciiz "Unhandled interrupt type\n"

.ktext 0x80000080

interrupt_handler:

    .set noat

    move $k1, $at

    .set at

    la   $k0, chunkIH
    sw   $a0, 0($k0)
    sw   $a1, 4($k0)

    mfc0 $k0, $13
    srl  $a0, $k0, 2
    and  $a0, $a0, 0xf
    bne  $a0, 0  , non_intrpt

interrupt_dispatch:

    mfc0 $k0, $13
    beq  $k0, $zero, $done

    and  $a0, $k0  , 0x2000
    bne  $a0, 0    , kick_interrupt

    and  $a0, $k0  , 0x4000
    bne  $a0, 0    , puzzle_interrupt

    li   $v0, 4
    la   $a0, unhandled_str
    syscall

    j    interrupt_done

kick_interrupt:

    j    interrupt_dispatch

puzzle_interrupt:

    j    interrupt_dispatch

non_intrpt:

    li   $v0, 4
    la   $a0, non_intrpt_str
    syscall
    j    interrupt_done

interrupt_done:

    la   $k0, chunkIH
    lw   $a0, 0($k0)
    lw   $a1, 4($k0)
    mfc0 $k0, $14

    .set noat

    move $at  $k1

    .set at

    rfe
    jr   $k0
    nop
