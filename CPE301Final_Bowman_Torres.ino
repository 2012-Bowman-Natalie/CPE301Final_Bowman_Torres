/* CPE 301 Final Project: Swamp Cooler
Team Members (2): Natalie Bowman and Jessica Torres
Due: 5/11/2024
*/

//libraries included
#include <Stepper.h>                  //Arduino Stepper Library
#include <LiquidCrystal.h>            //Liquid Crystal Library used to initialize LCD screen

//defined variables
#define RDA 0x80
#define TBE 0x20

const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;          //LCD pins (constants)
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);                           //LCD setup with pins
const int stepsPerRevolution = 2038;                                 // Defines the number of steps per rotation
Stepper myStepper = Stepper(stepsPerRevolution, 7, 9, 8, 10);        // Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
const byte interruptPin = 18;                                        //button attachInterrupt setup, pin18
volatile byte buttonState = LOW;

volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
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

//Degrees symbol
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

//global variables
int waterlevel = 0;
int flag = 0;

//main funtion
void setup() {
  // put your setup code here, to run once:
  U0init(9600);          //initialized baud rate
  //Set up LEDs
  *ddr_a |= 0x01;
  *ddr_c |= 0x01;
  *ddr_g |= 0x01;
  *ddr_l |= 0x01;
  //adc initialized
  adc_init();
  *ddr_f = 0b10000000;
  if(flag == 1){
    errorMessage();
    flag = 0;
  }
  int temperature = tempRead();        //Store temperature values recorded

  //will go under function under select condition
  digitalPintoInterrupt(interruptPin);
  attachInterrupt(digitalPintoInterrupt(interruptPin), button_ISR, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  int oneMinute = my_delay(60000);      //value int set for delay, used for true/false
  int input = adc_read(7);
  waterresults(input);
}

//Attatchinterupt, used to interupt 
void button_ISR(){
  fanMotor(0);
}

void disabled(){
  lightSwitch(10);
  // make sure fan motor is off include code later
}

void lightSwitch(int state){
  if (state == 10){
    //Yellow LED only ON all others OFF
    *port_g |= (0x01);
    *port_a &= ~(0x01 << 3);
    *port_c &= ~(0x01 << 3);
    *port_l &= ~(0x01 << 3);
  } else if(state == 11){
    //Greeen LED only ON all others OFF
    *port_c |= (0x01);   
    *port_a &= ~(0x01 << 3);
    *port_g &= ~(0x01 << 3);
    *port_l &= ~(0x01 << 3);
  } else if(state == 20){
    //Red LED ON pnly all others OFF
    *port_a |= (0x01);
    *port_c &= ~(0x01 << 3);
    *port_g &= ~(0x01 << 3);
    *port_l &= ~(0x01 << 3);
  } else if(state == 22){
    //Blue LED only ON all others OFF
    *port_l |= (0x01);
    *port_c &= ~(0x01 << 3);
    *port_g &= ~(0x01 << 3);
    *port_a &= ~(0x01 << 3);
  }else {
    1 == 1;
  }
}

void updates(){
  
}

void idle(){
  *port_c |= (0x01);   //turns on green light all others off
  *port_a &= ~(0x01 << 3);
  *port_g &= ~(0x01 << 3);
  *port_l &= ~(0x01 << 3);
  // make sure fan motor is off include code later
}

//check and return temperature value
int tempRead(){
  int chk = DHT.read11(DHT11_PIN);
  return chk;
}

//Function to turn fan MOTOR on/off
void fanMotor(int buttonsState){
  if (buttonState == 0){
    //stop motor if function is called BY INTERRUPT
    myStepper.setSpeed(0);
    myStepper.step(0);
  } else {
    //Rotate CW slowly at 5RPM
    myStepper.setSpeed(5);
    myStepper.step(-stepsPerRevolution);
  }
}

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

void errorMessage(){
  lcd.begin(16,2);      //Set parameters (# of columns, # of rows)
  lcd.setCursor(5,0);   //Place cursor to begin write
  lcd.write("ERROR");   //Print error message
  my_delay(1000);          //  !!!!!! check if valid delay function
  lcd.setCursor(2,0);
  lcd.write("WATER LEVEL");
  lcd.setCursor(4, 1);
  lcd.write("TOO LOW");
  my_delay(1000);
  lcd.clear();

  lightSwitch(20);        //Red LED ON
}

//Updates humidty and temperature readings. Displays onto LCD screen
void statusUpdates(int humidity, int temperature){
  //Scrolls text to the left to display full text
  for (int i = 13; i >= 1; i--){
    int chk = DHT.read11(DHT11_PIN);
    lcd.setCursor(0,0);
    lcd.write("Humidity: ");
    lcd.write(DHT.humidity);
    lcd.setCursor(i,1);
    lcd.write("Temperature: ");
    lcd.write(DHT.temperature);
    my_delay(300);
    lcd.clear();
  }
}

//delay function uses milliseconds 
int my_delay(unsigned int millis){
  unsigned int ticks = 16000;         //kept as variable for clarity
  for(int i = 0; i < millis; i++){    //For loop to count 60 cycles (60 seconds)
    *myTCCR1B &= 0xF8;                //make sure timer is off
    *myTCNT1 = (unsigned int) (65536 - ticks);  //counter
    *myTCCR1B |= 0x01;                //Pre-scalar 1 in order to scale to 1 kHz -> 0.001 sec
    while((*myTIFR1 & 0x01) == 0);    //Wait for TIFR overflow flag bit to be set
    *myTCCR1B &= 0xF8;                //Turn off the timer after getting delay
    *myTIFR1 |= 0x01;                 //Clear the flag bit for next use
  }return 1;                          //Return value of 1 when for loop is complete with iterations
}
void waterresults(unsigned int waterinfo){
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

void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

void U0init(unsigned long U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while(!(*myUCSR0A & TBE));
  *myUDR0 = U0pdata;
}


unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

