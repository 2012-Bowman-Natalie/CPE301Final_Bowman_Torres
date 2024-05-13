/* CPE 301 Final Project: Swamp Cooler
Team Members (2): Natalie Bowman and Jessica Torres
Due: 5/11/2024
*/

//libraries included
#include <Stepper.h>                  //Arduino Stepper Library
#include <LiquidCrystal.h>            //Liquid Crystal Library used to initialize LCD screen
#include <Wire.h>                     //Used for RTC
#include <RTClib.h>                   //Used for RTC
#include <dht.h>                      //DHT Library to use temperature reading sensor

//defined variables
#define RDA 0x80
#define TBE 0x20
#define DHT11_PIN 7

//global variables
volatile int waterlevel;
volatile int flag = 0;
volatile byte buttonState = LOW;
volatile int state = 0;
volatile unsigned int seconds = 0;
volatile float temperature;
volatile float humidity;

const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;          //LCD pins (constants)
const int stepsPerRevolution = 2038;                                 // Defines the number of steps per rotation
const int w_threshold = 150;
const int t_threshold = 22;
const byte interruptPin = 18;                                        //button attachInterrupt setup, pin18

//my_delay declarations for flag/interrupt
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;
//UART declarations
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
//ADC 
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;
//red LED pin 29
volatile unsigned char* port_a = (unsigned char*) 0x22;
volatile unsigned char* ddr_a = (unsigned char*) 0x21;
volatile unsigned char* pin_a = (unsigned char*) 0x20;
//yellow LED pin 31
volatile unsigned char* port_c = (unsigned char*) 0x28;
volatile unsigned char* ddr_c = (unsigned char*) 0x27;
volatile unsigned char* pin_c = (unsigned char*) 0x26;
//water sensor analog pin
volatile unsigned char *pin_f = (unsigned char*) 0x2F;
volatile unsigned char *ddr_f = (unsigned char *) 0x30;
volatile unsigned char *port_f = (unsigned char *) 0x31;
//green LED pin 41
volatile unsigned char* port_g = (unsigned char*) 0x34;
volatile unsigned char* ddr_g = (unsigned char*) 0x33;
volatile unsigned char* pin_g = (unsigned char*) 0x32;
//blue LED pin 43
volatile unsigned char* port_l = (unsigned char*) 0x10B;
volatile unsigned char* ddr_l = (unsigned char*) 0x10A;
volatile unsigned char* pin_l = (unsigned char*) 0x109;
//Port B register pointers
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24;
volatile unsigned char* pin_b  = (unsigned char*) 0x23; 


//initialize libraries used with pin/connection info
Stepper myStepper = Stepper(stepsPerRevolution, 7, 9, 8, 10);        // Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);                           //LCD setup with pins
RTC_DS1307 rtc;
dht DHT;

void U0init(unsigned long U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);      // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

unsigned char U0kbhit(){
  return *myUCSR0A & RDA;
}
unsigned char U0getchar(){
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata){
  while(!(*myUCSR0A & TBE));
  *myUDR0 = U0pdata;
}

//Set up ADC registers/bits
void adc_init(){
  // setup the A register
  *my_ADCSRA |= 0b10000000;     // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111;     // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111;     // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000;     // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111;     // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000;     // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111;     // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000;     // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111;     // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000;     // clear bit 4-0 to 0 to reset the channel and gain bits
}

//ADC set to mode
unsigned int adc_read(unsigned char adc_channel_num){
  *my_ADMUX  &= 0b11100000;         // clear the channel selection bits (MUX 4:0)
  *my_ADCSRB &= 0b11110111;         // clear the channel selection bits (MUX 5)
  if(adc_channel_num > 7){          // set the channel number
    adc_channel_num -= 8;           // set the channel selection bits, but remove the most significant bit (bit 3)
    *my_ADCSRB |= 0b00001000;       // set MUX bit 5
  }
  *my_ADMUX  += adc_channel_num;    // set the channel selection bits
  *my_ADCSRA |= 0x40;               // set bit 6 of ADCSRA to 1 to start a conversion
  while((*my_ADCSRA & 0x40) != 0);  // wait for the conversion to complete
  return *my_ADC_DATA;               // return the result in the ADC data register
}

//delay function uses seconds 
int my_delay(seconds){
  unsigned int ticks = 15624;           //kept as variable for clarity
  *myTCCR1A = 0x00;                     //Setting initial values of counters (16*10^6) / (1024) -1
  *myTCCR1B = 0x00;
  for(int i = 0; i < seconds; i++){     //For loop to count 60 cycles (60 seconds)
    *myTCCR1B &= 0xF8;                  //make sure timer is off
    *myTCNT1 = (unsigned int) (65536 - ticks);  //counter
    *myTCCR1B |= 0x05;                  //Pre-scalar 1024 in order to scale to 1 Hz -> 1 sec
    while((*myTIFR1 & 0x01) == 0);      //Wait for TIFR overflow flag bit to be set
    *myTCCR1B &= 0xF8;                  //Turn off the timer after getting delay
    *myTIFR1 |= 0x01;                   //Clear the flag bit for next use
  }return 1;                            //Return value of 1 when for loop is complete with iterations
}

