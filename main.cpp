/**
@file main.cpp

SPACEGAME

Geoff Grevers

*/

#include "mbed.h"
#include "N5110.h"
#include "main.h"

DigitalOut buzzer(PTA2);

int main()
{
    led = 1;                                        // initialise led, remains green until on last life
    switch_external.fall(&switch_external_isr);     // initialising switch on PCB, calls on falling edge of input
    switch_external.mode(PullDown);                 // input pin mode parameter for PCB switch
    lcd.init();                                     // initialising LCD display
    lcd.clear();
    ticker_fsm.attach(&timer_isr_fsm, 0.2);
    g_state = START_STATE;      // define FSM state
    g_alive = 1;                // define alive state
    g_number_lives = 3;         // number of lives

    while(g_alive != 2)    {

        length_score = sprintf(buffer_score,"Sc:%3d",g_score);          // print formatted data to buffer
        if (length_score <= 14) {                                       // is string fits on display
            lcd.printString(buffer_score,0,0);                          // display on screen
        }
        length_lives = sprintf(buffer_lives,"Lives:%1d",g_number_lives);    // print formatted data to buffer
        if (length_lives <= 7) {                                            // is string fits on display (7 from 42 pixels / 6 pixels per char)
            lcd.printString(buffer_lives,42,0);                             // display on screen
        }
        lcd.drawLine(0, 8, WIDTH - 1, 8, 1);                           // display boundary

        if (g_timer_flag_ship == 1) {           // used for controlling the ship
            shipcontrol();
            g_timer_flag_ship = 0;              //reset flag
        }
        if (g_timer_flag_fsm == 1) {            // used for timing in states
            g_timer_flag_fsm = 0;               // reset flag
            (*state[g_state].function)();       // calls the function needed for that state
            ticker_fsm.attach (&timer_isr_fsm,state[g_state].time);       // timing during states
        }
        if (g_switch_external_flag || g_timer_flag_bullet) {              // controls movement of bullet across screen
            g_switch_external_flag = 0;         // reset flag
            g_timer_flag_bullet = 0;            // reset flag
            shoot();                            // calls the shoot routine
        }
        if (state[g_state].shoot_ability == 1 && g_timer_flag_enemy_bullet) {   // controls whether the enemy can shoot or not
            g_timer_flag_enemy_bullet = 0;                                      // by reseting the flag and
            enemy_shoot();                                                      // calling the enemy_shoot routine
        }
        if (g_number_lives < 1 && g_alive == 0) {       // is he out of lives and dead?
            g_alive = 2;                                // end while loop and display endscreen
        }
        if (g_new_state == 1) {                             // controls moving to the next state in the FSM
            g_state = state[g_state].nextState[g_alive];    // calls the next state in the FSM
            g_new_state  = 0;                               // resets the flag
            firsttime = 0;
        }
        if (g_number_lives > 0 && g_alive == 0) {       // dies but lives are remaining
            g_alive = 1;                                // reset flag
        }
        if (g_number_lives == 1) {      // turn on red LED when on last life
            led = 0;
        }
        sleep();        // saves power
    }
    endscreen();        // game over screen showing score
}


void start()    // setting up starting conditions
{
    int i;
    ship_x = SHIP_OFFSET + 1;           // initial ship position x axis
    ship_y = HEIGHT/2;                  // initial ship position y axis
    ticker_ship.detach();
    ticker_fsm.attach (&timer_isr_fsm, 5.0);
    lcd.clear();
    paint_character(ship_x, ship_y, spaceship, SET);
    g_new_state = 1;        // move to next state once initial conditions are set
    lcd.refresh();
    ticker_ship.attach(&timer_isr_ship, 0.1);
    g_no_of_obj = 0;        // no enemies currently
    for (i = 0; i < state[g_state].total_objects; i++) {
        enemy_array[i].iteration = 0;          // the iteration has to match or be greater than the other to make the enemy be displayed 
        enemy_array[i].x = 0;
        enemy_array[i].y = 0;                  // spits out enemies in a random y-axis positision in the gameplay portion of the screen
        enemy_array[i].max_y_offset = 0;
        enemy_array[i].min_y_offset = 0;
        enemy_array[i].live = 0;
        enemy_array[i].length = 0;
    }
}

