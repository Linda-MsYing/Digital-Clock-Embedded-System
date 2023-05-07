#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hw_memmap.h"
#include "debug.h"
#include "gpio.h"
#include "hw_i2c.h"
#include "hw_types.h"
#include "i2c.h"
#include "pin_map.h"
#include "sysctl.h"
#include "systick.h"
#include "interrupt.h"
#include "uart.h"
#include "hw_ints.h"
#include "pwm.h"

#define SYSTICK_FREQUENCY		1000			//1000hz
#define   FASTFLASHTIME			(uint32_t) 600000
#define   SLOWFLASHTIME			(uint32_t) FASTFLASHTIME*5
#define	I2C_FLASHTIME				500				//500mS
#define GPIO_FLASHTIME			300				//300mS
#define PWM_PERIOD					4000
//*****************************************************************************
//
//I2C GPIO chip address and resigster define
//
//*****************************************************************************
#define TCA6424_I2CADDR 					0x22
#define PCA9557_I2CADDR						0x18

#define PCA9557_INPUT							0x00
#define	PCA9557_OUTPUT						0x01
#define PCA9557_POLINVERT					0x02
#define PCA9557_CONFIG						0x03

#define TCA6424_CONFIG_PORT0			0x0c
#define TCA6424_CONFIG_PORT1			0x0d
#define TCA6424_CONFIG_PORT2			0x0e

#define TCA6424_INPUT_PORT0				0x00
#define TCA6424_INPUT_PORT1				0x01
#define TCA6424_INPUT_PORT2				0x02

#define TCA6424_OUTPUT_PORT0			0x04
#define TCA6424_OUTPUT_PORT1			0x05
#define TCA6424_OUTPUT_PORT2			0x06


void  PWM_Init(void);
void 		Delay(uint32_t value);
void 		S800_GPIO_Init(void);
uint8_t 	I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData);
uint8_t 	I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr);
void		S800_I2C0_Init(void);
void 		S800_UART_Init(void);
void UARTStringPut(const char *cMessage);
//功能函数
void startup();
void time12run();
void time12flash();
void time24run();
void time24flash();
void dateflash();
void ddlflash();
void daojishurun();
void daojishuflash();
//重置，重启，待机
void chongzhi();
//12小时时间，24小时时间，日期，倒计时
void sound(uint16_t hz);
void musicplay();
//蜂鸣器响，唱小星星

//计时变量
uint8_t time[6]={0,0,0,0,0,0};
uint8_t time24[6]={1,2,0,0,0,0};
uint8_t ddl[6]={0,0,0,1,0,0};
uint8_t daotime[6]={0,0,1,5,0,0};
uint8_t date[6]={2,2,0,6,1,2};
//计数器
uint8_t i=0x0;
uint8_t count =0x0;
uint8_t daojishiend=0x0;
//开关量
uint8_t kaiguan =0x0;
uint8_t kaiguan2 =0x0;
uint8_t button1=0x0;
uint8_t button2=0x0;
uint8_t button3=0x0;
uint8_t button4=0x0;
uint8_t button5=0x0;
uint8_t button6=0x0;
uint8_t button7=0x0;
//闹钟响应
bool ddlreach=false;
bool closealarm=false;
//通信时间、日期
char uart_receive_string[30];
char uart_put_string12[9]="00:00:00";
char uart_put_string24[9]="12:00:00";
char uart_put_date[9]="22-06-12";
char uart_put_alarm[9]="00:01:00";
//音乐曲谱
uint32_t tone[7] = {262,294,330,349,392,440,494};
uint8_t littlestar[28]={1,1,5,5,6,6,5,4,4,3,3,2,2,1,5,5,4,4,3,3,2,5,5,4,4,3,3,2};
uint16_t play=0;
//重启
bool reset;
	
	
volatile uint16_t systick_10ms_couter,systick_100ms_couter,systick_1s_couter;
volatile uint8_t	systick_10ms_status,systick_100ms_status,systick_1s_status;
volatile uint8_t result,cnt,key_value,gpio_status;
uint32_t ui32SysClock,ui32IntPriorityGroup,ui32IntPriorityMask;
uint32_t ui32IntPrioritySystick,ui32IntPriorityUart0;

