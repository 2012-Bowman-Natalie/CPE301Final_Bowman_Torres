/* CPE 301 Final Project: Swamp Cooler
Team Members (2): Natalie Bowman and Jessica Torres
Due: 5/11/2024
*/
#define RDA 0x80
#define TBE 0x20

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

void setup() {
  // put your setup code here, to run once:
  U0init(9600);
  int temperature = tempRead();        //Store temperature values recorded
}

void loop() {
  // put your main code here, to run repeatedly:

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
