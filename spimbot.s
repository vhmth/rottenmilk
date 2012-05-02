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

    boards:         .space 1000                  # the boards' storage space: 324 bytes per board; 324 = 4 * 9 * 9 = word size * # sudoku spaces on 1 board = memory for 1 board
    boards_cur:     .word 0                      # the current board to solve (rotates between 0 and boards_max)
    boards_pending: .word 0                      # the number of boards being generated
    boards_max:     .word 2                      # the max number of boards

    newline:        .asciiz "\n"        # useful for printing commands
    star:           .asciiz "*"
    kick:           .asciiz "kick"
    runto:          .asciiz "run"
    sudoku:         .asciiz "sudoku"

.text
main:

    # set our interrupts
    li   $t4, 0x6001                             # enable suduko and kick
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

    # request initial boards
    #  - note: only request more when one's been solved (completion based rather than checking every loop)
    la   $t0, boards
    li   $t1, 0
    lw   $t2, boards_max
    sw   $t2, boards_pending
main_init_boards:
    sw   $t0, 0xffff00e8($0)                     # SUDOKU_GET- writes 324 bytes to the address passed in
    addi $t0, $t0, 324
    addi $t1, $t1, 1
    blt  $t1, $t2, main_init_boards

main_while:

################### START KICK ####################
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

###################
    la   $a0, kick
    li   $v0, 4
    #syscall
    la   $a0, newline
    li   $v0, 4
    #syscall
###################

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
    li   $t3, 150
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
    sw   $t0, 0xffff00c8($0)                     # kick with 25% energy, KICK_ENERGY

main_kickball_end:                               # should have already kicked the ball
    li   $t0, -1
    sw   $t0, ball_to_kick
################### END KICK ####################

################### START RUNTO ####################
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
    bge  $t9, 0, main_runto_end                  # check if there's a ball to kick

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
    bge  $t9, 0, main_runto_end                  # check if there's a ball to kick

###################
    la   $a0, runto
    li   $v0, 4
    #syscall
    la   $a0, newline
    li   $v0, 4
    #syscall
###################

    bne  $t6, 180000, main_runto_moveB           # any balls on the field?
    li   $t0, 0                                  # set velocity to zero
    sw   $t0, 0xffff0010($0)
    j    main_runto_end                          # done for now

main_runto_moveB:
    sw   $t0, 0xffff00d0($0)                     # SELECT_BALL
    lw   $t1, 0xffff00e0($0)                     # BALL_EXISTS
    beq  $t1, 0, main_runto_stop                  # check if this ball exists

#########################
#    li   $v0, 1
#    move $a0, $t0
#    syscall
#########################
                                                 # TODO: check if the ball is moving
    lw   $t4, 0xffff00d4($0)                     # t4 has BALL_X
    lw   $t5, 0xffff00d8($0)                     # t5 has BALL_Y
    sub  $t4, $t2, $t4                           # t4 has delta x
    sub  $t5, $t3, $t5                           # t5 has delta y
    move $a0, $t4
    move $a1, $t5

    jal  arctan                                  # call arctan
    move $t4, $v0                                # angle from arctan
    sub  $t4, $t4, 180

    sw   $t4, 0xffff0014($0)                     # set ORIENTATION_VALUE to arctan
    li   $t4, 1
    sw   $t4, 0xffff0018($0)                     # set ORIENTATION_CONTROL to absolute
    li   $t4, 10

    lw   $t9, ball_to_kick
    bge  $t9, 0, main_runto_end                  # check if there's a ball to kick

    sw   $t4, 0xffff0010($0)                     # run to ball via VELOCITY_VALUE
    j main_runto_end

main_runto_stop:
    li   $t0, 0
    sw   $t0, 0xffff0010($0)

main_runto_end:
################### END RUNTO ####################

