/*
  #更新说明 v1.1.0
  1.优化了读取Socket传来数据的代码,与关键词检测函数合并;
  2.新增"printline"函数,根据布参数判断数据输出流向;
  3.重写了AT指令集函数,接收数据流向参数;
  4.可通过Socket远程执行AT指令并返回结果(远程命令不缓存至栈内);

  #功能介绍
  1.支持AT指令(可自定义指令);
  2.可通过AT指令查改配置信息;
  3.配置信息保存于EEPROM中,断电后不会丢失;
  4.可自定义设置时间片的间隔时长;
  5.当时间片间隔过长时,多条AT指令缓存入栈中等待执行,栈长度可通过宏定义"_ATStackSize"设置;
  6.支持软串口,可使用AT指令通过硬件串口向软串口发送数据,使用软串口数据接收函数可将数据转发回硬件串口;

  #备注
  1.该程序使用模板:Multitasking_template_v1.2.5(模拟多线程的开源模块程序模板),删减了原模板中模拟多线程的功能;

  #作者信息
  ID:Landriesnidis
  E-mail:Landriesnidis@yeah.net
  Github:https://github.com/landriesnidis
*/
#include <EEPROM.h>
#include <SoftwareSerial.h>

/* - - - - - - - - -模板宏定义 - - - - - - - - - - */
#define _Terminator                   '\n'                      //硬件串口终止符
#define _TerminatorSS                 "\r\n"                    //软件串口终止符(连接的模块所使用的终止符)
#define _Separator                    '='                       //AT指令参数分隔符
#define _ATStackSize                  10                        //AT指令缓存栈长度

#define OUTPUTMODE_HARD                0                        //输出模式:硬件串口
#define OUTPUTMODE_SOFT                1                        //输出模式:软件串口
#define OUTPUTMODE_SOCKET              2                        //输出模式:Socket通信

/* - - - - - - - - 自定义宏定义 - - - - - - - - - -*/


/* - - - - - - - - - - - - - - - - - - - - - - - - */

//软串口设置
SoftwareSerial SSerial(2, 3); // RX, TX

//控制开关
boolean _BoolAT = false;

//设置命令队列
String CommandQueue[_ATStackSize];
unsigned int CommandQueue_num = 0;

/**
   EEPROM-参数表
*/
struct StructParam {
  unsigned long _LoopCycle =          50;                               //每0.05秒循环
  unsigned int _SerialBufferSize =    1024;                             //串口缓存字符长度
  unsigned long _BaudRate =           9600;                             //波特率
  unsigned long _SSBaudRate =         9600;                             //波特率
  boolean _SS_ECHO =                  false;                            //软串口数据回显
  const char _Version[30] =           "EndDevice_Esp8266 1.1.0";        //版本信息
  const char _Date[11] =              "2016-05-05";                     //烧录时间
  /* - - - - - - - - 自定义成员变量 - - - - - - - - - -*/
  char _WIFI_SSID[30] =               "NSI-ITA";                        //WIFI名称
  char _WIFI_PSWD[30] =               "liguijie";                       //WIFI密码
  bool _SERVER_TYPE   =               0;                                //服务器连接类型 0:TCP 1:UDP
  char _SERVER_IP[30] =               "10.70.21.41";                    //服务器地址
  int  _SERVER_PORT   =               12345;                            //服务器端口
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

  //START
  delay(50);
  printline_h("START");


}

void loop() {

  ReceiveSerialData();
  ReceiveSoftwareSerialData(0, ParameterList._SS_ECHO);

  if (CommandQueue_num > 0)
    AT_Analyze(getSerialData());

  //自定义函数执行列表
  if (!_BoolAT) {
        Timing_Function1(1000);
    //    Timing_Function2(1000);
  }
  //时间片
  delay(ParameterList._LoopCycle);
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
  Serial.println("Timing_Function1:Done.");

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

  //代码区域:结束
}

/**
   AT指令解析
*/
void AT_Analyze(String command) {
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
      } else {
        item = command.substring(0, index);
        value = command.substring(index + 1);
      }
      AT_Commands(item, value, OUTPUTMODE_HARD);
      command = getSerialData();
    }
  }
}

