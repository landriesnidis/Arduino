/*
   #更新说明 v1.2.2
   1.新增软串口数据接收功能;
   2.数据转发在硬件串口中显示;

  #功能介绍
  1.支持AT指令(可自定义指令);
  2.可通过AT指令查改配置信息;
  3.配置信息保存于EEPROM中,断电后不会丢失;
  4.功能函数支持定时执行/定时多线程执行;
  5.可自定义设置时间片的间隔时长;
  6.当时间片间隔过长时,多条AT指令缓存入栈中等待执行,栈长度可通过宏定义设置;
  7.支持软串口通信,AT指令向软串口转发数据;

  #备注
  1.使用多线程函数会占用较多的内存;
  2."多线程函数"和"通用定时函数"的示例函数各有两个;
  3.结构体StructParam成员变量发生改变后需通过AT指令"AT+INIT"初始化后才能正常使用;

  #作者信息
  ID:Landriesnidis
  E-mail:Landriesnidis@yeah.net
  Github:https://github.com/landriesnidis
*/
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <pt.h>

/* - - - - - - - - -模板宏定义 - - - - - - - - - - */
#define _Terminator                   '\n'                      //终止符
#define _Separator                    '='                       //分隔符
#define _ATStackSize                  10                        //AT指令缓存栈长度
/* - - - - - - - - 自定义宏定义 - - - - - - - - - -*/


/* - - - - - - - - - - - - - - - - - - - - - - - - */

//软串口设置
SoftwareSerial SSerial(2, 3); // RX, TX

//多线程
static struct pt _Thread1, _Thread2;

//控制开关
boolean _BoolAT = false;

//设置命令队列
String CommandQueue[_ATStackSize];
unsigned int CommandQueue_num = 0;

/**
   EEPROM-参数表
*/
struct StructParam {
  unsigned long _LoopCycle =          50;                           //每0.05秒循环
  unsigned int _SerialBufferSize =    1024;                         //串口缓存字符长度
  unsigned long _BaudRate =           9600;                         //波特率
  unsigned long _SSBaudRate =         9600;                         //波特率
  const char _Version[30] =           "Multitasking_template 1.2.2";//版本信息
  const char _Date[11] =              "2016-04-11";                 //烧录时间
  /* - - - - - - - - 自定义成员变量 - - - - - - - - - -*/


  /* - - - - - - - - - - - - - - - - - - - - - - - - - */
} ParameterList;

/**
   存储参数表
*/
void saveParameterBean() {
  EEPROM.put(sizeof(boolean), ParameterList);
  delay(100);
}

/**
   软复位
*/
void( *resetFunc) (void) = 0;

/**
   初始化
*/
void setup() {

  //读取EEPROM中的参数
  boolean sign;
  EEPROM.get(0, sign);
  if (!sign) {
    sign = true;
    EEPROM.put(0, sign);
    saveParameterBean();
  } else {
    EEPROM.get(sizeof(boolean), ParameterList);
  }

  //串口波特率初始化
  delay(50);
  Serial.begin(ParameterList._BaudRate);        //设置串口波特率
  SSerial.begin(ParameterList._SSBaudRate);      //设置软串口波特率

  //线程初始化
  delay(50);
  PT_INIT(&_Thread1);
  PT_INIT(&_Thread2);

  //START
  delay(50);
  Serial.print("START");
  Serial.print(_Terminator);

}

void loop() {

  ReceiveSerialData();
  ReceiveSoftwareSerialData();

  if (CommandQueue_num > 0)
    AT_Commands();

  //自定义函数执行列表
  if (!_BoolAT) {
    //    MultiThread_Function1(1000, &_Thread1);
    //    MultiThread_Function2(5000, &_Thread2);
    //    Timing_Function1(100);
    //    Timing_Function2(1000);
  }
  //时间片
  delay(ParameterList._LoopCycle);
}