void shipcontrol()      // function for controlling the ship using the joystick
{
    paint_character(ship_x, ship_y, spaceship, CLEAR);              // erase previous position of ship
    if (pot_y > (float)0.6 && ship_y < HEIGHT - SHIP_OFFSET - 1) {  // moving the ship down
        ship_y++;
    }
    if (pot_y < (float)0.4 && ship_y > 12) {                        // moving ship up
        ship_y--;
    }
    if (pot_x > (float)0.6 && ship_x < WIDTH - SHIP_OFFSET - 1) {   // moving ship right
        ship_x++;
    }
    if (pot_x < (float)0.4 && ship_x > SHIP_OFFSET) {               // moving ship left
        ship_x--;
    }
    paint_character(ship_x, ship_y, spaceship, SET);    // display the ship once new position is calculated
    ticker_ship.attach(&timer_isr_ship,pot);            // potentiometer controls the ships speed, as a form of difficulty  setting
    lcd.refresh();                                      // refresh display
}

void paint_character (int xcoord, int ycoord, image *Character, int flag)       // displays an image
{
    int i;
    for (i=0; i<80 && Character[i].x != 99; i++) {
        if (flag == 1) {                                                        // if a 1 is used, the image is displayed             
            lcd.setPixel (xcoord + Character[i].x, ycoord + Character[i].y);    
        } else {
            lcd.clearPixel (xcoord + Character[i].x, ycoord + Character[i].y);  // else it is cleared
        }
    }
}

void shoot()            // routine for shooting a bullet
{
    static int bullet_x = 0;
    static int bullet_y = 0;
    static int bullet_length = 0;
    int i;
    int j;
    if (bullet_length == 0) {                   // bullet to come out of the tip of the ship
        bullet_x = ship_x + SHIP_OFFSET;        // x-axis bullet starting location
        bullet_y = ship_y;                      // y-axis bullet starting location
    }
    if (bullet_x < WIDTH) {                     // if the bullet is on screen,
        lcd.setPixel (bullet_x,bullet_y);       // set the pixel
    }
    if (bullet_length == 3 || bullet_x == WIDTH) {                  // keeps the bullet to a length of 3 pixels
        lcd.clearPixel (bullet_x - bullet_length, bullet_y);        // clears pixels behind bullet
        if (bullet_x == WIDTH) {                                    // decrement the bullet if it is off the screen,
            bullet_length--;                                        // so it doesn't completely disappear at once
        }
    } else {
        bullet_length++;                        // if the bullet isn't at the edge of the lcd or of 3 length, increment the bullet length
    }
    if (bullet_x < WIDTH) {                     // if the bullet is on screen,
        bullet_x++;                             // increment on x-axis
        for (i = 0; i < state[g_state].total_objects; i++) {                                // used for clearing a bullet if it
            if (bullet_y >= (enemy_array[i].y - enemy_array[i].min_y_offset) &&             // manages to touch an enemy
                    bullet_y <= (enemy_array[i].y + enemy_array[i].max_y_offset) &&
                    (bullet_x == enemy_array[i].x || bullet_x == enemy_array[i].x+1)&&      // x+1 is used because if the two move simultaneously the bullet could skip over an enemy
                    enemy_array[i].live == 1) {
                enemy_array[i].live = 0;                    // clears the enemy
                g_no_of_obj--;                              // one less enemy
                g_score += state[g_state].score_value;      // adds appropiate number to score relevant to enemy type
                for (j = 0; j <= bullet_length; j++) {
                    lcd.clearPixel (bullet_x - (bullet_length-j), bullet_y);
                }
                bullet_length = 0;
                if (enemy_array[i].clear_object) {          // for boss do not clear
                    paint_character(enemy_array[i].x, enemy_array[i].y, state[g_state].space_object, CLEAR);
                }
            }
            if (bullet_y == (enemy_array[i].bullet_y) &&    // check for bullets head-on
                    (bullet_x == enemy_array[i].bullet_x || bullet_x == enemy_array[i].bullet_x+1) &&      //X+1 is used because if the two move simultaneously the bullet could skip over an enemy
                    enemy_array[i].bullet_live == 1) {
                enemy_array[i].bullet_live = 0;             // clears the bullet

                for (j = 0; j <= bullet_length; j++) {
                    lcd.clearPixel (bullet_x - (bullet_length-j), bullet_y);
                    lcd.clearPixel (enemy_array[i].bullet_x + (enemy_array[i].bullet_length - j), enemy_array[i].bullet_y);
                }
                bullet_length = 0;
                enemy_array[i].bullet_length = 0;
            }
        }
    }
    if (bullet_length == 0) {       //reseting the variables
        bullet_x = 0;
        bullet_y = 0;
        bullet_length = 0;
        ticker_bullet.detach();
        lcd.clearPixel(WIDTH, bullet_y);
    } else {
        ticker_bullet.attach (&timer_isr_bullet,0.02);    //bullet speed
    }
    lcd.refresh();
}