/**
   AT指令集
*/
void AT_Commands(String item, String value, int PRINT_MODE) {
  //执行AT指令
  if (item.equals("AT")) {                                //AT测试
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+STOP")) {                          //暂停工作
    _BoolAT = true;
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+WORK")) {                          //工作模式
    _BoolAT = false;
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+CLEAN")) {                        //清空缓存
    CommandQueue_num = 0;
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+TSP")) {                          //转发至虚拟串口
    if (value.equals("")) {
      SSerial.print(ReceiveLine(""));
      SSerial.print("\r\n");
    } else {
      SSerial.print(value);
      SSerial.print("\r\n");
    }
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+ECHO")) {                          //设置虚拟串口数据回显
    if (value.equals("TRUE") || value.equals("1")) {
      ParameterList._SS_ECHO = true;
      printline("OK", PRINT_MODE);
    } else if (value.equals("FALSE") || value.equals("0")) {
      ParameterList._SS_ECHO = false;
      printline("OK", PRINT_MODE);
    } else {
      printline("ERROR", PRINT_MODE);
    }
    saveParameterBean();
    return;
  }

  if (item.equals("AT+BAUD")) {                         //设置波特率
    if (!value.equals("")) {
      unsigned long baud[10] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200};
      int i;
      for (i = 0; i < 10; i++) {
        if (value.toInt() == baud[i])break;
      }
      if (i >= 10) {
        printline("ERROR", PRINT_MODE);
        return;
      }
      ParameterList._BaudRate = value.toInt();
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._BaudRate), PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+SSBAUD")) {                         //设置软串口波特率
    if (!value.equals("")) {
      unsigned long baud[10] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200};
      int i;
      for (i = 0; i < 10; i++) {
        if (value.toInt() == baud[i])break;
      }
      if (i >= 10) {
        printline("ERROR", PRINT_MODE);
        return;
      }
      ParameterList._SSBaudRate = value.toInt();
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._SSBaudRate), PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+VERSION")) {                      //查看版本
    printline(ParameterList._Version, PRINT_MODE);
    return;
  }
  if (item.equals("AT+DATE")) {                         //查看烧录时间
    printline(ParameterList._Date, PRINT_MODE);
    return;
  }
  if (item.equals("AT+CLEAN")) {                        //清空缓存
    CommandQueue_num = 0;
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+LC")) {                          //设置循环间隔时间
    if (!value.equals("")) {
      ParameterList._LoopCycle = value.toInt();
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._LoopCycle), PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+SBS")) {                          //设置串口缓存字符长度
    if (!value.equals("")) {
      ParameterList._SerialBufferSize = value.toInt();
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._SerialBufferSize), PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+RESET")) {                        //软复位
    printline("OK", PRINT_MODE);
    delay(50);
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
  int i;
  if (item.equals("AT+WIFISSID")) {                          //设置WIFI SSID
    if (!value.equals("")) {
      for (i = 0; i < value.length() && i < 30; i++)
        ParameterList._WIFI_SSID[i] = value[i];
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._WIFI_SSID), PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+WIFIPSWD")) {                          //设置WIFI 密码
    if (!value.equals("")) {
      for (i = 0; i < value.length() && i < 30; i++)
        ParameterList._WIFI_PSWD[i] = value[i];
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._WIFI_PSWD), PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+SERVERIP")) {                          //设置服务端IP
    if (!value.equals("")) {
      for (i = 0; i < value.length() && i < 30; i++)
        ParameterList._SERVER_IP[i] = value[i];
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._SERVER_IP), PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+SERVERPORT")) {                          //设置服务端端口号
    if (!value.equals("")) {
      ParameterList._SERVER_PORT = value.toInt();
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._SERVER_PORT), PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+CTWIFI")) {                             //连接WIFI
    connectToWifi(ParameterList._WIFI_SSID, ParameterList._WIFI_PSWD);
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+IPCONFIG")) {                           //查看IP地址
    printline_s("AT+CIFSR");
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+CTSERVER")) {                         //连接服务端
    connectToServer(ParameterList._SERVER_IP, ParameterList._SERVER_PORT);
    printline("OK", PRINT_MODE);
    return;
  }
  if (item.equals("AT+SERVERTYPE")) {                        //设置服务端连接类型
    if (!value.equals("")) {
      ParameterList._SERVER_TYPE = value.toInt();
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      Serial.print(ParameterList._SERVER_TYPE);
    }
    return;
  }
  if (item.equals("AT+SEND")) {                                 //发送Socket数据
    if (!value.equals("")) {
      sendData_Socket(value);
      printline("OK", PRINT_MODE);
    } else {
      printline("ERROR", PRINT_MODE);
    }
    return;
  }
  if (item.equals("AT+UNCT")) {                                 //断开连接
    if (value.equals("")) {
      closeConnection();
      printline("OK", PRINT_MODE);
    } else {
      printline("ERROR", PRINT_MODE);
    }
    return;
  }
  /* - - - - - - - - - - - - - - - - - - - - - - - - - */

  //未被识别的命令
  printline("ERROR", PRINT_MODE);

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
String ReceiveSoftwareSerialData(long Timer, boolean isEcho) {
  String temp_s = "";
  char temp_c;
  String KeyWord = "", KeyWord_Parm = "";

  if (Timer > 0)
    delay(Timer);
  if (SSerial.available() > 0) {
    while (SSerial.available() > 0)
    {
      //获取新字符
      temp_c = SSerial.read();

      //发现关键词
      findKeyWords(temp_c);

      //累加至字符串
      temp_s += temp_c;
      delay(2);
    }
    if (isEcho)showInfo("SSerial", temp_s);
  }
  return temp_s;
}

/**
  关键词检测
*/
void findKeyWords(char c) {
  const int N = 3;
  String words[N] = {"+IPD", "CONNECT FAIL", "CLOSED"};
  static int index[N] = {0};
  static String receiveline = "\n";
  static char startChar = '\n';

  //接收关键词后的参数
  if (!receiveline.equals("\n")) {
    //寻找开始符
    if (startChar != '\n') {
      if (c == startChar)
        startChar = '\n';
      return;
    }
    //非终止符时存储字符串
    if (c != '\n') {
      receiveline += c;
      return;
    }
    //输出字符串并初始化配置
    printline_h("Get Message: " + receiveline);                                    //在这里输出Socket接收到的字符串
    int i = receiveline.indexOf(_Separator);
    if (i == -1) {
      AT_Commands(receiveline, "", OUTPUTMODE_SOCKET);
    } else {
      printline_h(receiveline.substring(0, i));
      printline_h(receiveline.substring(i + 1));
      AT_Commands(receiveline.substring(0, i), receiveline.substring(i + 1), OUTPUTMODE_SOCKET);
    }
    receiveline = "\n";
    startChar = '\n';
  }

  int i;
  for (i = 0; i < N; i++) {
    //字符匹配失败
    if (c != words[i].charAt(index[i])) {
      index[i] = 0;
      continue;
    }
    //字符匹配成功
    index[i]++;
    if (index[i] >= words[i].length()) {
      //printline_h("Find a word:" + words[i]);
      index[i] = 0;
      switch (i) {
        case 0: //+IPD
          //开始接收数据
          receiveline = "";
          //开始符
          startChar = ':';
          break;
      }
    }
  }
  return;
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

/**
   发送Socket数据
*/
void sendData_Socket(String text) {
  String cmd = "AT+CIPSEND=" + String(text.length() + 1);
  printline_s(cmd);
  delay(100);
  SSerial.print(text);
//  SSerial.print("\n");
  SSerial.print(_TerminatorSS);
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
void connectToServer(String ip, int port) {
  String type;
  if (ParameterList._SERVER_TYPE)
    type = "UDP";
  else
    type = "TCP";
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
   输出一行字符串到指定串口
*/
void printline(String text, int mode) {
  switch (mode) {
    case OUTPUTMODE_HARD:
      printline_h(text);
      break;
    case OUTPUTMODE_SOFT:
      printline_s(text);
      break;
    case OUTPUTMODE_SOCKET:
      sendData_Socket(text);
      break;
  }
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