/**
   多线程函数示例
*/
static int MultiThread_Function1(const unsigned long Interval, struct pt *pt) {
  /*— — — — — — — — — ↓ 时间控制 请勿改动 ↓ — — — — — — — — — — */
  static unsigned int CODE_INDEX = 0;
  static unsigned int WAIT_TIMER = 0;
  static unsigned int freq = 0;
  if (WAIT_TIMER == 0)
    if (++freq * ParameterList._LoopCycle < Interval) {
      return 0;
    } else {
      freq = 0;
      CODE_INDEX = 0;
    }
  else {
    WAIT_TIMER -= ParameterList._LoopCycle;
    if (WAIT_TIMER == 0)
      CODE_INDEX++;
  }
  /*— — — — — — — — — ↑ 时间控制 请勿改动 ↑ — — — — — — — — — — */

  /*代码区域:开始*/PT_BEGIN(pt);
  /*— — — — — — — — — — — — — — — — — — — — — — — — — — — */
  Serial.println("MultiThread_Function1:1 - 0.");
  WAIT_TIMER = 1000; PT_WAIT_UNTIL(pt, CODE_INDEX == 1);
  Serial.println("MultiThread_Function1:1 - 1.");
  WAIT_TIMER = 1000; PT_WAIT_UNTIL(pt, CODE_INDEX == 2);
  Serial.println("MultiThread_Function1:1 - 2.");
  WAIT_TIMER = 1000; PT_WAIT_UNTIL(pt, CODE_INDEX == 3);
  Serial.println("MultiThread_Function1:1 - 3.");
  WAIT_TIMER = 1000; PT_WAIT_UNTIL(pt, CODE_INDEX == 4);
  Serial.println("MultiThread_Function1:1 - 4.");
  //  Serial.print(_Terminator);
  /*— — — — — — — — — — — — — — — — — — — — — — — — — — — */
  /*代码区域:结束*/PT_END(pt);
}

/**
   多线程函数示例
*/
static int MultiThread_Function2(const unsigned long Interval, struct pt *pt) {
  /*— — — — — — — — — ↓ 时间控制 请勿改动 ↓ — — — — — — — — — — */
  static unsigned int CODE_INDEX = 0;
  static unsigned int WAIT_TIMER = 0;
  static unsigned int freq = 0;
  if (WAIT_TIMER == 0)
    if (++freq * ParameterList._LoopCycle < Interval) {
      return 0;
    } else {
      freq = 0;
      CODE_INDEX = 0;
    }
  else {
    WAIT_TIMER -= ParameterList._LoopCycle;
    if (WAIT_TIMER == 0)
      CODE_INDEX++;
  }
  /*— — — — — — — — — ↑ 时间控制 请勿改动 ↑ — — — — — — — — — — */

  /*代码区域:开始*/PT_BEGIN(pt);
  /*— — — — — — — — — — — — — — — — — — — — — — — — — — — */
  Serial.println("MultiThread_Function2:2 - 0.");
  WAIT_TIMER = 1000; PT_WAIT_UNTIL(pt, CODE_INDEX == 1);
  Serial.println("MultiThread_Function2:2 - 1.");
  WAIT_TIMER = 1000; PT_WAIT_UNTIL(pt, CODE_INDEX == 2);
  Serial.println("MultiThread_Function2:2 - 2.");
  WAIT_TIMER = 1000; PT_WAIT_UNTIL(pt, CODE_INDEX == 3);
  Serial.println("MultiThread_Function2:2 - 3.");
  WAIT_TIMER = 1000; PT_WAIT_UNTIL(pt, CODE_INDEX == 4);
  Serial.println("MultiThread_Function2:2 - 4.");
  //  Serial.print(_Terminator);
  /*— — — — — — — — — — — — — — — — — — — — — — — — — — — */
  /*代码区域:结束*/PT_END(pt);
}

/**
   通用定时函数示例
*/
void Timing_Function1(const unsigned long Interval) {
  /*— — — — — — — — — ↓ 时间控制 请勿改动 ↓ — — — — — — — — — — */
  static unsigned int freq = 0;
  if (++freq * ParameterList._LoopCycle < Interval) {
    return;
  } else {
    freq = 0;
  }
  /*— — — — — — — — — ↑ 时间控制 请勿改动 ↑ — — — — — — — — — — */

  /*代码区域:开始*/
  Serial.print("Timing_Function1:Done.");
  Serial.print(_Terminator);
  /*代码区域:结束*/
}

/**
   通用定时函数示例
*/
void Timing_Function2(const unsigned long Interval) {
  /*— — — — — — — — — ↓ 时间控制 请勿改动 ↓ — — — — — — — — — — */
  static unsigned int freq = 0;
  if (++freq * ParameterList._LoopCycle < Interval) {
    return;
  } else {
    freq = 0;
  }
  /*— — — — — — — — — ↑ 时间控制 请勿改动 ↑ — — — — — — — — — — */

  //代码区域:开始
  Serial.print("Timing_Function2:Done.");
  Serial.print(_Terminator);
  //代码区域:结束
}