void enemy_shoot() // enemies that can shoot will use this
{
    int i;
    int j;

    for (i = 0; i < state[g_state].total_objects; i++) {       // loops round enemies in array
        if (enemy_array[i].bullet_live == 0 && (rand() % 400) == 1  && enemy_array[i].live == 1 && g_no_of_obj > 0) {  //to make sure bullets arent constantly firing the random number has to match
            enemy_array[i].bullet_live = 1;                    // flag to activate a shoot process
        }
        if (enemy_array[i].bullet_live == 1 ) {                // this object is shooting

            if (enemy_array[i].bullet_length == 0) {           // bullet has not yet been fired
                enemy_array[i].bullet_x = enemy_array[i].x;    // launch the bullet from gun object x
                enemy_array[i].bullet_y = enemy_array[i].y;    // and y coordinates
            }
            if (enemy_array[i].bullet_x > -1) {                                     // if the enemy bullet is on screen,
                lcd.setPixel (enemy_array[i].bullet_x, enemy_array[i].bullet_y);    // set the pixel
            }

            if (enemy_array[i].bullet_length == 3 || enemy_array[i].bullet_x == -1) {   // keeps the bullet to a length of 3 pixels
                lcd.clearPixel (enemy_array[i].bullet_x + enemy_array[i].bullet_length, enemy_array[i].bullet_y);        //clears pixels behind bullet
                if (enemy_array[i].bullet_x == -1) {                                    // decrement the bullet if it is off the screen,
                    enemy_array[i].bullet_length--;                                     // so it doesn't completely disappear at once
                }
            } else {
                enemy_array[i].bullet_length++;        // if the enemy bullet isn't at the edge of the lcd or of 3 length, increment the enemy bullet length
            }
            if (enemy_array[i].bullet_x > -1) {                     // if the enemy bullet is on screen,
                enemy_array[i].bullet_x--;                          // decrement on x-axis to move across screen
                if (enemy_array[i].bullet_y <= (ship_y + SHIP_OFFSET) &&                                // if a bullet manages to touch the ship
                        enemy_array[i].bullet_y >= (ship_y - SHIP_OFFSET) &&
                        (enemy_array[i].bullet_x == ship_x || enemy_array[i].bullet_x == ship_x-1)&&    //X-1 is used because if the two move simultaneously the bullet could skip over the ship
                        g_alive == 1) {
                    g_number_lives--;       // remove a life
                    g_alive = 0;            // kills the ship
                    g_new_state = 1;        // move to next state (beginning of game)
                    for (j = 0; j <= enemy_array[i].bullet_length; j++) {               // clearing pixel behind bullet
                        lcd.clearPixel (enemy_array[i].bullet_x + (enemy_array[i].bullet_length-j), enemy_array[i].bullet_y);
                    }
                    enemy_array[i].bullet_length = 0;                       // clear bullet if it hits the ship,
                    paint_character(ship_x, ship_y, spaceship, CLEAR);      // and clear the ship
                }
            }
            if (enemy_array[i].bullet_length == 0) {        // re-initialise
                enemy_array[i].bullet_x = 0;
                enemy_array[i].bullet_y = 0;
                enemy_array[i].bullet_length = 0;
                enemy_array[i].bullet_live = 0;
                ticker_enemy_bullet.detach();
                lcd.clearPixel(-1, enemy_array[i].bullet_y);
            }
        }
    }
    ticker_enemy_bullet.attach (&timer_isr_enemy_bullet,0.02);    // bullet speed
    lcd.refresh();                                                // update lcd from buffer
}

