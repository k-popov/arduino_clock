// include the library code:
#include <SerialLCD.h>
#include <SoftwareSerial.h> //this is a must

SerialLCD slcd(11,12);//this is a must, assign soft serial pins
byte seconds = 0;
byte minutes = 0;
byte hours = 0;

//record the previous time values to eliminate duplicate output
byte prev_seconds = 0;
byte prev_minutes = 0;
byte prev_hours = 0;

const int button_pin = 3;
const int debounce_delay = 30;

void timer2OverflowHandler() {
  cli();
  if ( (++seconds) == 60 ) {
    seconds = 0;
    if ( (++minutes) == 60 ) {
      minutes = 0;
      if ( (++hours) == 24 )
        hours = 0;
    }
  }
  sei();
}

ISR(TIMER1_COMPA_vect){
  timer2OverflowHandler();
}

void setup() {
/*
* setting up timer1
*/
  cli();
  //CTC vaweform mode | 1024 prescaler
  //  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
  //will start the timer on button press
  TCCR1B = 0x00; //now the timer is stopped
  //normal mode, all waveform ganeration bits are 0
  TCCR1A = 0x00;
  //reset the timer
  TCNT1 = 0x0000;
  //clear timer1 interrupt flags
  TIFR1 = 0x00;
  //counter seed for 1 second triggering
  OCR1A = 15624;
  //enable output compare interrupt
  TIMSK1 = (1 << OCIE1A);
  sei();

  //configure LCD display
  slcd.begin();
  slcd.backlight();
  slcd.setCursor(0, 0);
  slcd.print("Time since start");
  slcd.setCursor(0, 1);
  slcd.print("0 h 0 m 0 s");
}

void printTime() {
  //print time only if the corresponding value changed
  if (seconds != prev_seconds) {
    //if second value changed
    slcd.setCursor(8, 1);
    slcd.print("  s");
    slcd.setCursor(8, 1);
    slcd.print((long unsigned int)seconds, DEC);
    prev_seconds = seconds;
    if (minutes != prev_minutes) {
      //if minutes value changed
      slcd.setCursor(4, 1);
      slcd.print("  m");
      slcd.setCursor(4, 1);
      slcd.print((long unsigned int)minutes, DEC);
      prev_minutes = minutes;
      if (hours != prev_hours) {
        //if hours value changed
        slcd.setCursor(0, 1);
        slcd.print("  h");
        slcd.setCursor(0, 1);
        slcd.print((long unsigned int)hours, DEC);
        prev_hours = hours;
      }
    }
  }
}

int readButton(const int btn_pin) {
  byte button_state = digitalRead( btn_pin );
  delay(debounce_delay);
  if ( digitalRead(btn_pin) == button_state )
    return button_state;
}

void loop() {
  while (! TCCR1B)
  //wait for button press and start timer
    if ( readButton(button_pin) )
      //CTC vaweform mode | 1024 prescaler
      TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
  for (;;) {
    printTime();
  }
}