volatile uint8_t rightshift = 0x01;
uint32_t ui32SysClock;
uint8_t seg7[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x58,0x5e,0x079,0x71,0x5c};
uint8_t seg7hello[]={0x76,0x79,0x38,0x38,0x5C};
int main(void)
{
	//初始化流程
	volatile uint16_t	i2c_flash_cnt,gpio_flash_cnt;
	ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ |SYSCTL_OSC_INT | SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480), 20000000);
	
  SysTickPeriodSet(ui32SysClock/SYSTICK_FREQUENCY);
	SysTickEnable();
	SysTickIntEnable();																		//Enable Systick interrupt  
	
	S800_GPIO_Init();
	S800_I2C0_Init();
	S800_UART_Init();
	PWM_Init();
	startup();
	
	IntEnable(INT_UART0);
  UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);	//Enable UART0 RX,TX interrupt
  IntMasterEnable();	

	ui32IntPriorityMask				= IntPriorityMaskGet();
	IntPriorityGroupingSet(3);														//Set all priority to pre-emtption priority
	
	IntPrioritySet(INT_UART0,3);													//Set INT_UART0 to highest priority
	IntPrioritySet(FAULT_SYSTICK,0x0e0);									//Set INT_SYSTICK to lowest priority
	
	ui32IntPriorityGroup			= IntPriorityGroupingGet();

	ui32IntPriorityUart0			= IntPriorityGet(INT_UART0);
	ui32IntPrioritySystick		= IntPriorityGet(FAULT_SYSTICK);

	while (1)
	{
		//读开关状态（按键去抖）
		kaiguan2=kaiguan;
		kaiguan = ~I2C0_ReadByte(TCA6424_I2CADDR, TCA6424_INPUT_PORT0);
		if(kaiguan2==0x0&&kaiguan==0x01) 
		{
			button1 =(button1+1)%5;
			button2=0;
			button3=0;
		}
		if(kaiguan2==0x0&&kaiguan==0x02)
		{
			button2=~button2;
			button3=0;
			button4=0;
			button5=0;
		}
		if(kaiguan2==0x0&&kaiguan==0x04)
		{
			button3=(button3+1)%6;
			button4=0;
		}
		if(kaiguan2==0x0&&kaiguan==0x08) button4=0x01;
		if(kaiguan2==0x0&&kaiguan==0x10) button5=0x01;
		if(kaiguan2==0x0&&kaiguan==0x20) button6=0x01;
		if(kaiguan2==0x0&&kaiguan==0x40) button7=~button7;
		if(kaiguan==0xC0) reset=true;
		if(reset) {reset=false ;SysCtlReset();}//reset
		//进位流程
		//ALARM的进位
		while(time[5]>=0x0A)
		{
			time[4]++;
			time[5]-=0x0A;
		}
		while(time[4]>=0x06)
		{
			time[3]++;
			time[4]-=0x06;
		}
		while(time[3]>=0x0A)
		{
			time[2]++;
			time[3]-=0x0A;
		}
		while(time[2]>=0x06)
		{
			time[1]++;
			time[2]-=0x06;
		}
		while(time[1] >=0x0A)
		{
			time[0]++;
			time[1]-=0x0A;
		}
		while(time[0] >=0x02)
		{
			time[0]-=0x02;
		}
		while(time[0] ==0x01&&time[1]>=0x03)
		{
			time[0]=0x0;
			time[1]-=0x02;
		}
		if(time[0]==1&&time[1]==2)
		{
			time[0]=0;
			time[1]=0;
		}
		//24小时钟表的进位
		while(time24[5]>=0x0A)
		{
			time24[4]++;
			time24[5]-=0x0A;
		}
		while(time24[4]>=0x06)
		{
			time24[3]++;
			time24[4]-=0x06;
		}
		while(time24[3]>=0x0A)
		{
			time24[2]++;
			time24[3]-=0x0A;
		}
		while(time24[2]>=0x06)
		{
			time24[1]++;
			time24[2]-=0x06;
		}
		while(time24[1] >=0x0A)
		{
			time24[0]++;
			time24[1]-=0x0A;
		}
		while(time24[0] >=0x03)
		{
			time24[0]-=0x03;
		}
		while(time24[0] ==0x02&&time24[1]>=0x05)
		{
			time[0]=0x0;
			time[1]-=0x04;
		}
		if(time24[0]==2&&time24[1]==4)
		{
			time24[0]=0;
			time24[1]=0;
			date[5]++;
		}
		//日期的进位
		while(date[5]>=0x0A)
		{
			date [4]++;
			date [5]-=0x0A;
		}
		while(date[4]>=0x04)
		{
			date [4]-=0x03;
		}
		if(date[4]==3&&date[5]==2)
		{
			date [0]=0;
			date [1]=0;
			date[3]++;
		}
		while(date [3]>=0x0A)
		{
			date [2]++;
			date [3]-=0x0A;
		}
		while(date [2]>=0x02)
		{
			date [2]-=0x02;
		}
		if(date[2]==1&&date[3]==3)
		{
			date[2]=0;
			date[3]=0;
			date[1]++;
		}
		while(date[1] >=0x0A)
		{
			date [0]++;
			date [1]-=0x0A;
		}
		while(date[0] >=0x0A)
		{
			date [1]-=0x0A;
		}
		
		//闹钟ddl的进位
		while(ddl [5]>=0x0A)
		{
			ddl [4]++;
			ddl [5]-=0x0A;
		}
		while(ddl [4]>=0x06)
		{
			ddl [3]++;
			ddl [4]-=0x06;
		}
		while(ddl [3]>=0x0A)
		{
			ddl [2]++;
			ddl [3]-=0x0A;
		}
		while(ddl [2]>=0x06)
		{
			ddl [1]++;
			ddl [2]-=0x06;
		}
		while(ddl [1] >=0x0A)
		{
			ddl [0]++;
			ddl [1]-=0x0A;
		}
		while(ddl[0] >=0x02)
		{
			ddl [0]-=0x02;
		}
		while(ddl[0] ==0x01&&ddl [1]>=0x03)
		{
			ddl [0]=0x0;
			ddl [1]-=0x02;
		}
		if(ddl [0]==1&&ddl [1]==2)
		{
			ddl [0]=0;
			ddl [1]=0;
		}
		//将时间日期等信息存储为字符串，为串口发送做准备
		uart_put_string12[0]=(char)(time[0]+0x30);
		uart_put_string12[1]=(char)(time[1]+0x30);
		uart_put_string12[3]=(char)(time[2]+0x30);
		uart_put_string12[4]=(char)(time[3]+0x30);
		uart_put_string12[6]=(char)(time[4]+0x30);
		uart_put_string12[7]=(char)(time[5]+0x30);		
		uart_put_string24[0]=(char)(time24[0]+0x30);
		uart_put_string24[1]=(char)(time24[1]+0x30);
		uart_put_string24[3]=(char)(time24[2]+0x30);
		uart_put_string24[4]=(char)(time24[3]+0x30);
		uart_put_string24[6]=(char)(time24[4]+0x30);
		uart_put_string24[7]=(char)(time24[5]+0x30);	
		uart_put_date[0]=(char)(date[0]+0x30);
		uart_put_date[1]=(char)(date[1]+0x30);
		uart_put_date[3]=(char)(date[2]+0x30);
		uart_put_date[4]=(char)(date[3]+0x30);
		uart_put_date[6]=(char)(date[4]+0x30);
		uart_put_date[7]=(char)(date[5]+0x30);
		uart_put_alarm[0]=(char)(ddl[0]+0x30);
		uart_put_alarm[1]=(char)(ddl[1]+0x30);
		uart_put_alarm[3]=(char)(ddl[2]+0x30);
		uart_put_alarm[4]=(char)(ddl[3]+0x30);
		uart_put_alarm[6]=(char)(ddl[4]+0x30);
		uart_put_alarm[7]=(char)(ddl[5]+0x30);
		//闹钟的响应
		if(time[0]==ddl[0]&&time[1]==ddl[1]&&time[2]==ddl[2]&&time[3]==ddl[3]&&time[4]==ddl[4]&&time[5]==ddl[5]) 
			ddlreach=true;
		if(time24[0]==ddl[0]&&time24[1]==ddl[1]&&time24[2]==ddl[2]&&time24[3]==ddl[3]&&time24[4]==ddl[4]&&time24[5]==ddl[5]) 
			ddlreach=true;//闹钟时间到了
		if(ddlreach) 
		{
			musicplay();
			UARTStringPut("\r\nAlarm  is ringing\r\n");
		} 
		if(button6==0x1) 
		{
		ddlreach =false ; 
		UARTStringPut("\r\nAlarm has been closed\r\n"); 
		button6=0x0;
		}//关闭闹钟
		//恢复出厂设置
		if(button5==0x1)
		{
			chongzhi();
		}
		if(button7!=0x0)
		{
			result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x0ff);				//turn off the LED1-8
		}//睡眠模式
		else
		{
			result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x00);		//开灯
		//模式选择
		//12小时ALARM
		if(button1==0) 
		{
			time12flash();
			if(button2==0x0)time12run();//暂停or运行
			if(button4==0x1)
			{
				time[5-button3]++;
				button4=0;
			}//时间设置
		}
		//24小时时钟
	  if(button1==1)
		{
			time24flash();
			if(button2==0x0)time24run();//暂停or运行
			if(button4==0x1)
			{
				time24[5-button3]++;
				button4=0;
			}//时间设置
		}
		//日期显示
		if(button1==2) 
		{
			dateflash();
			if(button4==0x1)
			{
				date[5-button3]++;
				button4=0;
			}
		}
		//倒计时
		if(button1==3) 
		{
			daojishuflash();
			if(button2==0x0)daojishurun();
			if(button4==0x1)
			{
				daotime[5-button3]++;
				daojishiend=0x0;
				button4=0;
			}
		}
		//闹钟设置
		if(button1==4)
		{
			ddlflash();
			if(button4==0x1)
			{
				ddl[5-button3]++;
				button4=0;
			}
		}
	  }
	}
}
//功能函数实现
//开机画面
void startup()
{
	uint8_t result;
	uint8_t xuehao[12]={5,2,0,0,2,1,9,1,0,5,8,7};
	uint8_t i=0x40;
		result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x00);		//开灯
	Delay (FASTFLASHTIME);
	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x0ff);				//turn off the LED1-8
	Delay (FASTFLASHTIME);//全灭保持
	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x00);		//开灯
	while(i)
	{
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[xuehao[cnt]]);						//显示学号 				
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,rightshift);					//write port 2
	Delay (10000);
	cnt++;
	rightshift =rightshift<<1;
	if(cnt==0x08)
	{
		Delay (500);
		i--;
		cnt=0x0;
		rightshift =0x1;
	}
}
	i=0x40;
	cnt=0x8;
	rightshift =0x1;
	while(i)
	{
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[xuehao[cnt]]);						//显示学号 				
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,rightshift);					//write port 2
	Delay (20000);
	cnt++;
	rightshift =rightshift<<1;
	if(cnt==0x0C)
	{
		Delay (500);
		i--;
		cnt=0x08;
		rightshift =0x1;
	}
}	
	i=0x40;
	cnt=0x0;
	rightshift =0x1;
	while(i)
	{
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7hello[cnt]);						//显示HELLO 				
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,rightshift);					//write port 2
	Delay (17000);
	cnt++;
	rightshift =rightshift<<1;
	if(cnt==0x05)
	{
		Delay (500);
		i--;
		cnt=0x0;
		rightshift =0x1;
	}
}
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x0);
	sound (262);//1
	sound (294);//2
	sound (330);//3
	sound (349);//4
	sound (392);//5
	sound (440);//6
	sound (494);//7
}
//重置，重启，关机
void chongzhi()//重置初始化
{
	time[0]=0;
	time[1]=0;
	time[2]=0;
	time[3]=0;
	time[4]=0;
	time[5]=0;
	time24[0]=1;
	time24[1]=2;
	time24[2]=0;
	time24[3]=0;
	time24[4]=0;
	time24[5]=0;
	date[0]=2;
	date[1]=2;
	date[2]=0;
	date[3]=6;
	date[4]=1;
	date[5]=2;
	daotime[0]=0;
	daotime[1]=0;
	daotime[2]=1;
	daotime[3]=5;
	daotime[4]=0;
	daotime[5]=0;
	ddl[0]=0;
	ddl[1]=0;
	ddl[2]=0;
	ddl[3]=1;
	ddl[4]=0;
	ddl[5]=0;
	button2=0;
}
//模式实现
//12小时ALARM的运行和数码管显示
void time12run()
{
	if (systick_1s_status)
	{
		systick_1s_status = 0;
		time[5] ++;			
	}	
}
void time12flash()
{	
		i=0x0;
		rightshift =0x01;
		while(i!=0x06)
		{
		result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[time[i]]);	//write port 1 				
		result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,rightshift);	//write port 2
		rightshift= rightshift<<1;
		i++;
		Delay (5000);
		}
}
//24小时时钟的运行和数码管显示
void time24run()
{
	if (systick_1s_status)
	{
		systick_1s_status = 0;
		time24[5] ++;			
	}		
}
void time24flash()
{

	i=0x0;
	rightshift =0x01;
	Delay (5000);
	while(i!=0x06)
	{
		result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[time24[i]]);	//write port 1 				
		result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,rightshift);	//write port 2
		rightshift= rightshift<<1;
		i++;
		Delay (5000);
	}
}
//日期的数码管显示
void dateflash()
{
	i=0x0;
	rightshift =0x01;
	while(i!=0x06)
	{
	result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[date[i]]);	//write port 1 				
	result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,rightshift);	//write port 2
	rightshift= rightshift<<1;
	i++;
	Delay (5000);
	}	
}
//闹钟显示
void ddlflash()
{
	i=0x0;
	rightshift =0x01;
	while(i!=0x06)
	{
	result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[ddl[i]]);	//write port 1 				
	result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,rightshift);	//write port 2
	rightshift= rightshift<<1;
	i++;
	Delay (5000);
	}	
}
////倒计时秒表的运行和数码管显示
void daojishurun(void)
{
	if (systick_10ms_status)
	{
		systick_10ms_status		= 0;
		if(daotime[5]!=0)daotime[5]--;
		else
		{
			if(daotime[4]!=0) 
			{
			daotime[4]--;
			daotime[5]=9;
			}
			else
			{
				if(daotime[3]!=0)
				{
					daotime[3]--;
					daotime[4]=9;
					daotime[5]=9;
				}
				else
				{
					if(daotime[2]!=0)
					{
						daotime[2]--;
						daotime[3]=9;
						daotime[4]=9;
						daotime[5]=9;
					}
					else
					{
						if(daotime[1]!=0)
						{
							daotime[1]--;
							daotime[2]=5;
							daotime[3]=9;
							daotime[4]=9;
							daotime[5]=9;
						}
						else
						{
							if(daotime[0]!=0)
							{
								daotime[0]--;
								daotime[1]=9;
								daotime[2]=5;
								daotime[3]=9;
								daotime[4]=9;
								daotime[5]=9;
							}
						}
					}
				}
			}
		}
	}
	if(daojishiend<=0x1){
	if(daotime[0]==0&&daotime[1]==0&&daotime[2]==0&&daotime[3]==0&&daotime[4]==0&&daotime[5]==0)
	{
		sound(1000);
		daojishiend++;
	}
	}	
}
void daojishuflash()
{
	i=0x0;
	rightshift =0x01;
	while(i!=0x06)
	{
		result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[daotime[i]]);	//write port 1 				
		result 							= I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,rightshift);	//write port 2
		rightshift= rightshift<<1;
		i++;
		Delay (5000);
	}
}
void Delay(uint32_t value)
{
	uint32_t ui32Loop;
	for(ui32Loop = 0; ui32Loop < value; ui32Loop++){};
}


