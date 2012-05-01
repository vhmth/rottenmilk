##############################################################
# Main SPIMbot Code
#
# Taylor Fairbank | Vinay Hiremath
##############################################################



#============================================================
# Main Functionality
#============================================================

.data
    ball_to_kick:   .word -1
    goal_x:         .word 0                      # default to the left goal

.text
main:

    # set our interrupts
    li   $t4, 0x2001                             # enable suduko and kick
    mtc0 $t4, $12

    # set up our stack variables
    sub  $sp, $sp, 4
    sw   $ra, 0($sp)

    # figure out which goal we need to score on
    lw   $t0, 0xffff0020($0)                     # SPIMBOT_X
    li   $t3, 150
    bgt  $t0, $t3, main_goalsetup_end

    # we need to shoot for the right goal- default is to shoot to left (goal_x = 0)
    li   $t0, 300
    sw   $t0, goal_x

main_goalsetup_end:

main_while:

main_kickball:
    # 1.) check if ball_to_kick is greater than equal to zero
    #
    #     if (yes):
    #       check if ball exists
    #       select ball
    #       check if ball is kickable
    #       if (yes):
    #           kick ball
    lw   $t2, ball_to_kick
    bltz $t2, main_kickball_end                  # if there are no balls to kick

    # make spimbot's velocity 0
    li   $t0, 0
    sw   $t0, 0xffff0010($0)
###########################################
#    move $a0, $t2
#    li   $v0, 1
#    syscall
#infiniteC:
#    j infiniteC
###########################################

    sw   $t2, 0xffff00d0($0)                     # BALL_SELECT
    lw   $t3, 0xffff00e0($0)                     # check if BALL_EXISTS
    beq  $t3, 0, main_kickball_end

    lw   $t3, 0xffff00e4($0)                     # check if BALL_IS_KICKABLE
    beq  $t3, 0, main_kickball_end

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
    lw   $t1, 0xffff00b8($0)                     # get our current ENERGY
    li   $t0, 4
    div  $t0, $t1, $t0
############################################
#    move $a0, $t0
#    li   $v0, 1
#    syscall
#infiniteD:
#    j infiniteD
############################################
    sw   $t0, 0xffff00c8($0)                     # kick with 25% energy, KICK_ENERGY
    li   $t0, -1
    sw   $t0, ball_to_kick
main_kickball_end:

main_runto:
    # 2.) check closest ball that exists on field
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
    lw   $t9, ball_to_kick
    bge  $t9, 0, main_kickball                   # check if there's a ball to kick

    beq  $t1, 4, main_runto_move                 # found closest ball to run to

    sw   $t1, 0xffff00d0($0)                     # BALL_SELECT
    lw   $t7, 0xffff00e0($0)                     # see if BALL_EXISTS on field
    beq  $t7, 0, main_runto_whileB

    lw   $t4, 0xffff00d4($0)                     # t4 has BALL_X
    lw   $t5, 0xffff00d8($0)                     # t5 has BALL_Y

    sub  $t4, $t2, $t4                           # delta x
    sub  $t5, $t3, $t5                           # delta y
    li   $t8, 0                                  # temporary distance
    mul  $t7, $t4, $t4
    add  $t8, $t8, $t7
    mul  $t7, $t5, $t5
    add  $t8, $t8, $t7                           # t8 has temporary distance

    bgt  $t8, $t6, main_runto_whileB             # check if t8 has lowest distance
    move $t6, $t8                                # update closest distance
    move $t0, $t1                                # update closest ball

main_runto_whileB:
    add  $t1, $t1, 1                             # increment ball looper
    j    main_runto_while

main_runto_move:
    lw   $t9, ball_to_kick
    bge  $t9, 0, main_kickball                   # check if there's a ball to kick

    bne  $t6, 180000, main_runto_moveB           # any balls on the field?
    li   $t0, 0                                  # set velocity to zero
    sw   $t0, 0xffff0010($0)
    j    main_runto                              # look for other balls
