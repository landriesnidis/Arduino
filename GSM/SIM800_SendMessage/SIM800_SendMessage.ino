//***************************************************************************
//----------------本例程仅供学习使用，未经作者允许，不得用于其他用途。-----------
//------------------------版权所有，仿冒必究！---------------------------------
//----------------1.开发环境:Arduino IDE or Visual Studio 2010----------------
//----------------2.使用开发板型号：Arduino UNO R3            ----------------
//----------------3.单片机使用晶振：16M		                 ----------------
//----------------4.淘宝网址：Ilovemcu.taobao.com		     ----------------
//----------------			52dpj.taobao.com                 ----------------
//—————————         epic-mcu.taobao.com              ----------------
//----------------5.作者：神秘藏宝室			                 ----------------
//***************************************************************************/
#include <Arduino.h>

#define KEY1 2

void setup()
{
	pinMode(KEY1, INPUT); 

	Serial.begin(9600);	//设置通讯的波特率为9600
}

void loop()
{
	Scan_KEY();			
}

void SendMessage()
{
	Serial.print("AT+CSCS=");	
	Serial.print('"');	
	Serial.print("GSM");
	Serial.print('"');
	Serial.print("\r\n");

	delay(1000);
	Serial.print("AT+CMGF=1\r\n");
	delay(1000);
			
	Serial.print("AT+CMGS=");	
	Serial.print('"');	
	Serial.print("18067933376");			//发送目标手机号在这里设置
	Serial.print('"');
	Serial.print("\r\n");
	delay(1000);
	Serial.print("Hello World!");			//短信内容
	delay(1000);
	Serial.write(0x1A);			//发送操作
	delay(1000);
}



void Scan_KEY()						//按键扫描
{
	if( digitalRead(KEY1) == 0 )		//查看按键是否按下	
	{
		delay(20);					//延时20ms，去抖动	
		if( digitalRead(KEY1) == 0 ) //查看按键是否按下
		{			
			while(digitalRead(KEY1) == 0);		//松手检测
			SendMessage();
		}
	}
}

