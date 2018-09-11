/**
@file main.h
@brief Header file containing functions prototypes, defines and global variables and structures for SPACEGAME.
@brief Revision 1.0.
@author Geoff Grevers
@date   May 2016
*/


#ifndef MAIN_H
#define MAIN_H

#define MAX_ENEMIES 20
#define SHIP_OFFSET 3
#define START_STATE 0
#define ASTRO1_STATE 1
#define ALIEN1_STATE 2
#define BOSS1_STATE 3
#define DEATH_STATE 4
#define CLEAR 0
#define SET 1


/**
@namespace pot_x
@brief Input for joystick x-axis.
@namespace pot_y
@brief Input for joystick y-axis.
@namespace pot
@brief Analogue input for the potentiometer.
@namespace switch_external
@brief GPIO input for the button on the PCB.
@namespace led
@brief GPIO output for the LEDs. A red and green LED are connected to the same output, a 1 displays green and a 0 displays red.
@namespace lcd
@brief output for the Nokia display, with the pins being: VCC, SCE, RST, D/C, MOSI, SCLK, LED, in respective order.
*/
AnalogIn pot_x(PTB2);
AnalogIn pot_y(PTB3);
AnalogIn pot(PTB10);
InterruptIn switch_external(PTB18);
DigitalOut led(PTC2);   
N5110 lcd (PTE26 , PTA0 , PTC4 , PTD0 , PTD2 , PTD1 , PTC3);

Ticker ticker_ship;          /*!< Ticker used for ship movement */
Ticker ticker_bullet;        /*!< Ticker used for bullet speed */
Ticker ticker_enemy_bullet;  /*!< Ticker used for enemy bullet speed */
Ticker ticker_fsm;           /*!< Ticker used for timings in FSM */

/**
Structure for creating an image.
@brief To create an image there must an x-y coordinate pair for each pixel, from a chosen point.
@param x - x-coordinate from reference
@param y - y-coordinate from reference
*/
struct imagestruct {
    int x;
    int y;
};
typedef imagestruct image;

int total_objects; /*!< Number of enemies */

volatile int g_switch_external_flag = 0;    /*!< External switch flag set in ISR */
volatile int g_timer_flag_ship = 0;         /*!< Timer flag for ship speed set in ISR */
volatile int g_timer_flag_bullet = 0;       /*!< Timer flag for ship bullet speed set in ISR */
volatile int g_timer_flag_enemy_bullet = 0; /*!< Timer flag for enemy bullet speed set in ISR */
volatile int g_timer_flag_fsm = 0;          /*!< Timer flag for the FSM set in ISR */
int length_score;       /*!< Buffer size for score */
int length_lives;       /*!< Buffer size for lives */
int g_score = 0;        /*!< Score for the game, increases when enemies are killed by the player */
int g_alive = 0;        /*!< Alive or dead state of the spaceship, 1 or 0 */
int g_number_lives = 0; /*!< Number of lives the ship has */
int g_no_of_obj = 0;    /*!< Number of enemies */
int g_new_state = 0;    /*!< Decides whether the next state should be accessed, 1 or 0 */
int ship_x;             /*!< The x-coordinate of the ship */
int ship_y;             /*!< The y-coordinate of the ship */
int x;                  /*!< Used for x-coordinates */
int y;                  /*!< Used for y-coordinates */
int asteroid_x;         /*!< The x-coordinates of the asteroids */
int firsttime;          /*!< Used during initialisation of certain routines, 1 or 0 */
char buffer_score[14];  /*!< Score buffer used to display the score in a printable string */
char buffer_lives[7];   /*!< Lives buffer used to display lives in a printable string */

/**
Finite State Machine used for moving to next level or back to start when dead etc.
@param max_y_offset - The size of enemies, need to know for collisions. Amount of pixels that are part of the enemy, above it.
@param min_y_offset - The size of enemies, need to know for collisions. Amount of pixels that are part of the enemy, below it.
@param time - Speed of movement of enemies in each state
@param total_objects - Amount of enemies that wave
@param score_value - The amount of points an enemy adds to your score
@param shoot_ability - If the enemy can shoot or not
@param shoot_offset - Where the enemy shoud should from. Default is the centre point of the enemy
@param function - Defines the behaviour of the enemies
@param space_object - What the displayed enemy(s) will look like
@param nextState[] - Defines which state will happen next
*/
struct FSM {
    int max_y_offset;
    int min_y_offset;
    float time;
    int total_objects;
    int score_value;
    int shoot_ability;
    image *shoot_offset;
    void (*function)();
    image *space_object;
    int nextState[3];     // array of next states
};
typedef FSM stateType;

int g_state;    /*!< The current state */

