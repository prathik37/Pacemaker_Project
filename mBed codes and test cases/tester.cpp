#include "mbed.h"

/* CIS 541 
   Tester.cpp
   Fredy Monterroza
   Abhijeet
   Chitrang
   Rahul
   Prathik
*/
InterruptIn atrial_int(p17);
InterruptIn vent_int(p18);
LocalFileSystem local("local");
Serial pc(USBTX, USBRX); // tx, rx

DigitalOut myled(LED1);

//Units are milliseconds
#define MIN_V 1200
#define MIN_A 600

#define PULSE_WIDTH 1000

int global_clk = 0;
int testCaseCounter = 0;
// Test Cases (-1 value indicates we will not consider that element)
//Test Case 1
int timeStamps[4][2];
int outputTimeStamps[4][3];
int outputTCnt = 0;
// Test Case 2 (First ASense Ignored and Second ASense Considered)
//int timeStamps[4][2] = {{200,0},{1400,0},{-1,0},{-1,0}};
// Test Case 3 (First VSense Ignored and Second VSense - Extra Systole - Considered)
//int timeStamps[4][2] = {{150,1},{400,1},{-1,0},{-1,0}};
// Test Case 4 (To Check LRI)
//int timeStamps[4][2] = {{400,0},{-1,0},{-1,0},{-1,0}};
// Test Case 5 (To Check AVI - One after Apace and one after ASense)
//int timeStamps[4][2] = {{1400,0},{-1,0},{-1,0},{-1,0}};

int a_clk = 0;
int v_clk = 0;
int testerSynched = 0;

int aSensed = 0; // 0 means that we are expecting Apace or Asense next
int aSetHigh = 0;
int vSetHigh = 0;



#define BM(x) (1<<(x))

const int a_sense = 5; //pin 21
const int v_sense = 3; //pin 23
//DigitalOut myled(LED1);
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

    //if ((v_clk >= PVARP) && aSensed == 0){ // Receiving VPace
        if(outputTCnt < 4 && testerSynched == 1)
        {
            outputTimeStamps[outputTCnt][0] = global_clk;
            outputTimeStamps[outputTCnt][1] = 1;
            
            if(global_clk <= (timeStamps[outputTCnt][0]+4) && global_clk >= (timeStamps[outputTCnt][0]-4)) 
                outputTimeStamps[outputTCnt][2] = 1;
            else
                outputTimeStamps[outputTCnt][2] = 0; 
            outputTCnt++;
        }
        
        if(testerSynched == 0)
        {
            testerSynched = 1;   
            //printf("Synched");  
            global_clk = 0;   
        }
        //pc.printf("VPace %d\r\n",global_clk);
        
        a_clk= 0;
        v_clk= 0;
        aSensed = 0;
}


void atrial_stimulus()
{
           //if (v_clk >= VRP) { //Receiving APace
        //printf("test\r\n");    
        if(testerSynched == 1)
        {
            
            //pc.printf("APace %d\r\n",global_clk);
            v_clk = 0;
            a_clk = 0;
            aSensed = 1;
            if(outputTCnt < 4)
            {
                outputTimeStamps[outputTCnt][0] = global_clk;
                outputTimeStamps[outputTCnt][1] = 0;
                if(global_clk <= (timeStamps[outputTCnt][0]+4) && global_clk >= (timeStamps[outputTCnt][0]-4)) 
                outputTimeStamps[outputTCnt][2] = 1;
            else
                outputTimeStamps[outputTCnt][2] = 0;
                outputTCnt++;
            }
        }   

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
    
    LPC_GPIO2->FIODIR &= ~(1<<2);
    
    LPC_GPIO2->FIODIR |= 1<<3;
    
    LPC_GPIO2->FIODIR &= ~(1<<1);
    
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
    if(LPC_GPIOINT->IO2IntStatR & (1<<2)) //interrupt on pin 24
    {
        //if (v_clk >= VRP) { //Receiving APace
       // printf("test\r\n");    
        if(testerSynched == 1)
        {
            
            //printf("APace %d\r\n",v_clk);
            v_clk = 0;
            a_clk = 0;
            aSensed = 1;
            LPC_GPIOINT->IO2IntClr |= (1<<2);
        }
            
       //printf("in 24 VSense\r\n");    
    }
    else if (LPC_GPIOINT->IO2IntStatR & 0x02)//interrupt on pin 25
    //else if (LPC_GPIOINT->IO2IntStatR & (1<<1))//interrupt on pin 25
    {
         
         
        //if ((v_clk >= PVARP) && aSensed == 0){ // Receiving VPace
        if(testerSynched == 0)
        {
            testerSynched = 1;   
            //printf("Synched");     
        }
       // printf("VPace %d\r\n",a_clk);
        a_clk= 0;
        v_clk= 0;
        aSensed = 0;
        LPC_GPIOINT->IO2IntClr |= (1<<1);
        //printf("in 25 ASense\r\n");
    }
}
 
/**********************************************************
 ************ timer interrupt every 1 ms*********
 ***********************************************************/
    
