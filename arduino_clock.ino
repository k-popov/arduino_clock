// include the library code:
#include <SerialLCD.h>
#include <SoftwareSerial.h>	//this is a must

#define TIMER_SEED 15632
#define NULL ((void *)0)

SerialLCD slcd(11, 12);		//this is a must, assign soft serial pins
byte seconds = 0;
byte minutes = 0;
byte hours = 0;

//record the previous time values to eliminate duplicate output
byte prev_seconds = 0;
byte prev_minutes = 0;
byte prev_hours = 0;

//alarm settings. Do not check seconds.
//alarm_hours. 1 in major bit (128) sets alarm ON (manual switch)
//Something like    7 | (1 << 7)  may be used to set alarm at 7 AM
//Default if OFF
byte alarm_hours = 0;
//alarm_minutes. 1 in major bit (128) indicates alarm action should be
//taken. This is required for turning off the light or buzzer during
//the minute set as alarm minute and then restore ON action
//after the minutes value changes.
//Default is "action enabled".
byte alarm_minutes = ( 0 | (1 << 7) );


const int button_pin = 3;
const int debounce_delay = 30;
const int analog_input_pin = A0;
const int relay_pin = 5;

void timer2OverflowHandler() {
    cli();
    if ((++seconds) == 60) {
	seconds = 0;
	if ((++minutes) == 60) {
	    minutes = 0;
	    if ((++hours) == 24)
		hours = 0;
	}
    }
    sei();
}

ISR(TIMER1_COMPA_vect) {
    timer2OverflowHandler();
}

int readButton(const int btn_pin) {
    byte button_state = digitalRead(btn_pin);
    delay(debounce_delay);
    if (digitalRead(btn_pin) == button_state)
	return button_state;
}

byte getHours(int analog_value) {
    if (analog_value >= 230)
	return 23;		//can't set more than 23 hours
    return byte(analog_value / 10);	//very time-consuming but size-effective
}

byte getMinutes(int analog_value) {
    if (analog_value >= 226)
	return 59;		//can't set more than 59 minutes
    return byte(analog_value >> 2);	//fast division
}

void printTime() {
    //print time only if the corresponding value changed
    if (seconds != prev_seconds) {
	//if second value changed
	slcd.setCursor(6, 1);
	slcd.print("  ");
	slcd.setCursor(6, 1);
	slcd.print((long unsigned int) seconds, DEC);
	prev_seconds = seconds;
	if (minutes != prev_minutes) {
	    //if minutes value changed
	    slcd.setCursor(3, 1);
	    slcd.print("  ");
	    slcd.setCursor(3, 1);
	    slcd.print((long unsigned int) minutes, DEC);
	    prev_minutes = minutes;
	    if (hours != prev_hours) {
		//if hours value changed
		slcd.setCursor(0, 1);
		slcd.print("  ");
		slcd.setCursor(0, 1);
		slcd.print((long unsigned int) hours, DEC);
		prev_hours = hours;
	    }
	}
    }
}

void setTime(char* request, byte* hrs_ptr, byte* min_ptr, byte* sec_ptr) {
    byte prev_time_value = 0;

    // print the request message
    slcd.setCursor(0,0);
    slcd.print(request);

    //set hours
    slcd.setCursor(0, 1);
    while (!readButton(button_pin)) {
	*hrs_ptr = getHours(analogRead(analog_input_pin));
	if (prev_time_value != *hrs_ptr) {
	    prev_time_value = *hrs_ptr;
	    slcd.setCursor(0, 1);
	    slcd.print("  ");
	    slcd.setCursor(0, 1);
	    slcd.print((long unsigned int) (*hrs_ptr), DEC);
	}
    }

    slcd.setCursor(2, 1);
    slcd.print(":");
    while (readButton(button_pin));	//wait for button release

    //set minutes
    slcd.setCursor(3, 1);
    prev_time_value = 0;
    while (!readButton(button_pin)) {
	*min_ptr = getMinutes(analogRead(analog_input_pin));
	if (*min_ptr != prev_time_value) {
	    prev_time_value = *min_ptr;
	    slcd.setCursor(3, 1);
	    slcd.print("  ");
	    slcd.setCursor(3, 1);
	    slcd.print((long unsigned int) (minutes), DEC);
	}
    }
    slcd.setCursor(5, 1);
    slcd.print(":00");

    while (readButton(button_pin));	//wait for button release

           if (sec_ptr)			//if seconds pointer is not null
        *sec_ptr = 0;			//reset seconds to 0
}

void setup() {
/*
* setting up timer1
*/
    cli();
    //CTC vaweform mode | 1024 prescaler
    //  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
    //will start the timer on button press
    TCCR1B = 0x00;		//now the timer is stopped
    //normal mode, all waveform ganeration bits are 0
    TCCR1A = 0x00;
    //reset the timer
    TCNT1 = 0x0000;
    //clear timer1 interrupt flags
    TIFR1 = 0x00;
    //counter seed for 1 second triggering
    OCR1A = TIMER_SEED;
    //enable output compare interrupt
    TIMSK1 = (1 << OCIE1A);
    sei();

    // initialize the relay (light switch)
    pinMode(relay_pin, OUTPUT);
    digitalWrite(relay_pin, LOW);

    //initialize LCD disply
    slcd.begin();
    slcd.backlight();

    //set hours and minutes, reset seconds to 0 and wait for start
    setTime("Setting time:", &hours, &minutes, &seconds);
    slcd.setCursor(0, 0);
    slcd.print("Press to start. ");

    while (!readButton(button_pin));	//wait for button press

    //CTC vaweform mode | 1024 prescaler. Start the timer.
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);

    //print some stuff on LCD display while the timer is already running
    slcd.setCursor(0, 0);
    slcd.print("Current time    :");
}

void loop() {
    printTime();
    if (alarm_hours & 0b10000000) {
        //if alarm is on (check major bit)
        if ( ( (alarm_hours & 0b01111111) == hours) && 
             ( (alarm_minutes & 0b01111111) == minutes) ) {
            //time to weke'em up!
            if (alarm_minutes & 0b10000000) {
                //user didn't press button to snooze the alarm
                digitalWrite(relay_pin, HIGH);
                if ( readButton(button_pin) ) {
                    //turn the light off now (snooze)
                    alarm_minutes &= 0b01111111;
                    digitalWrite(relay_pin, LOW);
                }
            }
        }
        else
            //the tima has changed. Re-enable the snoozed alarm
            alarm_minutes |= (1 << 7);
    }
}
