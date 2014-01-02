#include "mbed.h"

/* CIS 541 
   Pacemaker.cpp
   Fredy Monterroza
   Abhijeet
   Chitrang
   Rahul
*/

//Units are milliseconds
#define LRI 1000
#define AVI 150
#define PVARP 300
#define VRP 200
#define PULSE_WIDTH 1000

InterruptIn atrial_int(p17);
InterruptIn vent_int(p18);


int a_clk = 0;
int v_clk = 0;
int aSensed = 0; // 0 means that we are expecting Apace or Asense next
int aSetHigh = 0;
int vSetHigh = 0;



#define BM(x) (1<<(x))

const int a_pace = 5; //pin 21
const int v_pace = 3; //pin 23
DigitalOut myled(LED1);
DigitalOut testpin(p6);

void initTimer();
void startTimer();
void initGPIO_outputs();
void setGPIO(const int pinName);
void clearGPIO(const int pinName);
void initInterrupts();
void atrial_stimulus();
void vent_stimulus();

void vent_stimulus()
{
       if (v_clk >= VRP) { //Ignore vSense outside this time interval
        v_clk = 0;
        aSensed = 0;
     }
     else
     {   
        v_clk-=12;
        a_clk-=12;
     }
     //printf("VSense\r\n"); 
}


void atrial_stimulus()
{
    if ((v_clk >= PVARP) && aSensed == 0){ 
        a_clk = 0;
        aSensed = 1;
    }
    else
     {   
        //v_clk-=12;
        a_clk-=12;
     }
   // printf("ASense\r\n");
}

void initInterrupts()
{
    LPC_GPIOINT->IO2IntEnR |= (1<<2)|(1<<1);
    NVIC_EnableIRQ(EINT3_IRQn);
}

void initGPIO_outputs()
{
    LPC_SC->PCONP |= 1<<15;
    LPC_GPIO2->FIODIR |= 1<<5;
    LPC_GPIO2->FIODIR |= 1<<3;
    LPC_GPIO2->FIODIR &= ~(1<<1);
    LPC_GPIO2->FIODIR &= ~(1<<2);
}

void setGPIO(const int pinName)
{
    LPC_GPIO2->FIOPIN |= (1<<pinName);
}

void clearGPIO(const int pinName)
{
    LPC_GPIO2->FIOPIN &= ~(1<<pinName);
}

void initTimer()
{
    // set up OS timer (timer0)
    LPC_SC->PCONP |= BM(1); //power up timer0
    LPC_SC->PCLKSEL0 |= BM(2); // clock = CCLK (96 MHz)
    LPC_TIM0->PR = 48000; // set prescale to 48000 (2048 Hz timer) 
    LPC_TIM0->MR0 = 1; // match0 compare value (32-bit)
    LPC_TIM0->MCR |= BM(0)|BM(1); // interrupt and reset on match0 compare
    NVIC_EnableIRQ(TIMER0_IRQn); // enable timer interrupt
}

void startTimer()
{
    LPC_TIM0->TCR |= BM(1); // reset timer1
    LPC_TIM0->TCR &= ~BM(1); // release reset
    LPC_TIM0->TCR |= BM(0); // start timer
}

void resetTimer()
{
    LPC_TIM0->TCR |= BM(1); // reset timer0
    LPC_TIM0->TCR &= ~BM(1); // release reset
}

/**********************************************************
 ************ GPIO interrupt every 1 ms*********
 ***********************************************************/
extern "C" void EINT3_IRQHandler (void)
{
 //Distinguish interrupts here..
    if(LPC_GPIOINT->IO2IntStatR & 0x04) //interrupt on pin 24
    {
       //printf("VSense\r\n"); 
       if (v_clk >= VRP) { //Ignore vSense outside this time interval
        v_clk = 0;
        aSensed = 0;
       }     
       LPC_GPIOINT->IO2IntClr |= (1<<2);   
    }
    else if (LPC_GPIOINT->IO2IntStatR & 0x02)//interrupt on pin 25
    {
        if ((v_clk >= PVARP) && aSensed == 0){ 
        a_clk = 0;
        aSensed = 1;
        }
        //printf("ASense\r\n");
        LPC_GPIOINT->IO2IntClr |= (1<<1);
    }
}
 
/**********************************************************
 ************ timer interrupt every 1 ms*********
 ***********************************************************/
    
extern "C" void TIMER0_IRQHandler (void) {
    if((LPC_TIM0->IR & 0x01) == 0x01) // if MR0 interrupt
    {
        LPC_TIM0->IR |= (1 << 0); // Clear MR0 interrupt flag
        
        
        /*if (aSetHigh >= 1) {
            clearGPIO(a_pace);
            aSetHigh =0;
            printf("set A low\r\n");
        }
        
          if (vSetHigh >= 1) {
            clearGPIO(v_pace);
            vSetHigh =0;
            printf("set V low\r\n");
        }*/
        
        
       if (v_clk >= (LRI-AVI) && aSensed == 0) {
            a_clk = 0;
            aSensed = 1;  
            //printf("Apace %d\r\n",v_clk);
            
            setGPIO(a_pace);
            wait_us(PULSE_WIDTH);
            clearGPIO(a_pace);
            //wait_us(PULSE_WIDTH);
            //aSetHigh = 1;
        }
        if ((a_clk >= AVI) && aSensed == 1) {
            v_clk = 0;
            aSensed = 0;
            //printf("Vpace %d\r\n",a_clk);
            setGPIO(v_pace);
            wait_us(PULSE_WIDTH);
             clearGPIO(v_pace);
           // wait_us(PULSE_WIDTH);
            //vSetHigh = 1;
           
        }
        v_clk++;
        a_clk++;
        
        /*if(aSetHigh != 0)
            aSetHigh++;
        if(vSetHigh != 0)
            vSetHigh++;*/
    } 
}

int main() {
    
    myled = 0;
    testpin = 0;
    initGPIO_outputs();
    
    /**********************************************************
    ************Initialize timer to interrupt every 1 ms*********
    ***********************************************************/
    
    initTimer();
    startTimer();
    //initInterrupts();
    atrial_int.rise(&atrial_stimulus);
    vent_int.rise(&vent_stimulus);
    while(1) {};
}