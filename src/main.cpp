#include <Arduino.h>
#include "Segment.h"
#include "ShiftReg.h"
#include <LCD.h>
//#include <USART.h>
#include <string.h>
//shift reg variables
#define LATCH 3
#define DATA 2
#define CLOCK 4

//buzzer variable
#define BUZZER 5

//led variables
#define LEFTLED PC2
#define TOPLED PC3
#define RIGHTLED PC4
#define BOTTOMLED PC5

//button variable
#define BUTTONPIN PD3

//global game variables delcared here so they can be used throught the program
int timeleft = 9;
bool lostGame = true;
int answerKEY[200];
int level = 0;
int currentPos = 0;
bool pass = true;
//variable for keeping track of high score
int highScore = 0;

//7seg with shift reg variables
#define ARRAY_SIZE_DECIMAL 10
volatile byte oldPortValue = 0x00;
byte digits[ARRAY_SIZE_DECIMAL] = {0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6};
byte statechange = 0;

char text[200];

//the same ACD_Init and ADC_Read as from the lab
void ADC_Init()
{
  DDRC &= ~(1 << PC0); // 0x00;	        // Make ADC port as input
  DDRC &= ~(1 << PC1);
  ADCSRA |= (1 << ADEN) | (1 << ADATE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // 0xA7; / Enable ADC, with auto-trigger and freq/128
  ADMUX = 0x40;                                                                      // Vref: Avcc, ADC channel: 0
  DIDR0 = 0x03;                                                                      // disable digital pin C0 optional
}

int ADC_Read(byte channel)
{
  ADMUX = 0x40 | (channel & 0x07); // set input channel to read
  ADCSRA |= (1 << ADSC);           // Start ADC conversion
  while (!(ADCSRA & (1 << ADSC)))
    ; // Wait until end of conversion by polling ADC interrupt flag or when ADSC is 0 */
  // ADCSRA |= (1<<ADIF);               /* Clear interrupt flag */
  _delay_ms(1); /* Wait a little bit */
  return ADCW;  /* Return ADC word */
}

//function for displaying what the user has to input on the LEDS
void playList(int values[], int level)
{
  //loop over each value
  for (int i = 0; i < level; i++)
  {
    // light up the corisponding LED based on the values and sound the buzzer
    // after .25s turn them off
    switch (values[i])
    {
    case 1:
      PORTC |= (1 << RIGHTLED);
      PORTB |= (1 << BUZZER);
      _delay_ms(250);
      PORTC &= ~(1 << RIGHTLED);
      PORTB &= ~(1 << BUZZER);
      _delay_ms(250);
      break;

    case 2:
      PORTC |= (1 << LEFTLED);
      PORTB |= (1 << BUZZER);
      _delay_ms(250);
      PORTC &= ~(1 << LEFTLED);
      PORTB &= ~(1 << BUZZER);
      _delay_ms(250);
      break;

    case 3:
      PORTC |= (1 << TOPLED);
      PORTB |= (1 << BUZZER);
      _delay_ms(250);
      PORTC &= ~(1 << TOPLED);
      PORTB &= ~(1 << BUZZER);
      _delay_ms(250);
      break;

    case 4:
      PORTC |= (1 << BOTTOMLED);
      PORTB |= (1 << BUZZER);
      _delay_ms(250);
      PORTC &= ~(1 << BOTTOMLED);
      PORTB &= ~(1 << BUZZER);
      _delay_ms(250);
      break;

    default:
    //if we get a value that does not exist from 1-4 then do nothing
      break;
    }
  }
}

void endGame()
{
 //output final score to LCD
  LCD_command(1);
  LCD_command(0x80);
  LCD_string("Final Score: ");
  sprintf(text, "%d", level - 1);
  LCD_string(text);
  //Determine if the High Score is lower than the most recent score
  if (highScore < level - 1)
  {
    //Determine if the High Score is lower than the most recent score
    highScore = level - 1;
  }
  //output High Score to LCD
  LCD_command(0xC0);
  LCD_string("High Score: ");
  sprintf(text, "%d", highScore);
  LCD_string(text);
  //re-enable the button because the LCD gets it stuck on
  DDRD &= ~0b00001000;
  PORTD |= 0b000001000;
  //enable lost game flag
  lostGame = true;
}

int main()
{
#ifdef __DEBUG__
  dbg_start();
#endif
//initialize LCD and display the starting text
  LCD_init();
  LCD_command(1);
  LCD_string(" Click to begin");
  LCD_command(0xC0);
  LCD_string("High Score: 0");
  LCD_command(0x80);

//initialize the shift register
  init_shift(DATA, CLOCK, LATCH);

//set outputs for LEDs
  DDRC |= 0b111100;

//set output for buzzer
  DDRB |= 0b100000;

//set input and pull up resistor for joystick button
  DDRD &= ~0b00001000;
  PORTD |= 0b000001000;

// set the range values for the joystick
  int MAXVAL = 700;
  int MINVAL = 350;

//initialize the values for x and y for the joystick
  int valuex = 0;
  int valuey = 0;

//initialize analog to digital conversion
  ADC_Init();

//USART_init(); used for testing
//initlaize the variable use to hold the previous input direction
  int last = 0;

//set the random seed by reading one of the analog pins which will have a semi random number
  randomSeed(ADC_Read(PC1));

//initializing timer/prescaler/interupt
  TCCR1B |= (1 << CS12);  // prescalar 256
  TCNT1 = 0;          // set timer to 0 for 1s
  TIMSK1 |= (1 << TOIE1); // enable timer overflow interrupt

  while (1)
  {
    //check if the starting button has been pressed
    if (!(PIND & (1 << BUTTONPIN)))
    {
      //clear LCD and display the start of game text
      LCD_command(1);
      LCD_command(0xC0);
      LCD_string("High Score: ");
      sprintf(text, "%d", highScore);
      LCD_string(text);
      LCD_command(0x80);

      //reset game variables so the game can begin at the right level/position
      level = 0;
      currentPos = 0;
      pass = true;
      lostGame = false;

      //turn off all of the LEDs
      PORTC &= ~(1 << LEFTLED);
      PORTC &= ~(1 << TOPLED);
      PORTC &= ~(1 << BOTTOMLED);
      PORTC &= ~(1 << RIGHTLED);

      //reset the amount of time left
      timeleft = 9;
    }
    //this is the actual loop that runs the game, when the game is lost it is not entered untill the user clicks the button
    while (!lostGame)
    {
      //this loop occours when the user has succesfully completed a full "level" of inputs
      if (pass)
      {
        //reset the LCD to display full time remaining
        displyValue(digits[9]);

        //turn off the interupt so that the time does not tick while the user in watching the next level
        cli();

        //increment the score on the LCD
        LCD_command(0x80);
        level += 1;
        LCD_string("Score: ");
        sprintf(text, "%d", level - 1);
        LCD_string(text);

        //delay for a bit so the user can check score
        _delay_ms(500);

        //generate the next position in the sequence
        int nextNum = random(1, 5);

        //add the next position to answerKEY
        answerKEY[level - 1] = nextNum;

        //display answer key to user on LEDs
        playList(answerKEY, level);

        //resest game variables
        pass = false;
        currentPos = 0;
        timeleft = 9;
        //reset timer
        TCNT1 = 0;

        //reenable the interupts
        sei();
      }

      //read in the x and y analog input
      valuex = ADC_Read(PC1);
      valuey = ADC_Read(PC0);

      if (valuex > MAXVAL && last == 0)
      {
        //if joystick is right, turn on relavent LED and sound buzzer
        PORTC &= ~(1 << LEFTLED);
        PORTC &= ~(1 << TOPLED);
        PORTC &= ~(1 << BOTTOMLED);
        PORTC |= (1 << RIGHTLED);
        PORTB |= (1 << BUZZER);
        last = 1;
      }
      if (valuex < MINVAL && last == 0)
      {
        //if joystick is left, turn on relavent LED and sound buzzer
        PORTC |= (1 << LEFTLED);
        PORTC &= ~(1 << TOPLED);
        PORTC &= ~(1 << BOTTOMLED);
        PORTC &= ~(1 << RIGHTLED);
        PORTB |= (1 << BUZZER);
        last = 2;
      }
      if (valuey > MAXVAL && last == 0)
      {
        //if joystick is up, turn on relavent LED and sound buzzer
        PORTC &= ~(1 << LEFTLED);
        PORTC |= (1 << TOPLED);
        PORTC &= ~(1 << BOTTOMLED);
        PORTC &= ~(1 << RIGHTLED);
        PORTB |= (1 << BUZZER);
        last = 3;
      }
      if (valuey < MINVAL && last == 0)
      {
        //if joystick is down, turn on relavent LED and sound buzzer
        PORTC &= ~(1 << LEFTLED);
        PORTC &= ~(1 << TOPLED);
        PORTC |= (1 << BOTTOMLED);
        PORTC &= ~(1 << RIGHTLED);
        PORTB |= (1 << BUZZER);
        last = 4;
      }
      else if (valuey > MINVAL && valuey < MAXVAL && valuex > MINVAL && valuex < MAXVAL)
      {
        //when joystick returns to center, turn off leds and use the previous input to check if it is correct
        PORTC &= ~(1 << LEFTLED);
        PORTC &= ~(1 << TOPLED);
        PORTC &= ~(1 << BOTTOMLED);
        PORTC &= ~(1 << RIGHTLED);
        PORTB &= ~(1 << BUZZER);
        //start of input checking
        //check to see that the last input was not just having the joystick in the center
        if (last != 0)
        {
          //check to see if most recent answer is the same as the relavent value in answer key
          if (answerKEY[currentPos] == last)
          {
            //increment the position in the answer key 
            currentPos += 1;

            //check if the answer key has been completed
            if (currentPos == level)
            {
              //move onto the next level if true
              pass = true;
            }
          }
          else
          {
            //if the user's input was not correct, end the game
            endGame();
          }
        }
        //set last position to 0
        last = 0;
      }
    }
    //turn off all LEDs
    PORTC |= (1 << LEFTLED);
    PORTC |= (1 << TOPLED);
    PORTC |= (1 << BOTTOMLED);
    PORTC |= (1 << RIGHTLED);
    //turn off interupts so the timer does not interupt
    cli();
    //set timer to 0 so it does not tick up
    TCNT1 = 0;
    //turn off 7seg
    displyValue(0);
  }
}

// Timer interupt for the count down
ISR(TIMER1_OVF_vect)
{
  // Display the amount of time left
  displyValue(digits[timeleft]);

  // Check if the time has run out
  if (timeleft == 0)
  {
    // if time has run out end the game
    endGame();
  }
  // reduce the number displayed on the 7seg
  timeleft = timeleft - 1;
}
