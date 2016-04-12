/*
  #更新说明 v1.0.1
  1.完善注释,修正符号.
  
  #功能介绍
  1.将模块连接至软串口,数据通过Arduino在软件串口和硬件串口之间转发.

  #备注
  1.起初使用Arduino模拟TTL的目的是为了ESP8266在计算机USB接口由于供电不足不能正常工作的问题.
  2.与ESP8266连接后建议将软硬串口的比特率都设置为9600(ESP8266修改波特率指令:AT+UART=9600,8,1,0,0).
  3.该代码分离自Multitasking_template_v1.2.3(模拟多线程的开源程序模板);

  #作者信息
  ID:Landriesnidis
  E-mail:Landriesnidis@yeah.net
  Github:https://github.com/landriesnidis
*/
#include <SoftwareSerial.h>

#define _HBAUD          9600                              //硬件串口波特率
#define _SBAUD          9600                              //软件串口波特率
#define _Terminator     '\n'                              //硬件串口终止符
#define _TerminatorSS   "\r\n"                            //软件串口终止符(连接的模块所使用的终止符)

//软串口设置
SoftwareSerial SSerial(2, 3); // RX, TX

void setup() {
  //串口波特率初始化
  Serial.begin(_HBAUD);        //设置串口波特率
  SSerial.begin(_SBAUD);        //设置软串口波特率
  delay(50);
  Serial.println("START");
}

void loop() {
  ReceiveSerialData();
  ReceiveSoftwareSerialData();
}


/**
   接收串口通信数据
*/
void ReceiveSerialData() {
  String temp_s = "";
  char temp_c;

  if (Serial.available() <= 0)
    return;
  while (Serial.available() > 0)
  {
    temp_c = char(Serial.read());                                                //单字节读取串口数据
    if (temp_c == _Terminator) {                                                  //判断是否为终止符
      break;
    } else {
      temp_s += temp_c;
    }
    delay(2);
  }
  SSerial.print(temp_s);
  SSerial.print(_TerminatorSS);
}

/**
   接收软串口通信数据
*/
void ReceiveSoftwareSerialData() {
  String temp_s = "";
  char temp_c;

  if (SSerial.available() > 0) {
    Serial.print("[SSerial-BEGIN]\n\t|");
    while (SSerial.available() > 0)
    {
      temp_c = SSerial.read();
      temp_s += temp_c;
      delay(2);
    }
    temp_s.replace("\n\n", "\n");
    temp_s.replace("\n", "\n\t|");
    Serial.print(temp_s);
    Serial.print(_Terminator);
    Serial.print("[SSerial-END]");
    Serial.print(_Terminator);
  }
}