extern "C" void TIMER0_IRQHandler (void) {
    if((LPC_TIM0->IR & 0x01) == 0x01) // if MR0 interrupt
    {
        
        LPC_TIM0->IR |= (1 << 0); // Clear MR0 interrupt flag
        //printf("testerNotSynched");
        if(testerSynched == 1)
        {
            if(testCaseCounter<4)
            {
                if(global_clk == timeStamps[testCaseCounter][0])
                {
                    if(timeStamps[testCaseCounter][1]== 2)
                    {
                        setGPIO(a_sense);
                        wait_us(PULSE_WIDTH);
                        clearGPIO(a_sense);
                       // pc.printf("ASense %d\r\n",global_clk);
                        if(outputTCnt < 4)
                        {
                            outputTimeStamps[outputTCnt][0] = global_clk;
                            outputTimeStamps[outputTCnt][1] = 2;
                            if(global_clk <= (timeStamps[outputTCnt][0]+4) && global_clk >= (timeStamps[outputTCnt][0]-4)) 
                                outputTimeStamps[outputTCnt][2] = 1;
                            else
                                outputTimeStamps[outputTCnt][2] = 0;
                            outputTCnt++;
                        }
                    }
                    else if(timeStamps[testCaseCounter][1]== 3)
                    {
                        setGPIO(v_sense);
                        wait_us(PULSE_WIDTH);
                        clearGPIO(v_sense);
                        //pc.printf("VSense %d\r\n",global_clk);
                        if(outputTCnt < 4)
                        {
                            outputTimeStamps[outputTCnt][0] = global_clk;
                            outputTimeStamps[outputTCnt][1] = 3;
                            if(global_clk <= (timeStamps[outputTCnt][0]+4) && global_clk >= (timeStamps[outputTCnt][0]-4)) 
                                outputTimeStamps[outputTCnt][2] = 1;
                            else
                                outputTimeStamps[outputTCnt][2] = 0;
                            outputTCnt++;
                        }
                    }
                    testCaseCounter++;
                }
            }
            v_clk++;
            a_clk++;
        }
        global_clk++;
        if(outputTCnt == 4)
        {
            /*for(int i=0;i<=3;i++)
            {
                pc.printf("%d",outputTimeStamps[i][0]);
                pc.printf("-");
                pc.printf("%d",outputTimeStamps[i][1]);
                pc.printf("\n");
             }*/
            char str[10];
            FILE *fp1 = fopen("/local/out.txt", "a");
            for(int i=0;i<=3;i++)
            {
                sprintf(str,"%d", outputTimeStamps[i][0]);  
                fprintf(fp1,str);
                
                fprintf(fp1,",");
                
                sprintf(str,"%d", outputTimeStamps[i][1]);  
                fprintf(fp1,str);
                
                fprintf(fp1,",");
                sprintf(str,"%d", outputTimeStamps[i][2]);  
                fprintf(fp1,str);
                
                fprintf(fp1,"\n");
                
            }
            fprintf(fp1,"\n");
            fprintf(fp1,"-------------------------");
            fprintf(fp1,"\n");
            fclose(fp1);
            
            testerSynched = 0;
            global_clk = 0;
            outputTCnt = 5;
        }
        
    } 
}

int main() {
    
    myled = 0;
    testpin = 0;
    initGPIO_outputs();
    
    //FILE *fp = fopen("/local/out.txt", "w");  // Open "out.txt" on the local file system for writing
    //fprintf(fp, "Hello World!");
    //fclose(fp);
    
    
    /**********************************************************
    ************Initialize timer to interrupt every 1 ms*********
    ***********************************************************/
        
    char buffer [128];
    
    FILE *fp = fopen("/local/out1.txt", "r");
    bool ret =  (fgets(buffer, 64, fp));
    
   if (ret)
   { 
    myled = 1;
    char * pch;
    pch = strtok (buffer,",");
    int cnt = 0;
    while (pch != NULL)
    {
    //fprintf(fp1, pch);
    //fprintf(fp1,"----");
    if(cnt % 2 == 0)
    timeStamps[cnt/2][0] = atoi(pch);
    else
    {
    if(strcmp (pch,"AP") == 0)
        timeStamps[cnt/2][1] = 0;
    else if(strcmp (pch,"VP") == 0)
        timeStamps[cnt/2][1] = 1;
    else if(strcmp (pch,"AS") == 0)
        timeStamps[cnt/2][1] = 2;
    else if(strcmp (pch,"VS") == 0)
        timeStamps[cnt/2][1] = 3;
    }
    pch = strtok (NULL, ",");
    cnt++;
    }
   }
    fclose(fp);
   
   /*for(int i=0;i<=1;i++)
    {
        pc.printf("%d",timeStamps[i][0]);
     }*/
    //wait(0.5);
    initTimer();
    startTimer();
    //initInterrupts();
    atrial_int.rise(&atrial_stimulus);
    vent_int.rise(&vent_stimulus);
    
    /*
    
    char str[10];
    
    for(int i=0;i<=1;i++)
    {
        sprintf(str,"%d", inputs[i][0]);  
        fprintf(fp1,str);
        
        fprintf(fp1,",");
        
        sprintf(str,"%d", inputs[i][1]);  
        fprintf(fp1,str);
        
        fprintf(fp1,"\n");
    }
    fclose(fp1);
    
    */
    
    while(1) {};
}
