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
    goal_x:         .word 0                      # default to the left goal

    boards:         .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0
                    .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0
                    .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0
                    .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0
                    .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0
                    .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0
                    .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0
                    .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0
                    .word 0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0    0 0 0 0 0 0 0 0 0

    boards_num:     .word 0
    boards_cur:     .word 0
    boards_max:     .word 3

.text
main:

    # set our interrupts
    li   $t4, 0x6001                             # enable suduko and kick
    mtc0 $t4, $12

    # set up our stack variables
    sub  $sp, $sp, 4
    sw   $s0, 0($sp)

    # figure out which goal we need to score on
    lw   $t0, 0xffff0020($0)                     # SPIMBOT_X
    li   $t3, 150
    bgt  $t0, $t3, main_while

    # we shoot for the right goal
    li   $t0, 300
    sw   $t0, goal_x

main_while:

    # 1.) check if num_puzzles is zero
    #
    #     if (yes):
    #       request X suduko puzzles
    lw   $t0, num_puzzles
    bgtz $t0, main_kickball
    # TODO: request X suduko puzzles

main_kickball:

    # 2.) check if ball_to_kick is greater than equal to zero
    #
    #     if (yes):
    #       check if ball exists
    #       select ball
    #       check if ball is kickable
    #       if (yes):
    #           kick ball
    lw   $t2, ball_to_kick
    bltz $t2, main_runto                         # if there are no balls to kick

    sw   $t2, 0xffff00d0($0)                     # BALL_SELECT
    lw   $t3, 0xffff00e0($0)                     # check if BALL_EXISTS
    beq  $t3, 0, main_runto

    lw   $t3, 0xffff00e4($0)                     # check if BALL_IS_KICKABLE
    beq  $t3, 0, main_runto

    lw   $t0, 0xffff00d4($0)                     # get angle of ball to goal, BALL_X
    lw   $t1, 0xffff00d8($0)                     # BALL_Y
    lw   $t2, goal_x
    li   $t3, 150                                # TODO: variable aim for goal's Y pos
    sub  $t0, $t0, $t2                           # delta x
    sub  $t1, $t1, $t3                           # delta y
    move $a0, $t0
    move $a1, $t1

    jal  arctan
    move $t0, $v0                                # angle from arctan
    sub  $t0, $t0, 180
    sw   $t0, 0xffff00c4($0)                     # set KICK_ORIENTATION
    li   $t0, 1
    li   $t1, 4
    div.d $t0, $t0, $t1
    lw   $t1, 0xffff00b8($0)                     # get our current ENERGY
    mult $t0, $t0, $t1                           # 25% of energy
    sw   $t0, 0xffff00c8($0)                     # kick with 25% energy, KICK_ENERGY
                                                 # TODO: check how much energy and kick with at most X percent

main_runto:

    # 3.) check closest ball that exists on field
    #
    #     calculate arc tan to ball
    #     run towards ball
    li   $t0, 0                                  # t0 has closest ball
    li   $t1, 0                                  # t1 is the ball looper
    lw   $t2, 0xffff0020($0)                     # t2 has SPIMBOT_X_LOCATION
    lw   $t3, 0xffff0024($0)                     # t3 has SPIMBOT_Y_LOCATION
                                                 # t6 has closest distance
    li   $t6, 180000                             # initialize t6 to ridiculous distance

main_runto_while:
    beq  $t1, 4, main_runto_end                  # found closest ball to run to

    sw   $t1, 0xffff00d0($0)                     # BALL_SELECT
    lw   $t7, 0xffff00e0($0)                     # see if BALL_EXISTS on field
    beq  $t7, 0, main_runto_whileB

    lw   $t4, 0xffff00d4($0)                     # t4 has BALL_X
    lw   $t5, 0xffff00d8($0)                     # t5 has BALL_Y

    sub  $t4, $t2, $t4                           # delta x
    sub  $t5, $t3, $t5                           # delta y
    li   $t8, 0                                  # temporary distance
    mult $t7, $t4, $t4
    add  $t8, $t8, $t7
    mult $t7, $t5, $t5
    add  $t8, $t8, $t7                           # t8 has temporary distance

    bgt  $t8, $t6, main_runto_whileB             # check if t8 has lowest distance
    move $t6, $t8                                # update closest distance
    move $t0, $t1                                # update closest ball

main_runto_whileB:
    add  $t1, $t1, 1                             # increment ball looper
    j    main_runto_while

main_runto_end:
    sw   $t0, 0xffff00d0($0)                     # SELECT_BALL
    lw   $t1, 0xffff00e0($0)                     # BALL_EXISTS
    beq  $t1, 0, main_while_end                  # check if this ball exists

                                                 #TODO: check if the ball is moving
    lw   $t4, 0xffff00d4($0)                     # t4 has BALL_X
    lw   $t5, 0xffff00d8($0)                     # t5 has BALL_Y
    sub  $t4, $t2, $t4                           # t4 has delta x
    sub  $t5, $t3, $t4                           # t5 has delta y
    move $a0, $t4
    move $a1, $t5

    jal arctan                                   # call arctan
    move $t4, $v0                                # angle from arctan
    sub  $t4, $t4, 180

    sw   $t4, 0xffff0014($0)                     # set ORIENTATION_VALUE to arctan angle
    li   $t4, 1
    sw   $t4, 0xffff0018($0)                     # set ORIENTATION_CONTROL to absolute
    li   $t4, 10
    sw   $t4, 0xffff0010($0)                     # run to ball via VELOCITY_VALUE

main_while_end:

    # 4.) jump to top of loop
    j main_while

main_return:
    jr   $ra



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