//蜂鸣器部分
void PWM_Init(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK)){}
  //GPIOPinConfigure(GPIO_PF0_M0PWM0);
	//GPIOPinConfigure(GPIO_PF1_M0PWM1);
	//GPIOPinConfigure(GPIO_PF2_M0PWM2);
	//GPIOPinConfigure(GPIO_PF3_M0PWM3);
	//GPIOPinConfigure(GPIO_PG0_M0PWM4);
	//GPIOPinConfigure(GPIO_PG1_M0PWM5);
	//GPIOPinConfigure(GPIO_PK4_M0PWM6);
	GPIOPinConfigure(GPIO_PK5_M0PWM7);

	GPIOPinTypePWM(GPIO_PORTK_BASE, GPIO_PIN_5);
  PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN |
                    PWM_GEN_MODE_NO_SYNC);	
  PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, PWM_PERIOD);	
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7,
                     PWM_PERIOD );
	PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false );
	PWMGenEnable(PWM0_BASE, PWM_GEN_3);
}
void sound(uint16_t hz)
{
	uint32_t fre;
	fre=5000000/hz;
	
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, fre);	
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7,
                     PWM_PERIOD );
	PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_3);
	Delay(1000000);
	PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, false);
	Delay(1000000);
}
void musicplay()
{
	sound(tone[littlestar[play]]);
	play=(play+1)%28;
}
//UART部分
void UARTStringPut(const char *cMessage)
{
	while(*cMessage!='\0')
		UARTCharPut(UART0_BASE,*(cMessage++));
}
void UARTStringPutNonBlocking(const char *cMessage)
{
	while(*cMessage!='\0')
		UARTCharPutNonBlocking(UART0_BASE,*(cMessage++));
}
void UARTStringGetNonBlocking(char *msg)
{
	while(UARTCharsAvail(UART0_BASE)){
		*msg++=UARTCharGetNonBlocking(UART0_BASE);
	}
	*msg='\0';
}
void S800_UART_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);						//Enable PortA
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));			//Wait for the GPIO moduleA ready

	GPIOPinConfigure(GPIO_PA0_U0RX);												// Set GPIO A0 and A1 as UART pins.
  GPIOPinConfigure(GPIO_PA1_U0TX);    			

  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	// Configure the UART for 115,200, 8-N-1 operation.
  UARTConfigSetExpClk(UART0_BASE, ui32SysClock,115200,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));
	UARTStringPut((uint8_t *)"\r\nDear user,welcome to timer!\r\n");
}
//GPIO部分
void S800_GPIO_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);						//Enable PortF
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));			//Wait for the GPIO moduleF ready
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);						//Enable PortJ	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));			//Wait for the GPIO moduleJ ready	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);						//Enable PortN	
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));			//Wait for the GPIO moduleN ready		
	
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);			//Set PF0 as Output pin
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);			//Set PN0 as Output pin
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);		//Set PN1 as Output pin	

	GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE,GPIO_PIN_0 | GPIO_PIN_1);//Set the PJ0,PJ1 as input pin
	GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0 | GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
}
//I2C
void S800_I2C0_Init(void)
{
	uint8_t result;
	uint8_t xuehao[12]={5,2,0,0,2,1,9,1,0,5,8,7};
	uint8_t i=0x40;
  SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);
  GPIOPinConfigure(GPIO_PB3_I2C0SDA);
  GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
  GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

	I2CMasterInitExpClk(I2C0_BASE,ui32SysClock, true);										//config I2C0 400k
	I2CMasterEnable(I2C0_BASE);	

	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT0,0x0ff);		//config port 0 as input
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT1,0x0);			//config port 1 as output
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_CONFIG_PORT2,0x0);			//config port 2 as output 

	//result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT1,seg7[8]);						//显示8 				
	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x0);					//write port 2

	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_CONFIG,0x00);					//config port as output
	result = I2C0_WriteByte(PCA9557_I2CADDR,PCA9557_OUTPUT,0x0ff);				//turn off the LED1-8
	Delay (FASTFLASHTIME);//全灭保持

	result = I2C0_WriteByte(TCA6424_I2CADDR,TCA6424_OUTPUT_PORT2,0x0);
	
}
uint8_t I2C0_WriteByte(uint8_t DevAddr, uint8_t RegAddr, uint8_t WriteData)
{
	uint8_t rop;
	while(I2CMasterBusy(I2C0_BASE)){};
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	I2CMasterDataPut(I2C0_BASE, RegAddr);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	while(I2CMasterBusy(I2C0_BASE)){};
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);

	I2CMasterDataPut(I2C0_BASE, WriteData);
	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	while(I2CMasterBusy(I2C0_BASE)){};

	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	return rop;
}
uint8_t I2C0_ReadByte(uint8_t DevAddr, uint8_t RegAddr)
{
	uint8_t value,rop;
	while(I2CMasterBusy(I2C0_BASE)){};	
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, false);
	I2CMasterDataPut(I2C0_BASE, RegAddr);