################### START SUDOKU ####################
main_sudoku:
    # 3.) solve the sudoku puzzel
    #
    # if (boards_pending != boards_max) {        // we have boards to solve
    #     Pick 1:
    #         rule 1 until no changes happen
    #         rule 2 until no changes happen
    #         brute force
    # }

    lw   $t0, boards_max
    lw   $t1, boards_pending
    beq  $t0, $t1, main_sudoku_end               # no valid boards to solve, all pending

###################
    la   $a0, sudoku
    li   $v0, 4
    #syscall
    la   $a0, newline
    li   $v0, 4
    #syscall
###################

main_sudoku_solve:
    # TODO: handle rule2 and brute force
    lw   $t0, boards_cur
    mul  $t0, $t0, 324
    la   $s0, boards
    add  $s0, $s0, $t0                           # choose the current board
    move $a0, $s0

    # test: print board
    #jal print_board
    #move $a0, $s0
    move $a0, $s0
    li   $v0, 1
    syscall
    la   $a0, newline
    li   $v0, 4
    syscall
    move $a0, $s0

    jal  rule1                              # limit on jumps, jump to a halfway point first

    # test: print rule1 result
    move $s4, $v0
    move $a0, $v0
    li   $v0, 1
    syscall
    la   $a0, newline
    li   $v0, 4
    syscall
    move $v0, $s4

    bne  $v0, 0, main_sudoku_end                # if we made a successful pass with our rules, go back to main loop and wait for next cycle

    # TODO: bruteforce here

    sw   $s0, 0xffff00ec($0)                     # SUDOKU_DONE- solved board pointer
    lw   $t0, boards_cur
    addi $t0, $t0, 1
    lw   $t1, boards_max
    bne  $t0, $t1, main_sudoku_getnext
    li   $t0, 0                                  # wrap around

main_sudoku_getnext:                             # solved a board, request another
###################
    la   $a0, sudoku
    li   $v0, 4
    syscall
    la   $a0, newline
    li   $v0, 4
    syscall
###################

    sw   $t0, boards_cur
    sw   $s0, 0xffff00e8($0)                     # SUDOKU_GET- writes 324 bytes to the address passed in
    lw   $t0, boards_pending                     # increment boards_pending
    addi $t0, $t0, 1
    sw   $t0, boards_pending

main_sudoku_end:
################### END SUDOKU ####################

main_while_end:
    # 4.) jump to top of loop
    j main_while

main_return:
    # free stack space
    li   $t0, 324
    lw   $t3, boards_max
    mul  $t0, $t0, $t3
    add  $sp, $sp, $t0

    lw   $ra, 0($sp)
    addi $sp, $sp, 4

    jr   $ra

#################### SINGLETON ####################     
is_singleton:
    li    $v0, 0
    beq    $a0, 0, is_singleton_done        # return 0 if value == 0
    sub    $a1, $a0, 1
    and    $a1, $a0, $a1
    bne    $a1, 0, is_singleton_done        # return 0 if (value & (value - 1)) == 0
    li    $v0, 1
is_singleton_done:
    jr    $ra

#################### GET_SINGLETON ####################     
get_singleton:
    li    $v0, 0            # i
    li    $t1, 1
gs_loop:sll    $t2, $t1, $v0        # (1<<i)
    beq    $t2, $a0, get_singleton_done
    add    $v0, $v0, 1
    blt    $v0, 9, gs_loop        # repeat if (i < 9)
get_singleton_done:
    jr    $ra

#################### Sudoku Helpers ####################
get_square_begin:
    div $v0, $a0, 3
    mul $v0, $v0, 3
    jr  $ra

board_address:
    mul    $v0, $a1, 9        # i*9
    add    $v0, $v0, $a2        # (i*9)+j
    sll    $v0, $v0, 2        # ((i*9)+j)*4
    add    $v0, $a0, $v0
    jr    $ra

