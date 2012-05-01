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
    goal_x          .word 0           # default to the left goal

main:

    # set our interrupts
    li   $t4, 0x6001                  # enable suduko and kick
    mtc0 $t4, $12

    # set up our stack variables
    sub  $sp, $sp, 4
    sw   $s0, 0($sp)

    # figure out which goal we need to score on
    sw   $t0, 0xffff0020($0)
    li   $t3, 150
    bgt  $t0, $t3, main_while

    # we shoot for the right goal
    li   $t0, 300
    lw   $t0, goal_x

main_while:
    # 1.) check if num_puzzles is zero
    #
    #     if (yes):
    #       request X suduko puzzles
    sw   $t0, num_puzzles
    bgtz $t0, kickball_main

    # request X suduko puzzles

kickball_main:
    # 2.) check if ball_to_kick is greater than equal to zero
    #
    #     if (yes):
    #       check if ball exists
    #       select ball
    #       check if ball is kickable
    #       if (yes):
    #           kick ball
    sw   $t2, ball_to_kick
    bltz $t2, runto_main              # if there are no balls to kick

    sw   $t2, 0xffff00d0($0)          # select ball
    lw   $t3, 0xffff00e0($0)          # check if ball exists
    beq  $t3, 0, runto_main

    lw   $t3, 0xffff00e4($0)          # check if ball is kickable
    beq  $t3, 0, runto_main

    lw   $t0, 0xffff00d4($0)          # get angle of ball to goal
    lw   $t1, 0xffff00d8($0)
    sw   $t2, goal_x
    li   $t3, 150
    sub  $t0, $t0, $t2                # delta x
    sub  $t1, $t1, $t3                # delta y
    move $a0, $t0
    move $a1, $t1

    move $s0, $ra
    jal  arctan
    move $ra, $s0
    move $t0, $v0                     # angle from arctan
    sub  $t0, $t0, 180
    sw   $t0, 0xffff00c4($0)          # set kick orientation
    li   $t0, 50
    sw   $t0, 0xffff00c8($0)          # kick with 50 energy

runto_main:
    # 3.) check closest ball that exists on field
    #
    #     calculate arc tan to ball
    #     run towards ball

end_while:
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