//	I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);		
	I2CMasterControl(I2C0_BASE,I2C_MASTER_CMD_SINGLE_SEND);
	while(I2CMasterBusBusy(I2C0_BASE));
	rop = (uint8_t)I2CMasterErr(I2C0_BASE);
	Delay(1000);
	//receive data
	I2CMasterSlaveAddrSet(I2C0_BASE, DevAddr, true);
	I2CMasterControl(I2C0_BASE,I2C_MASTER_CMD_SINGLE_RECEIVE);
	while(I2CMasterBusBusy(I2C0_BASE));
	value=I2CMasterDataGet(I2C0_BASE);
		Delay(1000);
	return value;
}

/*
	Corresponding to the startup_TM4C129.s vector table systick interrupt program name
*/
//中断
void SysTick_Handler(void)
{
	if (systick_1s_couter	!= 0)
		systick_1s_couter--;
	else
	{
		systick_1s_couter	= SYSTICK_FREQUENCY;
		systick_1s_status 	= 1;
	}
	
	if (systick_100ms_couter	!= 0)
		systick_100ms_couter--;
	else
	{
		systick_100ms_couter	= SYSTICK_FREQUENCY/10;
		systick_100ms_status 	= 1;
	}
	
	if (systick_10ms_couter	!= 0)
		systick_10ms_couter--;
	else
	{
		systick_10ms_couter		= SYSTICK_FREQUENCY/100;
		systick_10ms_status 	= 1;
	}
	if (GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0) == 0)
	{
		systick_100ms_status	= systick_10ms_status = 0;
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0,GPIO_PIN_0);		
	}
	else
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0,0);		
}