//Initialize LED states
void LED_Setup(){
  *ddr_a |= 0x01;
  *ddr_c |= 0x01;
  *ddr_g |= 0x01;
  *ddr_l |= 0x01;
}

void lightSwitch(state){
  if (state == 10){            //Yellow LED only ON all others OFF
    *port_g |= (0x01);
    *port_a &= ~(0x01 << 3);
    *port_c &= ~(0x01 << 3);
    *port_l &= ~(0x01 << 3);
  } else if(state == 11){      //Green LED only ON all others OFF
    *port_c |= (0x01);   
    *port_a &= ~(0x01 << 3);
    *port_g &= ~(0x01 << 3);
    *port_l &= ~(0x01 << 3);
  } else if(state == 20){      //Red LED ON pnly all others OFF
    *port_a |= (0x01);
    *port_c &= ~(0x01 << 3);
    *port_g &= ~(0x01 << 3);
    *port_l &= ~(0x01 << 3);
  } else if(state == 22){      //Blue LED only ON all others OFF
    *port_l |= (0x01);
    *port_c &= ~(0x01 << 3);
    *port_g &= ~(0x01 << 3);
    *port_a &= ~(0x01 << 3);
  }else {
    1 == 1;
  }
}

//Function to turn fan MOTOR on/off
void fanMotor(int reading){
  if (reading != 0){
    myStepper.setSpeed(10);   //Rotate CcW slowly at 5RPM
    myStepper.step(stepsPerRevolution);
  } else {
    myStepper.setSpeed(0);   //OFF
    myStepper.step(0);
}

int temperature(){
  int chk = DHT.read11(DHT11_PIN);
  return chk;
}

//Checks if the water levels are within threshold, if not, it triggers following operations
void waterresults(waterlevel){
  if(waterlevel = w_threshold){
    flag = 1;
  } else if(waterlevel < w_threshold){
    flag = 1;
  } else{
    flag = 0;
  }
}
 
//Prints date and time to LCD screen
void myClock(){
  DateTime now = rtc.now();
  lcd.setCursor(0,0);
  lcd.print("TIME: ");
  lcd.print(now.hour(), DEC);
  lcd.print(":");
  lcd.print(now.minute(), DEC);
  lcd.print(":");
  lcd.print(now.second(), DEC); 
  lcd.setCursor(0,1);
  lcd.print("DATE: ");
  lcd.print(now.month(), DEC);
  lcd.print("/");
  lcd.print(now.day(), DEC);
  lcd.print("/");
  lcd.print(now.year(), DEC);
  my_delay(2);
}

//Prints error message to LCD screen if conditions are not met
void errorMessage(){
  attachInterrupt(digitalPintoInterrupt(interruptPin), button_ISR, RISING);
  lightSwitch(20);        //Red LED ON
  fanMotor(0);            //Motor is off
  lcd.setCursor(5,0);     //Place cursor to begin write
  lcd.write("ERROR");     //Print error message
  my_delay(2);         
  lcd.setCursor(2,0);
  lcd.write("WATER LEVEL");
  lcd.setCursor(4, 1);
  lcd.write("TOO LOW");
  my_delay(2);
  lcd.clear()
}

//Updates humidty and temperature readings. Displays onto LCD screen
void statusUpdates(int chk){
    lcd.setCursor(0,0);
    lcd.write("Humidity: ");
    lcd.print(DHT.humidity, 1);
    lcd.write("%");
    lcd.setCursor(0,1);
    lcd.write("Temp: ");
    lcd.print(DHT.temperature, 1);
    lcd.write((char)223);
    my_delay(2);
    lcd.clear();
  }
}

void disabled(){
  lightSwitch(10);        //Yellow LED on
  fanMotor(0);             // make sure fan motor is off
  oneMinute = 0;           //Disables readings
}

void idle(){
  lightSwitch(11);
  fanMotor(0);          // make sure fan motor is off
  waterresults(waterlevel);
  if(flag == 1){
   lcd.clear();
   errorMessage();
   flag = 0;
  }
}

void running(int temperature, int waterlevel){
 lightSwitch(22);
 fanMotor(1);
 if(temperature <= t_threshold){
  idle();
 }else if (waterlevel < w_threshold){
  errorMessage();
}

//Attatchinterupt, used to interupt 
void button_ISR(){
   disabled();
   buttonState = LOW;
  }

//main funtion
void setup() {
  U0init(9600);                  //initialized baud rate
  adc_init();                    //adc initialized
  Wire.begin();                  //Initialize RTC
  rtc.begin();                   //Initialize RTC
  lcd.begin(16,2);               //Set parameters (# of columns, # of rows)
  *ddr_f = 0b10000000;        //initialize water sensor
  LED_Setup();                   //Initialize LEDs
  digitalPintoInterrupt(interruptPin);
}

void loop() {
  myClock();
  int oneMinute = my_delay(60);      //value int set for delay, used for true/false
  int waterlevel = adc_read(7);
  int check = temperatureRead();
  int temperature = temperatureRead();        //Store temperature values recorded

  if(oneMinute == 1){
   lcd.clear();
   statusUpdates(check);
   if(waterlevel > w_threshold && temperature > t_threshold){
    running(temperature, waterlevel);
    attachInterrupt(digitalPintoInterrupt(interruptPin), button_ISR, RISING)
     }else{
       idle();
    }oneMinute = 0;
  }
}








