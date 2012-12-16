// include the library code:
#include <SerialLCD.h>
#include <SoftwareSerial.h>	//this is a must

#define TIMER_SEED 15632
#define NULL ((void *)0)

//free alarm setting variable from flags. Return time only
//see "alarm_hours" and "alarm_minutes" below
#define getAlarmTime(X) X & 0b01111111
//free alarm setting from time. Return flag value only
//see "alarm_hours" and "alarm_minutes" below
#define getAlarmFlag(X) X & 0b10000000

SerialLCD slcd(11, 12);		//this is a must, assign soft serial pins
byte seconds = 0;
byte minutes = 0;
byte hours = 0;

//record the previous time values to minimize duplicate output
byte prev_seconds = 0;

//alarm settings. Not checking seconds
byte alarm_hours = 0;
byte alarm_minutes = 0;
byte alarm_flags = 0; //store all the alarm falgs

//alarm is globally enabled (bit 0)
#define ALARM_ENABLED 0
//alarm is currently enabled, not turned off by "snooze" (bit 1)
#define ALARM_UNSNOOZED 1
//alarm is currently active (ringing, light is on, etc...) (bit 2)
#define ALARM_ACTIVE 2

#define getAlarmFlag(flag_variable, requested_parameter) (flag_variable & (1 << requested_parameter) )
#define setAlarmFlag(flag_variable, requested_parameter) (flag_variable |= (1 << requested_parameter) )
#define unsetAlarmFlag(flag_variable, requested_parameter) (flag_variable &= (255 ^ (1 << requested_parameter) ) )

const int button_pin = 3;
const int debounce_delay = 50;
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
    //print time only if seconds value changed
    if (seconds != prev_seconds) {
	//if second value changed
        slcd.setCursor(0, 1);
        slcd.print("  :  :          ");
        slcd.setCursor(0, 1);
        slcd.print((long unsigned int) hours, DEC);
        slcd.setCursor(3, 1);
        slcd.print((long unsigned int) minutes, DEC);
        slcd.setCursor(6, 1);
        slcd.print((long unsigned int) seconds, DEC);
	prev_seconds = seconds;
    }
}

void setTime(char* request, byte* hrs_ptr, byte* min_ptr, byte* sec_ptr) {
    byte prev_time_value = 0;

    while ( readButton(button_pin) ); //wait till button is released

    // print the request message
    slcd.setCursor(0,0);
    slcd.print("                "); //clear line
    slcd.setCursor(0,0);
    slcd.print(request);

    //set hours
    slcd.setCursor(0, 1);
    slcd.print("                "); //clear line
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

    while ( readButton(button_pin) ); //wait till button is released

    slcd.setCursor(2, 1);
    slcd.print(":");

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
	    slcd.print((long unsigned int) (*min_ptr), DEC);
	}
    }
    slcd.setCursor(5, 1);
    slcd.print(":00");

    while (readButton(button_pin));	//wait for button release

           if (sec_ptr)			//if seconds pointer is not null
        *sec_ptr = 0;			//reset seconds to 0
}

void alarmOn() {
    digitalWrite(relay_pin, HIGH);
    setAlarmFlag(alarm_flags, ALARM_ACTIVE);
}

void alarmOff() {
    digitalWrite(relay_pin, LOW);
    unsetAlarmFlag(alarm_flags, ALARM_ACTIVE);
}

int checkAlarm() {
    if ( getAlarmFlag(alarm_flags, ALARM_ENABLED) ) {
        //if alarm is globally enabled
        if ( (alarm_hours == hours) && 
             (alarm_minutes == minutes) ) {
            //time to weke'em up!
            if (getAlarmFlag(alarm_flags, ALARM_UNSNOOZED)) {
                //user didn't press button to snooze the alarm
                //here we already may turn alarm signal on but we will return 1 later
                if ( readButton(button_pin) ) {
                    //turn the light off now (snooze)
                    unsetAlarmFlag(alarm_flags, ALARM_UNSNOOZED);
                    return 0; //say it's time to turn alarm off
                }
                return 1; //Yes, turn alarm signal on!
            }
        }
        else
            //the time has changed. Re-enable the snoozed alarm
            setAlarmFlag(alarm_flags, ALARM_UNSNOOZED);;
    }
    return 0; //no alarm enabled
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
    while ( readButton(button_pin) ); //wait till button is released
}

void loop() {
    printTime();
    if ( checkAlarm() )
        alarmOn();
    else
        alarmOff();
    if ( readButton(button_pin) ) {
        //alarm setting requested
        //set actual alarm time values
        setTime("Setting alarm:", &alarm_hours, &alarm_minutes, (byte*)NULL);
        //set "alarm enabled" flag
        setAlarmFlag(alarm_flags, ALARM_ENABLED);
        //set "alarm not snoozed" flag
        setAlarmFlag(alarm_flags, ALARM_UNSNOOZED);
    }
}
