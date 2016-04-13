/*
   #更新说明 v1.2.4
   1.AT指令解析函数需接收参数,参数为要解析的指定命令,参数为空字符串的时候获取缓存栈中的数据顺序执行解析;
   2.新增格式化信息输出函数;
   3.新增宏定义"软件串口终止符"
   4.接收软串口数据函数(ReceiveSoftwareSerialData)类型改为String.接收参数"延迟时间"和"是否显示".

  #功能介绍
  1.支持AT指令(可自定义指令);
  2.可通过AT指令查改配置信息;
  3.配置信息保存于EEPROM中,断电后不会丢失;
  4.功能函数支持定时执行/定时多线程执行(模拟多线程);
  5.可自定义设置时间片的间隔时长;
  6.当时间片间隔过长时,多条AT指令缓存入栈中等待执行,栈长度可通过宏定义"_ATStackSize"设置;
  7.支持软串口,可使用AT指令通过硬件串口向软串口发送数据,使用软串口数据接收函数可将数据转发回硬件串口;

  #备注
  1.使用多线程函数会占用较多的内存,多线程函数在其工作时段中只能被调用一次,多次调用会缩短下次执行的时间;
  2."多线程函数"和"通用定时函数"的示例函数各有两个;
  3.结构体StructParam成员变量发生改变后需通过AT指令"AT+INIT"初始化后才能正常使用;
  4.模板具备相对较完整的功能,实际使用可根据情况删减不必要的库/代码以消减内存开销;

  #作者信息
  ID:Landriesnidis
  E-mail:Landriesnidis@yeah.net
  Github:https://github.com/landriesnidis
*/
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <pt.h>

/* - - - - - - - - -模板宏定义 - - - - - - - - - - */
#define _Terminator                   '\n'                      //硬件串口终止符
#define _TerminatorSS                 "\r\n"                    //软件串口终止符(连接的模块所使用的终止符)
#define _Separator                    '='                       //AT指令参数分隔符
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
  const char _Version[30] =           "Multitasking_template 1.2.4";//版本信息
  const char _Date[11] =              "2016-04-13";                 //烧录时间
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
  ReceiveSoftwareSerialData(0, true);

  if (CommandQueue_num > 0)
    AT_Commands(getSerialData());

  //自定义函数执行列表
  if (!_BoolAT) {
    //    MultiThread_Function1(1000, &_Thread1);
    //    MultiThread_Function2(5000, &_Thread2);
    //    Timing_Function1(500);
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
   发送Socket数据
*/
void sendData_Socket(String text) {
  String cmd = "AT+CIPSEND=" + text.length();
  printline_s(cmd);
  delay(100);
  printline_s(text);
}

/**
   连接wifi
*/
void connectToWifi(String ssid, String psw) {
  String cmd = ("AT+CWJAP=\"" + ssid + "\",\"" + psw + "\"");
  printline_s(cmd);
}

/**
   连接服务器
*/
void connectToServer(String type, String ip, int port) {
  String cmd = ("AT+CIPSTART=\"" + type + "\",\"" + ip + "\"," + port);
  printline_s(cmd);
}

/**
   关闭连接
*/
void closeConnection() {
  printline_s("AT+CIPCLOSE");
}

/**
   输出一行字符串到硬件串口
*/
void printline_h(String text) {
  Serial.print(text);
  Serial.print(_Terminator);
}

/**
   输出一行字符串到软件串口
*/
void printline_s(String text) {
  SSerial.print(text);
  SSerial.print(_TerminatorSS);
}

/**
   AT指令集
*/
void AT_Commands(String command) {
  //解析AT指令
  String item = "", value = "";
  int index = -1;
  if (command.equals("")) {
    command = getSerialData();
    if (command.equals("")) return;
  } else {
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
    if (!value.equals("")) {
      SSerial.print(ReceiveLine(""));
      SSerial.print("\r\n");
      Serial.print("OK");
      Serial.print(_Terminator);
    } else {
      Serial.print("ERROR");
      Serial.print(_Terminator);
    }
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
String ReceiveSoftwareSerialData(long Timer, boolean isShow) {
  String temp_s = "";
  char temp_c;

  if (Timer > 0)
    delay(Timer);
  if (SSerial.available() > 0) {
    while (SSerial.available() > 0)
    {
      temp_c = SSerial.read();
      temp_s += temp_c;
      delay(2);
    }
    if (isShow)showInfo("SSerial", temp_s);
  }
  return temp_s;
}

/**
   格式化输出文本信息
*/
void showInfo(String title, String text) {
  text.replace("\n\n", "\n");
  text.replace("\n", "\n\t|");
  Serial.print("[" + title + "-BEGIN]\n\t|");
  Serial.print(text);
  Serial.print(_Terminator);
  Serial.print("[" + title + "-END]");
  Serial.print(_Terminator);
}

/**
   从串口接受一行文本
*/
String ReceiveLine(String hint) {
  String temp_s = "";
  char temp_c;
  Serial.print(hint + ">");
  while (1)
  {
    if (Serial.available() > 0) {
      temp_c = char(Serial.read());                                                //单字节读取串口数据
      if (temp_c != _Terminator) {                                                  //判断是否为终止符
        temp_s += temp_c;
      } else {
        break;
      }
      delay(2);
    }
  }
  Serial.print(temp_s);
  Serial.print(_Terminator);
  return temp_s;
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