#################### RULE 1 ####################
rule1:
    sub    $sp, $sp, 32         
    sw    $ra, 0($sp)        # save $ra and free up 7 $s registers for
    sw    $s0, 4($sp)        # i
    sw    $s1, 8($sp)        # j
    sw    $s2, 12($sp)        # board
    sw    $s3, 16($sp)        # value
    sw    $s4, 20($sp)        # k
    sw    $s5, 24($sp)        # changed
    sw    $s6, 28($sp)        # temp
    move    $s2, $a0
    li    $s5, 0            # changed = false

    li    $s0, 0            # i = 0
r1_loop1:
    li    $s1, 0            # j = 0
r1_loop2:
    move    $a0, $s2        # board
    move     $a1, $s0        # i
    move    $a2, $s1        # j
    jal    board_address
    
    move $a0, $s2
    move $t0, $v0
    li   $v0, 1
    syscall
    la   $a0, newline
    li   $v0, 4
    syscall
    move $v0, $t0
    
    lw    $s3, 0($v0)        # value = board[i][j]
    move    $a0, $s3        
    jal    is_singleton
    beq    $v0, 0, r1_loop2_bot    # if not a singleton, we can go onto the next iteration

    li    $s4, 0            # k = 0
r1_loop3:
    beq    $s4, $s1, r1_skip_row    # skip if (k == j)
    move    $a0, $s2        # board
    move     $a1, $s0        # i
    move    $a2, $s4        # k
    jal    board_address
    
    move $a0, $s2
    move $t0, $v0
    li   $v0, 1
    syscall
    la   $a0, newline
    li   $v0, 4
    syscall
    move $v0, $t0
    
    lw    $t0, 0($v0)        # board[i][k]
    and    $t1, $t0, $s3        
    beq    $t1, 0, r1_skip_row
    not    $t1, $s3
    and    $t1, $t0, $t1        
    sw    $t1, 0($v0)        # board[i][k] = board[i][k] & ~value
    li    $s5, 1            # changed = true
    
r1_skip_row:
    beq    $s4, $s0, r1_skip_col    # skip if (k == i)
    move    $a0, $s2        # board
    move     $a1, $s4        # k
    move    $a2, $s1        # j
    jal    board_address
    
    move $a0, $s2
    move $t0, $v0
    li   $v0, 1
    syscall
    la   $a0, newline
    li   $v0, 4
    syscall
    move $v0, $t0
    
    lw    $t0, 0($v0)        # board[k][j]
    and    $t1, $t0, $s3        
    beq    $t1, 0, r1_skip_col
    not    $t1, $s3
    and    $t1, $t0, $t1        
    sw    $t1, 0($v0)        # board[k][j] = board[k][j] & ~value
    li    $s5, 1            # changed = true

r1_skip_col:    
    add    $s4, $s4, 1        # k++
    blt    $s4, 9, r1_loop3

## doubly nested loop
    move    $a0, $s0        # i
    jal    get_square_begin
    move    $s6, $v0        # ii
    move    $a0, $s1        # j
    jal    get_square_begin    # jj
    move     $t0, $s6        # k = ii
    add     $s6, $v0, 3        # jj + GRIDSIZE
    add    $t1, $t0, 3        # ii + GRIDSIZE

r1_loop4_outer:
    sub    $t2, $s6, 3        # l = jj

r1_loop4_inner:
    bne    $t0, $s0, r1_loop4_1
    beq    $t2, $s1, r1_loop4_bot

r1_loop4_1:    
    mul    $v0, $t0, 9        # k*9
    add    $v0, $v0, $t2        # (k*9)+l
    sll    $v0, $v0, 2        # ((k*9)+l)*4
    add    $v0, $s2, $v0        # &board[k][l]
    lw    $v1, 0($v0)        # board[k][l]
       and    $t3, $v1, $s3        # board[k][l] & value
    beq    $t3, 0, r1_loop4_bot

    not    $t3, $s3
    and    $v1, $v1, $t3        
    sw    $v1, 0($v0)        # board[k][l] = board[k][l] & ~value
    li    $s5, 1            # changed = true