void movement()     // behaviour of enemies' movement
{

    static int iteration = 0;          // used to make ships not appear all at once
    int i;

    if(firsttime == 0) {               // initial conditions, runs first time loop runs 
        firsttime = 1;
        iteration = 0;
        g_no_of_obj = state[g_state].total_objects;
        for (i = 0; i < state[g_state].total_objects; i++) {
            enemy_array[i].iteration = (rand() % 6)*5;          // the iteration has to match or be greater than the other to make the enemy be displayed
            enemy_array[i].x = WIDTH -1;
            enemy_array[i].y = (rand() % 37) + 10;              // spits out enemies in a random y-axis positision in the gameplay portion of the screen
            enemy_array[i].max_y_offset = state[g_state].max_y_offset;
            enemy_array[i].min_y_offset = state[g_state].min_y_offset;
            enemy_array[i].live = 1;
            enemy_array[i].length = -2;                         // at -2 the enemy is fully off the screen and ready to be cleared
            enemy_array[i].clear_object = 1;
        }
        if (state[g_state].shoot_ability == 1) {                // attach timer for shooting enemies
            ticker_enemy_bullet.attach (&timer_isr_enemy_bullet,0.02);
        }
    }
    iteration++;    // loop counter
    for (i = 0; i < state[g_state].total_objects; i++) {
        if (iteration >= enemy_array[i].iteration && enemy_array[i].live == 1) {
            paint_character(enemy_array[i].x, enemy_array[i].y, state[g_state].space_object, CLEAR);     // clear opponent
            enemy_array[i].x--;                                                                          // move enemy along screen
            if (enemy_array[i].x < enemy_array[i].length) {                       // opponent off the screen
                enemy_array[i].live = 0;
                g_no_of_obj--;
            } else {
                paint_character (enemy_array[i].x, enemy_array[i].y, state[g_state].space_object, SET);
                if (((ship_y + SHIP_OFFSET >= (enemy_array[i].y - enemy_array[i].min_y_offset)) ||        // setting up which pixel dimensions that will cause a collision
                        (ship_y - SHIP_OFFSET >= (enemy_array[i].y - enemy_array[i].min_y_offset))) &&    // and kill the shapeship
                        ((ship_y - SHIP_OFFSET <= (enemy_array[i].y + enemy_array[i].max_y_offset)) ||
                         (ship_y + SHIP_OFFSET <= (enemy_array[i].y + enemy_array[i].max_y_offset))) &&
                        (ship_x == enemy_array[i].x)) {
                    paint_character(ship_x, ship_y, spaceship, CLEAR);            // enemy collision kills spaceship, blanks it out
                    g_number_lives--;                                             // remove a life
                    g_alive = 0;                                                  // ship dead
                    g_new_state = 1;                                              // go to next state
                } else {
                    paint_character (enemy_array[i].x, enemy_array[i].y, state[g_state].space_object, SET);     
                }
            }
        }
    }
    if (g_no_of_obj == 0 || g_alive == 0) {         // if there are no enemies or the ship dies
        g_new_state = 1;                            // next state
        firsttime = 0;                              // re-initialise
        iteration = 0;
        if (state[g_state].shoot_ability == 1) {    // detach ticker
            ticker_enemy_bullet.detach();
        }
    }
}