/*
	Corresponding to the startup_TM4C129.s vector table UART0_Handler interrupt program name
*/
void UART0_Handler(void)
{
	bool flag=false;
	bool beiyong=true;
	bool question=false ;
	uint16_t length;
	int32_t uart0_int_status;
  uart0_int_status 		= UARTIntStatus(UART0_BASE, true);		// Get the interrrupt status.
  UARTIntClear(UART0_BASE, uart0_int_status);								//Clear the asserted interrupts
	
  count=0;
	while(UARTCharsAvail(UART0_BASE))    											// Loop while there are characters in the receive FIFO.不读空格
  {
		uart_receive_string[count]=UARTCharGetNonBlocking(UART0_BASE);
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,GPIO_PIN_1 );
		if (uart_receive_string[count]=='?'&&count==0){question =true;}
		if (uart_receive_string[count] <= 'z' && uart_receive_string[count] >= 'a')
				uart_receive_string[count] -= 'a' - 'A';		
		if(uart_receive_string[count]==' '){}
		else   count++;
	}
	Delay(2000);
	while(UARTCharsAvail(UART0_BASE))    											// Loop while there are characters in the receive FIFO.不读空格
  {
		uart_receive_string[count]=UARTCharGetNonBlocking(UART0_BASE);
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,GPIO_PIN_1 );
		if (uart_receive_string[count]=='?'&&count==0){question =true;}
		if (uart_receive_string[count] <= 'z' && uart_receive_string[count] >= 'a')
				uart_receive_string[count] -= 'a' - 'A';		
		if(uart_receive_string[count]==' '){}
		else   count++;
	}
	Delay(2000);
	while(UARTCharsAvail(UART0_BASE))    											// Loop while there are characters in the receive FIFO.不读空格
  {
		uart_receive_string[count]=UARTCharGetNonBlocking(UART0_BASE);
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,GPIO_PIN_1 );
		if (uart_receive_string[count]=='?'&&count==0){question =true;}
		if (uart_receive_string[count] <= 'z' && uart_receive_string[count] >= 'a')
				uart_receive_string[count] -= 'a' - 'A';		
		if(uart_receive_string[count]==' '){}
		else   count++;
	}
	Delay(2000);
	while(UARTCharsAvail(UART0_BASE))    											// Loop while there are characters in the receive FIFO.不读空格
  {
		uart_receive_string[count]=UARTCharGetNonBlocking(UART0_BASE);
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,GPIO_PIN_1 );
		if (uart_receive_string[count]=='?'&&count==0){question =true;}
		if (uart_receive_string[count] <= 'z' && uart_receive_string[count] >= 'a')
				uart_receive_string[count] -= 'a' - 'A';		
		if(uart_receive_string[count]==' '){}
		else   count++;
	}
	Delay(2000);
	while(UARTCharsAvail(UART0_BASE))    											// Loop while there are characters in the receive FIFO.
  {
		uart_receive_string[count]=UARTCharGetNonBlocking(UART0_BASE);
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,GPIO_PIN_1 );	
		if(uart_receive_string[count]==' '){}
		else   count++;
	}
	Delay(2000);
	while(UARTCharsAvail(UART0_BASE))    											// Loop while there are characters in the receive FIFO.
  {
		uart_receive_string[count]=UARTCharGetNonBlocking(UART0_BASE);
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,GPIO_PIN_1 );	
		if(uart_receive_string[count]==' '){}
		else   count++;
	}
	uart_receive_string[count]='\0';
	length =count;
	UARTStringPut(uart_receive_string);
	//开始比对读入的指令
	if(question)  //询问指令 
	{
		flag=true;
		UARTStringPut("you can put in:\nINITCLOCK\nCLOSEALARM\nSETTIME1XX:XX:XX\nSETTIME2XX:XX:XX\nSETDATEXX-XX-XX\nSETALARMTIMEXX:XX:XX\nGETTIME1\nGETTIME2\nGETDATE\nGETALARM\nRESET\r\n");
	}

	if(uart_receive_string[0]=='I'&&uart_receive_string[1]=='N'&&uart_receive_string[2]=='I'&&uart_receive_string[3]=='T'&&uart_receive_string[4]=='C'&&uart_receive_string[5]=='L'&&uart_receive_string[6]=='O'&&uart_receive_string[7]=='C'&&uart_receive_string[8]=='K'&&length==9)//INITCLOCK
	{
		flag=true;
		beiyong =false ;
		time24[0]=1;
		time24[1]=2;
		time24[2]=0;
		time24[3]=0;
		time24[4]=0;
		time24[5]=0;
		time[0]=0;
		time[1]=0;
		time[2]=0;
		time[3]=0;
		time[4]=0;
		time[5]=0;
		return ;
	}
	if(uart_receive_string[0]=='C'&&uart_receive_string[1]=='L'&&uart_receive_string[2]=='O'&&uart_receive_string[3]=='S'&&uart_receive_string[4]=='E'&&uart_receive_string[5]=='A'&&uart_receive_string[6]=='L'&&uart_receive_string[7]=='A'&&uart_receive_string[8]=='R'&&uart_receive_string[9]=='M'&&length==10)//CLOSEALARM
	{
		flag=true;
		beiyong=false;
		ddlreach =false;
		UARTStringPut("\r\nAlarm has been closed\r\n"); 
		return ;
	}
	if(uart_receive_string[0]=='S'&&uart_receive_string[1]=='E'&&uart_receive_string[2]=='T'&&uart_receive_string[3]=='T'&&uart_receive_string[4]=='I'&&uart_receive_string[5]=='M'&&uart_receive_string[6]=='E'&&length==16)//SETTIME1/2 00:56:03
	{
		if(uart_receive_string[7]=='1')
		{
		flag=true;
		time[0]=(uint8_t)uart_receive_string[8]-0x30;
		time[1]=(uint8_t)uart_receive_string[9]-0x30;
		time[2]=(uint8_t)uart_receive_string[11]-0x30;
		time[3]=(uint8_t)uart_receive_string[12]-0x30;
		time[4]=(uint8_t)uart_receive_string[14]-0x30;
		time[5]=(uint8_t)uart_receive_string[15]-0x30;	
		}
		if(uart_receive_string[7]=='2')
		{
		flag=true;
		time24[0]=(uint8_t)uart_receive_string[8]-0x30;
		time24[1]=(uint8_t)uart_receive_string[9]-0x30;
		time24[2]=(uint8_t)uart_receive_string[11]-0x30;
		time24[3]=(uint8_t)uart_receive_string[12]-0x30;
		time24[4]=(uint8_t)uart_receive_string[14]-0x30;
		time24[5]=(uint8_t)uart_receive_string[15]-0x30;
		}
	}	
	if(uart_receive_string[0]=='S'&&uart_receive_string[1]=='E'&&uart_receive_string[2]=='T'&&uart_receive_string[3]=='D'&&uart_receive_string[4]=='A'&&uart_receive_string[5]=='T'&&uart_receive_string[6]=='E'&&length==15)//SETDATE22-06-13
	{
		flag=true;
		date [0]=uart_receive_string[7]-0x30;
		date [1]=uart_receive_string[8]-0x30;
		date [2]=uart_receive_string[10]-0x30;
		date [3]=uart_receive_string[11]-0x30;
		date [4]=uart_receive_string[13]-0x30;
		date [5]=uart_receive_string[14]-0x30;
	}
	if(uart_receive_string[0]=='S'&&uart_receive_string[1]=='E'&&uart_receive_string[2]=='T'&&uart_receive_string[3]=='A'&&uart_receive_string[4]=='L'&&uart_receive_string[5]=='A'&&uart_receive_string[6]=='R'&&uart_receive_string[7]=='M'&&uart_receive_string[8]=='T'&&uart_receive_string[9]=='I'&&uart_receive_string[10]=='M'&&uart_receive_string[11]=='E'&&length==20)//SETALARMTIME00:56:03
	{
		flag=true;
		ddl[0]=uart_receive_string[12]-0x30;
		ddl[1]=uart_receive_string[13]-0x30;
		ddl[2]=uart_receive_string[15]-0x30;
		ddl[3]=uart_receive_string[16]-0x30;
		ddl[4]=uart_receive_string[18]-0x30;
		ddl[5]=uart_receive_string[19]-0x30;
	}
	if(uart_receive_string[0]=='G'&&uart_receive_string[1]=='E'&&uart_receive_string[2]=='T'&&uart_receive_string[3]=='T'&&uart_receive_string[4]=='I'&&uart_receive_string[5]=='M'&&uart_receive_string[6]=='E'&&length==8)//GETTIME1/2
	{
		if(uart_receive_string[7]=='1'){
		flag=true;
		UARTStringPut(uart_put_string12);
		}
		if(uart_receive_string[7]=='2'){
		flag=true;
		UARTStringPut(uart_put_string24);
		}
	}	
	if(uart_receive_string[0]=='G'&&uart_receive_string[1]=='E'&&uart_receive_string[2]=='T'&&uart_receive_string[3]=='D'&&uart_receive_string[4]=='A'&&uart_receive_string[5]=='T'&&uart_receive_string[6]=='E'&&length==7)//GETDATE
	{
		flag=true;
		UARTStringPut(uart_put_date);
	}
	if(uart_receive_string[0]=='G'&&uart_receive_string[1]=='E'&&uart_receive_string[2]=='T'&&uart_receive_string[3]=='A'&&uart_receive_string[4]=='L'&&uart_receive_string[5]=='A'&&uart_receive_string[6]=='R'&&uart_receive_string[7]=='M'&&length==8)//GETALARM
	{	
		flag=true;
		UARTStringPutNonBlocking(uart_put_alarm);
	}
	if(uart_receive_string[0]=='R'&&uart_receive_string[1]=='E'&&uart_receive_string[2]=='S'&&uart_receive_string[3]=='E'&&uart_receive_string[4]=='T'&&length==5)//RESET
	{
		flag=true;
		SysCtlReset();
	}
	if(beiyong )
	{
	if(!flag)//wrong
	{
		if(uart_receive_string[0]=='I') UARTStringPut("please put in \nINITCLOCK\r\n");
		if(uart_receive_string[0]=='C') UARTStringPut("please put in \nCLOSEALARM\r\n");
		if(uart_receive_string[0]=='S') UARTStringPut("please put in \nSETTIME1XX:XX:XX \nSETTIME2XX:XX:XX \nSETDATEXX-XX-XX \nSETALARMTIMEXX:XX:XX\r\n");
		if(uart_receive_string[0]=='G') UARTStringPut("please put in \nGETTIME1 \nGETTIME2 \nGETDATE \nGETALARM\r\n");
		if(uart_receive_string[0]=='R') UARTStringPut("please put in \nRESET\r\n");
		if(uart_receive_string[0]==' '){}
		if(uart_receive_string[0]!='I'&&uart_receive_string[0]!='S'&&uart_receive_string[0]!='G'&&uart_receive_string[0]!='R'&&uart_receive_string[0]!=' ')
			UARTStringPut("please put in\nINITCLOCK\nCLOSEALARM\nSETTIME1XX:XX:XX\nSETTIME2XX:XX:XX\nSETDATEXX-XX-XX\nSETALARMTIMEXX:XX:XX\nGETTIME1\nGETTIME2\nGETDATE\nGETALARM\nRESET");
	}	
	}

}