/**
   AT指令集
*/
void AT_Commands() {
  //解析AT指令
  String command = getSerialData();
  String item = "", value = "";
  int index = -1;
  while (!command.equals("")) {
    index = command.indexOf(_Separator);
    if (index == -1) {
      item = command;
      value = "";
    }
    item = command.substring(0, index);
    value = command.substring(index + 1);
    command = getSerialData();
  }

  //执行AT指令
  if (item.equals("AT")) {                                //AT测试
    Serial.print("OK");
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+STOP")) {                          //暂停工作
    _BoolAT = true;
    Serial.print("OK");
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+WORK")) {                          //工作模式
    _BoolAT = false;
    Serial.print("OK");
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+CLEAN")) {                        //清空缓存
    CommandQueue_num = 0;
    Serial.print("OK");
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+TSP")) {                          //转发至虚拟串口
    SSerial.print(value);
    SSerial.print("\r\n");
    Serial.print("OK");
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+BAUD")) {                         //设置波特率
    if (!value.equals("")) {
      switch (value.toInt()) {
        case 0: ParameterList._BaudRate = 300;
          break;
        case 1: ParameterList._BaudRate = 1200;
          break;
        case 2: ParameterList._BaudRate = 2400;
          break;
        case 3: ParameterList._BaudRate = 4800;
          break;
        case 4: ParameterList._BaudRate = 9600;
          break;
        case 5: ParameterList._BaudRate = 19200;
          break;
        case 6: ParameterList._BaudRate = 38400;
          break;
        case 7: ParameterList._BaudRate = 57600;
          break;
        case 8: ParameterList._BaudRate = 74880;
          break;
        case 9: ParameterList._BaudRate = 115200;
          break;
        default:
          Serial.print("ERROR");
          Serial.print(_Terminator);
          return;
      }
      saveParameterBean();
      Serial.print("OK");
    } else {
      Serial.print(ParameterList._BaudRate);
    }
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+SSBAUD")) {                         //设置软串口波特率
    if (!value.equals("")) {
      switch (value.toInt()) {
        case 0: ParameterList._SSBaudRate = 300;
          break;
        case 1: ParameterList._SSBaudRate = 1200;
          break;
        case 2: ParameterList._SSBaudRate = 2400;
          break;
        case 3: ParameterList._SSBaudRate = 4800;
          break;
        case 4: ParameterList._SSBaudRate = 9600;
          break;
        case 5: ParameterList._SSBaudRate = 19200;
          break;
        case 6: ParameterList._SSBaudRate = 38400;
          break;
        case 7: ParameterList._SSBaudRate = 57600;
          break;
        case 8: ParameterList._SSBaudRate = 74880;
          break;
        case 9: ParameterList._SSBaudRate = 115200;
          break;
        default:
          Serial.print("ERROR");
          Serial.print(_Terminator);
          return;
      }
      saveParameterBean();
      Serial.print("OK");
    } else {
      Serial.print(ParameterList._BaudRate);
    }
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+VERSION")) {                      //查看版本
    Serial.print(ParameterList._Version);
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+DATE")) {                         //查看烧录时间
    Serial.print(ParameterList._Date);
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+CLEAN")) {                        //清空缓存
    CommandQueue_num = 0;
    Serial.print("OK");
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+SBS")) {                          //设置串口缓存字符长度
    if (!value.equals("")) {
      ParameterList._SerialBufferSize = value.toInt();
      saveParameterBean();
      Serial.print("OK");
    } else {
      Serial.print(ParameterList._SerialBufferSize);
    }
    Serial.print(_Terminator);
    return;
  }
  if (item.equals("AT+RESET")) {                        //软复位
    Serial.print("OK");
    Serial.print(_Terminator);
    delay(100);
    resetFunc();
    return;
  }
  if (item.equals("AT+INIT")) {                         //恢复初始化
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
    boolean sign = false;
    EEPROM.get(0, sign);
    delay(100);
    saveParameterBean();
    resetFunc();
    return;
  }
  if (item.equals("OK")) {                              //"OK"
    return;
  }

  /* - - - - - - - - - 自定义AT指令 - - - - - - - - - -*/


  /* - - - - - - - - - - - - - - - - - - - - - - - - - */

  //未被识别的命令
  Serial.print("ERROR");
  Serial.print(_Terminator);
}

/**
   接收串口通信数据
*/
void ReceiveSerialData() {
  static String temp_s = "";
  char temp_c;

  if (!Serial)
    return;
  while (Serial.available() > 0)
  {
    temp_c = char(Serial.read());                                                //单字节读取串口数据
    if (temp_c == _Terminator) {                                                  //判断是否为终止符
      if (addSerialData(temp_s)) {                                                //判断缓存区未占满
        temp_s = "";
      } else {
        return;
      }
    } else {
      temp_s += temp_c;
    }
    delay(2);
  }
  if (temp_s.length() > ParameterList._SerialBufferSize) {
    temp_s = "";
  }
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

/**
   添加数据进入栈内
*/
boolean addSerialData(String comdata) {
  if (CommandQueue_num >= _ATStackSize) {
    return false;
  } else {
    CommandQueue_num++;
    CommandQueue[CommandQueue_num - 1] = comdata;
    return true;
  }
}

/**
   取出栈顶的数据
*/
String getSerialData() {
  if (CommandQueue_num <= 0) {
    return "";
  } else {
    CommandQueue_num--;
    return CommandQueue[CommandQueue_num];
  }
}