/**
Structure for enemy data
@param x - integer x-coordinate value
@param y - integer y-coordinate value
@param max_y_offset - The vertical upward pixel range of the enemy
@param min_y_offset - The vertical downward pixel range of the enemy
@param length - Where the enemy is fully off the screen
@param iteration - Used for counting
@param live - Whether the enemy is alive or not
@param bullet_x - The enemy bullet x-coordinate
@param bullet_y - The enemy bullet y-coordinate
@param bullet_live - Whether an enemy bullet is active or not
@param bullet_length - The length of an enemy bullet
@param clear_object - Used for clearing or not clearing an enemy
*/
struct objects {
    int x;
    int y;
    int max_y_offset;
    int min_y_offset;
    int length;
    int iteration;
    int live;
    int bullet_x;
    int bullet_y;
    int bullet_live;
    int bullet_length;
    int clear_object;
};
typedef objects obj;
obj enemy_array[MAX_ENEMIES]; /*!< The maximum amount of enemies */

/**
@namespace switch_external_isr
@brief sets flag for switch on PCB in iterrupt service routine
@namespace start
@brief initialise values for the game in the start state
@namespace endscreen
@brief displays the screen when the spaceship is destroyed
@namespace timer_isr_ship
@brief used for timing of the ship's movement
@namespace timer_isr_bullet
@brief timing for the speed of a bullet
@namespace timer_isr_fsm
@brief used for the timings of each state
@namespace shipcontrol
@brief used to control the movement of the ship.
@namespace shoot
@brief for firing a bullet
@namespace enemy_shoot
@brief used for enemy ships to fire bullets
@namespace movement
@brief this is for the movement of the enemies
@namespace boss_movement
@brief the movement behaviour of the boss
*/

void switch_external_isr();
void start();
void endscreen();
void timer_isr_ship();
void timer_isr_bullet();
void timer_isr_enemy_bullet();
void timer_isr_fsm();
void shipcontrol();
void shoot();
void enemy_shoot();
void movement();
void boss_movement();

/**
Displays an image on the display
@param xcoord - x-coordinate of image (integer)
@param ycoord - y-coordinate of image (integer)
@param image - calls an image 
@param flag - display or clear the image
@returns an array of pixels displayed on the lcd
*/
void paint_character(int, int, image *, int);

image spaceship[] = {0,0,-3,-3,-2,-2,-1,-2,-2,-1,-1,-1,-1,0,-1,1,-1,2,-2,1,
                        -2,2,-1,2,-3,3,0,-1,0,1,1,0,1,-1,1,1,2,0,3,0,99};         /*!< The image of the spaceship */
image asteroid[] = {0,0,-1,-1,0,-1,0,-2,1,-1,1,0,2,0,-1,1,0,1,0,1,1,1,1,2,99};    /*!< The image of the asteroids */
image enemy_spaceship[] = {0,0,0,1,0,-1,1,-2,1,2,-1,0,99};                        /*!< The image of the enemy spaceships */
image boss1[] = {2,-4, 3,-4, 4,-4,
                 -1,-3, 0,-3, 1,-3, 2,-3, 3,-3,
                 -3,-2, -2,-2, -1,-2, 0,-2, 1,-2, 2,-2, 3,-2, 4,-2, 5,-2,
                 -5,-1, -4,-1, -3,-1, -2,-1, -1,-1, 0,-1, 1,-1, 2,-1, 3,-1,
                 -6,0, -5,0, -4,0, -3,0, -2,0, -1,0, 0,0, 1,0, 2,0, 3,0, 4,0,
                 -5,1, -4,1, -3,1, -2,1, -1,1, 0,1, 1,1, 2,1, 3,1,
                 -3,2, -2,2, -1,2, 0,2, 1,2, 2,2, 3,2, 4,2, 5,2,
                 -1,3, 0,3, 1,3, 2,3, 3,3,
                 2,4, 3,4, 4,4, 99};                                              /*!< The image of the boss */
image boss_guns[] = {2,-4, -1,-3, -4,-2, -5,-1, -6,0, -5,1, -4,2, -1,3, 2,4, 99}; /*!< Used to make bullets fire from the correct positions on the boss */

/**
FSM
@brief Sets the changes for each state.
*/
stateType state[5] = {
    {0, 0, 3.0,  0,  0, 0, 0, start, 0, {START_STATE, ASTRO1_STATE, DEATH_STATE}},
    {2, 2, 0.2, 10,  5, 0, 0, movement, asteroid, {START_STATE, ALIEN1_STATE, DEATH_STATE}},
    {2, 2, 0.2, 15, 10, 1, 0, movement, enemy_spaceship, {START_STATE, BOSS1_STATE, DEATH_STATE}},
    {4, 4, 0.3, 9, 100, 1, boss_guns, boss_movement, boss1, {START_STATE, ASTRO1_STATE, DEATH_STATE}},
    {0, 0, 0.0, 0,   0, 0, 0, 0, 0, {0, 0, 0}}
}; 

#endif