void boss_movement()    // behaviour of the boss
{

    static int l_direction = 0;         // initialise local variables
    static int l_boss;
    static int l_boss_alive;
    int i;

    l_boss = state[g_state].min_y_offset;

    if(firsttime == 0) {                        // initialisation
        firsttime = 1;
        for (i = 0; i < state[g_state].total_objects; i++) {    // 9 total objects for the boss, so he can shoot from 9 places and has 9 lives
            enemy_array[i].live = 1;                            // shooting a gun on the boss will remove 1 life and 1 gun from the boss
            enemy_array[i].max_y_offset = 0;                    // each row is being treated as a seperate entity
            enemy_array[i].min_y_offset = 0;
            enemy_array[i].clear_object = 0;

        }
        enemy_array[l_boss].x = WIDTH -1;              // initial x coordinate
        enemy_array[l_boss].y = HEIGHT/2;              // initial y coordinate
        g_no_of_obj = state[g_state].total_objects;    // initial lives of boss

        if (state[g_state].shoot_ability > 0) {
            ticker_enemy_bullet.attach (&timer_isr_enemy_bullet,0.02);  // initialise bullet speed
        }
    }

    for (i = 0; i < state[g_state].total_objects; i++) { // check if any of the boss's lives are left
        l_boss_alive = 0;
        if (enemy_array[i].live == 1) {
            l_boss_alive = 1;
        }
    }

    if (l_boss_alive == 1) {        // movement, collisions and where boss shoots from when it's alive
        paint_character(enemy_array[l_boss].x, enemy_array[l_boss].y, state[g_state].space_object, CLEAR);     // clear opponent

        if (l_direction == 0 && enemy_array[l_boss].x > 68) {   // movement of the boss and boundaries
            enemy_array[l_boss].x--;                            // left
            if (enemy_array[l_boss].x <= 68) {                  // dont move across screen like other enemies
                l_direction = 1;
            }
        }
        if (l_direction == 1) {
            enemy_array[l_boss].y++;                // down

            if (enemy_array[l_boss].y >= 33) {      // past 33 boss will be in death 'animation'
                l_direction = 2;                    
            }
        } else {
            enemy_array[l_boss].y--;                // up

            if (enemy_array[l_boss].y <= 13) {      // go back down when boss has reached the top of the screen
                l_direction = 1;
            }
        }

        if (((ship_y + SHIP_OFFSET >= (enemy_array[l_boss].y - state[g_state].max_y_offset)) ||          // setting up which pixel dimensions that will cause a collision
                (ship_y - SHIP_OFFSET >= (enemy_array[l_boss].y - state[g_state].max_y_offset))) &&      // and kill the shapeship
                ((ship_y - SHIP_OFFSET <= (enemy_array[l_boss].y + state[g_state].max_y_offset)) ||
                 (ship_y + SHIP_OFFSET <= (enemy_array[l_boss].y + state[g_state].max_y_offset))) &&
                (ship_x == enemy_array[l_boss].x)) {
            paint_character(ship_x, ship_y, spaceship, CLEAR);          // enemy collision kills spaceship, blanks it out
            g_number_lives--;                                           // remove a life
            g_alive = 0;                                                // ship dead
            g_new_state = 1;                                            // next state
        } else {
            paint_character (enemy_array[l_boss].x, enemy_array[l_boss].y, state[g_state].space_object, SET);   // if it's not dead, display
        }

        for (i = 1; i < state[g_state].total_objects; i++) {            // location of shooters on boss
            if (i != l_boss) {                                          
                enemy_array[i].x = enemy_array[l_boss].x + state[g_state].shoot_offset[i].x;
                enemy_array[i].y = enemy_array[l_boss].y + state[g_state].shoot_offset[i].y;
            }
        }
    }

    if (g_alive == 0) {   // if the ship dies
        g_new_state = 1;  // move to the start state
        firsttime = 0;    // re-initialise
        if (state[g_state].shoot_ability == 1) {    // detach ticker for enemy bullets
            ticker_enemy_bullet.detach();
        }

    }

    if (l_boss_alive <= 0) {   // if the boss dies

        paint_character(enemy_array[l_boss].x, enemy_array[l_boss].y, state[g_state].space_object, CLEAR);      // clear the boss
        if (enemy_array[l_boss].y == HEIGHT + 4) {                                                              // if the boss is off the screen,
            g_new_state = 1;                                                                                    // then move to the start state
            firsttime = 0;                                                                                      // re-intialise
            if (state[g_state].shoot_ability == 1) {    // detach ticker for enemy bullets
                ticker_enemy_bullet.detach();
            }
        } else {
            g_no_of_obj = 0;
            enemy_array[l_boss].y++;                    // lower it off the screen before clearing (death 'animation')
            paint_character(enemy_array[l_boss].x, enemy_array[l_boss].y, state[g_state].space_object, SET);   // repaint the boss as it moves down
        }
    }
}

void endscreen()
{
    lcd.clear();
    lcd.printString("GAME OVER",15,2);                          // display game over screen
    int length = sprintf(buffer_score,"Score: %3d",g_score);    // print formatted data to buffer
    if (length <= 11) {                                         // if string fits on display
        lcd.printString(buffer_score,15,3);                     // display on screen
    }
    lcd.refresh();                                              // update display
}

void timer_isr_ship()
{
    g_timer_flag_ship = 1;             // set flag in ISR
}

void switch_external_isr()
{
    g_switch_external_flag = 1;        // set flag in ISR
}

void timer_isr_bullet()
{
    g_timer_flag_bullet = 1;           // set flag in ISR
}

void timer_isr_enemy_bullet()
{
    g_timer_flag_enemy_bullet = 1;     // set flag in ISR
}

void timer_isr_fsm()
{
    g_timer_flag_fsm = 1;              // set flag in ISR
}