main_runto_moveB:
    sw   $t0, 0xffff00d0($0)                     # SELECT_BALL
    lw   $t1, 0xffff00e0($0)                     # BALL_EXISTS
    beq  $t1, 0, main_runto_end                  # check if this ball exists

                                                 # TODO: check if the ball is moving
    lw   $t4, 0xffff00d4($0)                     # t4 has BALL_X
    lw   $t5, 0xffff00d8($0)                     # t5 has BALL_Y
    sub  $t4, $t2, $t4                           # t4 has delta x
    sub  $t5, $t3, $t5                           # t5 has delta y
    move $a0, $t4
    move $a1, $t5

    jal arctan                                   # call arctan
    move $t4, $v0                                # angle from arctan
    sub  $t4, $t4, 180

    sw   $t4, 0xffff0014($0)                     # set ORIENTATION_VALUE to arctan
    li   $t4, 1
    sw   $t4, 0xffff0018($0)                     # set ORIENTATION_CONTROL to absolute
    li   $t4, 1
    sw   $t4, 0xffff0010($0)                     # run to ball via VELOCITY_VALUE
    j main_while_end

main_runto_end:
    li   $t0, 0
    sw   $t0, 0xffff0010($0)
    j    main_runto

main_while_end:
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
    beq  $k0, $zero, interrupt_done

    and  $a0, $k0, 0x2000
    bne  $a0, 0, kick_interrupt

    li   $v0, 4
    la   $a0, unhandled_str
    syscall

    j    interrupt_done

kick_interrupt:
    # figure out the ball that we've bumped into
    li   $a0, -1

kick_interrupt_loop:
    add  $a0, $a0, 1                             # increment ball looper
    sw   $a0, 0xffff00d0($0)                     # select ball
    lw   $a1, 0xffff00e0($0)                     # check if ball exists
    beq  $a1, 0, kick_interrupt_loop

    lw   $a1, 0xffff00e4($0)                     # check if ball is kickable
    beq  $a1, 0, kick_interrupt_loop

kick_interrupt_loop_done:
    sw   $a0, ball_to_kick                       # a0 has ball to kick
#######################################
#    li   $v0, 1
#    syscall
#infiniteA:
#    j infiniteA
#######################################
    sw   $a0, 0xffff0064($0)                     # acknowledge interrupt: can write any value
    j    interrupt_dispatch

non_intrpt:
    li   $v0, 4
    la   $a0, non_intrpt_str
    syscall
######################################
infiniteB:
    j infiniteB
######################################
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

#### Code to compute angle ####

.data
three:  .float 3.0
five:   .float 5.0
PI:     .float 3.14159
F180:   .float 180.0

.text

# use a Taylor series approximation to compute arctan(x,y)

arctan:                      # a0 = delta_x, a1 = delta_y
                             # t0 = abs(a0), t1 = abs(a1)
        abs   $t0, $a0       # t0 = |delta_x|
        abs   $t1, $a1       # t1 = |delta_y|
        li    $v0, 0         # v0 = angle
        ble   $t1, $t0, no_TURN_90

                             # if (abs(y) > abs(x)) { rotate 90 degrees }
        move  $v0, $a1       # temp = y
        sub   $a1, $0, $a0   # y = -x
        move  $a0, $v0       # x = temp
        li    $v0, 90        # angle = 90.0

no_TURN_90:
        bge   $a0, $0, pos_x       # skip if (x >= 0.0)
        addi  $v0, $v0, 180

pos_x:  mtc1  $a0, $f0
        mtc1  $a1, $f1
        cvt.s.w $f0, $f0           # convert from ints to floats
        cvt.s.w $f1, $f1

        div.s   $f0, $f1, $f0      # float v = (float) y / (float) x;
        mul.s   $f1, $f0, $f0      # v^^2
        mul.s   $f2, $f1, $f0      # v^^3
        l.s     $f3, three($0)     # load 3.0
        div.s   $f3, $f2, $f3      # v^^3/3
        sub.s   $f6, $f0, $f3      # v - v^^3/3
        mul.s   $f4, $f1, $f2      # v^^5
        l.s     $f5, five($0)      # load 5.0
        div.s   $f5, $f4, $f5      # v^^5/5
        add.s   $f6, $f6, $f5      # value = v - v^^3/3 + v^^5/5

        l.s     $f8, PI($0)        # load PI
        div.s   $f6, $f6, $f8      # value / PI
        l.s     $f7, F180($0)      # load 180.0
        mul.s   $f6, $f6, $f7      # 180.0 * value / PI

        cvt.w.s $f6, $f6           # convert "delta" back to integer
        mfc1    $a0, $f6
        add     $v0, $v0, $a0      # angle += delta

        jr    $ra