r1_loop4_bot:    
    add    $t2, $t2, 1        # l++
    blt    $t2, $s6, r1_loop4_inner

    add    $t0, $t0, 1        # k++
    blt    $t0, $t1, r1_loop4_outer

r1_loop2_bot:    
    add    $s1, $s1, 1        # j++
    blt    $s1, 9, r1_loop2

    add    $s0, $s0, 1        # i++
    blt    $s0, 9, r1_loop1

    move    $v0, $s5        # return changed
    lw    $ra, 0($sp)        # restore registers and return
    lw    $s0, 4($sp)
    lw    $s1, 8($sp)
    lw    $s2, 12($sp)
    lw    $s3, 16($sp)
    lw    $s4, 20($sp)
    lw    $s5, 24($sp)
    lw    $s6, 28($sp)
    add    $sp, $sp, 32
    jr    $ra

#################### PRINT_NEWLINE ####################     
print_newline:
    lb       $a0, newline($0)            # read the newline char
    li       $v0, 11            # load the syscall option for printing chars
    syscall                  # print the char
    
    jr      $ra              # return to the calling procedure


#################### PRINT_INT_AND_SPACE ####################     
print_int_and_space:
    li       $v0, 1             # load the syscall option for printing ints
    syscall                  # print the element

    li       $a0, 32            # print a black space (ASCII 32)
    li       $v0, 11            # load the syscall option for printing chars
    syscall                  # print the char
    
    jr      $ra              # return to the calling procedure

#################### PRINT BOARD ####################     
print_board:
    sub    $sp, $sp, 20
    sw    $ra, 0($sp)        # save $ra and free up 4 $s registers for
    sw    $s0, 4($sp)        # i
    sw    $s1, 8($sp)        # j
    sw    $s2, 12($sp)        # the function argument
    sw    $s3, 16($sp)        # the computed pointer (which is used for 2 calls)
    move    $s2, $a0

    li    $s0, 0            # i
pb_loop1:
    li    $s1, 0            # j
pb_loop2:
    mul    $t0, $s0, 9        # i*9
    add    $t0, $t0, $s1        # (i*9)+j
    sll    $t0, $t0, 2        # ((i*9)+j)*4
    add    $s3, $s2, $t0
    lw    $a0, 0($s3)
    jal    is_singleton        
    beq    $v0, 0, pb_star        # if it was not a singleton, jump
    lw    $a0, 0($s3)
    jal    get_singleton        
    add    $a0, $v0, 1        # print the value + 1
    li    $v0, 1
    syscall
    j    pb_cont

pb_star:        
    li    $v0, 4            # print a "*"
    la    $a0, star
    syscall

pb_cont:    
    add    $s1, $s1, 1        # j++
    blt    $s1, 9, pb_loop2

    li    $v0, 4            # at the end of a line, print a newline char.
    la    $a0, newline
    syscall    
    
    add    $s0, $s0, 1        # i++
    blt    $s0, 9, pb_loop1

    lw    $ra, 0($sp)        # restore registers and return
    lw    $s0, 4($sp)
    lw    $s1, 8($sp)
    lw    $s2, 12($sp)
    lw    $s3, 16($sp)
    add    $sp, $sp, 20
    jr    $ra

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

    and  $a0, $k0, 0x4000
    bne  $a0, 0, puzzle_interrupt

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

puzzle_interrupt:
    # board is stored in passed in heap address
###################
    la   $a0, sudoku
    li   $v0, 4
    #syscall
    la   $a0, newline
    li   $v0, 4
    #syscall
###################

    lw   $a0, boards_pending                     # decrement boards_pending
    sub  $a0, $a0, 1
    sw   $a0, boards_pending

    sw   $a0, 0xffff0068($0)                     # acknowledge interrupt: can write any value
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
