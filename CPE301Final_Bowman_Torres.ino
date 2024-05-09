/* CPE 301 Final Project: Swamp Cooler
Team Members (2): Natalie Bowman and Jessica Torres
Due: 5/11/2024
*/

//Includes the Arduino Stepper Library
#include <Stepper.h>
// Defines the number of steps per rotation
const int stepsPerRevolution = 2038;
// Creates an instance of stepper class
// Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
Stepper myStepper = Stepper(stepsPerRevolution, 7, 9, 8, 10);


#define RDA 0x80
#define TBE 0x20

//UART declarations
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
//red LED pin 29
volatile unsigned char* port_a = (unsigned char*) 0x22;
volatile unsigned char* ddr_a = (unsigned char*) 0x21;
volatile unsigned char* pin_a = (unsigned char*) 0x20;
//yellow LED pin 31
volatile unsigned char* port_c = (unsigned char*) 0x28;
volatile unsigned char* ddr_c = (unsigned char*) 0x27;
volatile unsigned char* pin_c = (unsigned char*) 0x26;
//green LED pin 41
volatile unsigned char* port_g = (unsigned char*) 0x34;
volatile unsigned char* ddr_g = (unsigned char*) 0x33;
volatile unsigned char* pin_g = (unsigned char*) 0x32;
//blue LED pin 43
volatile unsigned char* port_l = (unsigned char*) 0x10B;
volatile unsigned char* ddr_l = (unsigned char*) 0x10A;
volatile unsigned char* pin_l = (unsigned char*) 0x109;

//button attachInterrupt setup, pin18
const byte interruptPin = 18;  
volatile byte buttonState = LOW;

//main funtion
void setup() {
  // put your setup code here, to run once:
  U0init(9600);
  *ddr_a |= 0x01;
  *ddr_c |= 0x01;
  *ddr_g |= 0x01;
  *ddr_l |= 0x01;
  int temperature = tempRead();        //Store temperature values recorded

  //will go under function under select condition
  digitalPintoInterrupt(interruptPin);
  attachInterrupt(digitalPintoInterrupt(interruptPin), button_ISR, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  int oneMinute = my_delay(60000);      //value int set for delay, used for true/false

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
    //Yellow LED only ON
    *port_g |= (0x01);   //turns on yellow light all others off
    *port_a &= ~(0x01 << 3);
    *port_c &= ~(0x01 << 3);
    *port_l &= ~(0x01 << 3);
  } else if{state == 11}{
    //red LED on
  } else if(){
    
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

void waterLevel(){
  
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

void clock(){
  
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
  
  *port_a |= (0x01);   //turns on red light all others off
  *port_c &= ~(0x01 << 3);
  *port_g &= ~(0x01 << 3);
  *port_l &= ~(0x01 << 3);
}
//Updates humidty and temperature readings. Displays onto LCD screen
void statusUpdates(int humidity, int temperature){
  //Scrolls text to the left to display full text
  for (int i = 13; i >= 1; i--){
    lcd.setCursor(0,0);
    lcd.write("Humidity: ");
    lcd.write(humidity);
    lcd.setCursor(i,1);
    lcd.write("Temperature: ");
    lcd.write(temperature);
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
