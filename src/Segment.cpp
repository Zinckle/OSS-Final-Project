#include"Segment.h"
#include"ShiftReg.h"

#define LATCH 3
#define DATA 2
#define CLOCK 4

void displyValue(byte value)
{
    //set latch to low
      PORTB&=~(1<<LATCH);
      //shift the data in shift register
      myShiftOut(DATA, CLOCK, LSBFIRST, value);
      //set the latch to high again
      PORTB|=(1<<LATCH);
}
