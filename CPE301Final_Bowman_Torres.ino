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
#define DHT_PIN 7
#define DHT_TYPE

//global variables
volatile int waterlevel = 0;
volatile int flag = 0;
volatile int waterinfo;
volatile byte buttonState = LOW;
volatile int state = 0;
volatile unsigned int seconds = 0;

const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;          //LCD pins (constants)
const int stepsPerRevolution = 2038;                                 // Defines the number of steps per rotation
const byte interruptPin = 18;                                        //button attachInterrupt setup, pin18

//my_delay declarations for flag/interrupt
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;
volatile unsigned char *portDDRB = (unsigned char *) 0x24;
volatile unsigned char *portB = (unsigned char *) 0x25;
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

//initialize libraries used with pin/connection info
Stepper myStepper = Stepper(stepsPerRevolution, 7, 9, 8, 10);        // Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);                           //LCD setup with pins
RTC_DS1307 RTC;
DHT dht(DHT_PIN, DHT_TYPE);

//Degrees symbol custom character
byte customChar[8] = {
  0b00100,
  0b01010,
  0b10001,
  0b01010,
  0b00100,
  0b00000,
  0b00000,
  0b00000
};

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
void fanMotor(int buttonsState){
  if (buttonState == 0){          //stop motor if function is called BY INTERRUPT
    myStepper.setSpeed(0);
    myStepper.step(0);
  } else {                        //Rotate CW slowly at 5RPM
    myStepper.setSpeed(5);
    myStepper.step(-stepsPerRevolution);
  }
}

//Checks if the water levels are within threshold, if not, it triggers following operations
void waterresults(waterinfo){
  *ddr_f = 0b10000000;        //initialize water sensor
  unsigned char flag = 0;
  int threshold = 100;
  if(waterinfo = threshold){
    flag = 1;
  } else if(waterlevel < threshold){
    flag = 1;
  } else{
    flag = 0;
  }
}

//Prints date and time to LCD screen
void myClock(){
  DateTime now = RTC.now();
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
  delay(1000);
}

//Prints error message to LCD screen if conditions are not met
void errorMessage(){
  lightSwitch(20);        //Red LED ON
  lcd.begin(16,2);        //Set parameters (# of columns, # of rows)
  lcd.setCursor(5,0);     //Place cursor to begin write
  lcd.write("ERROR");     //Print error message
  delay(1000);         
  lcd.setCursor(2,0);
  lcd.write("WATER LEVEL");
  lcd.setCursor(4, 1);
  lcd.write("TOO LOW");
  delay(1000);
  lcd.clear()
}

//Updates humidty and temperature readings. Displays onto LCD screen
void statusUpdates(int humidity, int temperature){
    lcd.createChar(1, customChar);
    int chk = DHT.read11(DHT11_PIN);
    lcd.setCursor(0,0);
    lcd.write("Humidity: ");
    lcd.write(DHT.humidity);
    lcd.setCursor(0,1);
    lcd.write("Temp in C: ");
    lcd.write(DHT.temperature);
    lcd.write((byte)1);
    delay(500);
    lcd.clear();
  }
}



void disabled(){
  lightSwitch(10);        //Greeb LED on
  // make sure fan motor is off include code later
}

void idle(){
  lightSwitch(11);
  // make sure fan motor is off include code later
}

//Attatchinterupt, used to interupt 
void button_ISR(){
  fanMotor(0);
}

//main funtion
void setup() {
  U0init(9600);                  //initialized baud rate
  adc_init();                    //adc initialized
  Wire.begin();                  //Initialize RTC
  RTC.begin();                   //Initialize RTC
  LED_Setup();                   //Initialize LEDs
  if(flag == 1){
    errorMessage();
    flag = 0;
  }
  int temperature = tempRead();        //Store temperature values recorded
    if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  //will go under function under select condition
  digitalPintoInterrupt(interruptPin);
  attachInterrupt(digitalPintoInterrupt(interruptPin), button_ISR, RISING);

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  int oneMinute = my_delay(60000);      //value int set for delay, used for true/false
  int input = adc_read(7);
  waterresults(input);
}







