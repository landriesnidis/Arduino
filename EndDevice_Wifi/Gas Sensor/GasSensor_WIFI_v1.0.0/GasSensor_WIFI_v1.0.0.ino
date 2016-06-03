/*
  #更新说明 v1.0.0
  1.气体检测传感器,实时监测当前环境下的甲烷/一氧化碳/烟雾等.
  2.当检测数值达到警戒线时触发蜂鸣器并报警.

  #功能介绍
  1.支持AT指令(可自定义指令);
  2.可通过AT指令查改配置信息;
  3.配置信息保存于EEPROM中,断电后不会丢失;
  4.可自定义设置时间片的间隔时长;
  5.当时间片间隔过长时,多条AT指令缓存入栈中等待执行,栈长度可通过宏定义"_ATStackSize"设置;
  6.支持软串口,可使用AT指令通过硬件串口向软串口发送数据,使用软串口数据接收函数可将数据转发回指定串口(或Socket通信);
  7.支持Socket远程执行AT指令并返回结果(远程命令不缓存至栈内);
  8.气体检测传感器,实时监测当前环境下的甲烷/一氧化碳/烟雾等.
  9.当检测数值达到警戒线时触发蜂鸣器并报警.
  
  #备注
  1.该程序使用模板:EndDevice_Wifi_v1.1.2(支持多任务和WIFI模块通信的开源模块程序模板);

  #作者信息
  ID:Landriesnidis
  E-mail:Landriesnidis@yeah.net
  Github:https://github.com/landriesnidis


  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  数字引脚：
    ◎ 00  ……………………  RX
    ◎ 01  ……………………  TX
    ◎ 02  ……………………  模拟RX
    ◎ 03  ……………………
    ◎ 04  ……………………  模拟TX
    ◎ 05  ……………………  空
    ◎ 06  ……………………  空
    ◎ 07  ……………………  空
    ◎ 08  ……………………  空
    ◎ 09  ……………………  空
    ◎ 10  ……………………  空
    ◎ 11  ……………………  空
    ◎ 12  ……………………  空
    ◎ 13  ……………………  板载IED

    ◎ a0  ……………………  MQ-6 可燃气体检测
    ◎ a1  ……………………  MQ-2 烟雾等有害气体检测
    ◎ a2  ……………………  空
    ◎ a3  ……………………  空
    ◎ a4  ……………………  空
    ◎ a5  ……………………  空

  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

#include <EEPROM.h>
#include <SoftwareSerial.h>

/* - - - - - - - - -模板宏定义 - - - - - - - - - - */
#define _Terminator                     '\n'                      //硬件串口终止符
#define _TerminatorSS                   "\r\n"                    //软件串口终止符(连接的模块所使用的终止符)
#define _Separator                      '='                       //AT指令参数分隔符
#define _ATStackSize                    10                        //AT指令缓存栈长度

#define OUTPUTMODE_HARD                 0                        //输出模式:硬件串口
#define OUTPUTMODE_SOFT                 1                        //输出模式:软件串口
#define OUTPUTMODE_SOCKET               2                        //输出模式:Socket通信

/* - - - - - - - - 自定义宏定义 - - - - - - - - - -*/
#define MQ6PIN                          0                        //可燃气体模拟引脚
#define MQ2PIN                          1                        //烟雾报警模拟引脚
#define BEEPIN                          11                       //烟雾报警模拟引脚
/* - - - - - - - - - - - - - - - - - - - - - - - - */

//软串口设置
SoftwareSerial SSerial(2, 4);                                   // RX, TX

//设备信息常量
const String _VERSION = "v1.0.0";                               //版本号
const String _DEVICE  = "GS";                                   //设备类型
const String _DATE    = "2016-05-26";                           //烧录日期

//控制开关
boolean _BoolAT        = false;
boolean _BoolCT2Server = false;
boolean _BoolBee       = false;

//设置命令队列
String CommandQueue[_ATStackSize];
unsigned int CommandQueue_num = 0;

/**
   EEPROM-参数表
*/
struct StructParam {
  /* - - - - - - - - 基本设置成员变量 - - - - - - - - - -*/
  unsigned long _LoopCycle =          50;                               //每0.05秒循环
  unsigned int  _SerialBufferSize =   1024;                             //串口缓存字符长度
  unsigned long _BaudRate =           9600;                             //波特率
  unsigned long _SSBaudRate =         9600;                             //软串口波特率
  short         _SS_ECHO =            0;                                //软串口数据回显流向 0:null  1:hard  2:soft  3:socket
  /* - - - - - - - - WIFI模块成员变量 - - - - - - - - - -*/
  char _WIFI_SSID[30] =               "KoalaTrans";                     //WIFI名称
  char _WIFI_PSWD[30] =               "app2team";                       //WIFI密码
  bool _SERVER_TYPE   =               0;                                //服务器连接类型 0:TCP 1:UDP
  char _SERVER_IP[30] =               "172.19.58.16";                   //服务器地址
  int  _SERVER_PORT   =               12346;                            //服务器端口
  bool _SERVER_AUTOCT =               1;                                //服务器自动连接AT+GSVALUE
  /* - - - - - - - - 自定义成员变量 - - - - - - - - - -*/
  char _DEVICE_ID[20] =               "device";                         //当前设备的名称
  unsigned int _ALARM_INTERVAL =      3000;                              //警报发送间隔时间
  unsigned short _MQ2_ALARMVALUE =    800;                              //烟雾有害气体报警值
  unsigned short _MQ6_ALARMVALUE =    60;                                //可燃气体报警值
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

  //引脚模式
  pinMode(BEEPIN,OUTPUT);
  
  //START
  delay(50);
  printline_h("START");
}

/**
  循环函数
*/
void loop() {

  //接收软硬串口数据
  ReceiveSerialData();
  ReceiveSoftwareSerialData(0, ParameterList._SS_ECHO);

  //主动执行命令解析
  if (CommandQueue_num > 0)
    AT_Analyze(getSerialData());

  //自定义函数执行列表
  if (!_BoolAT) {
    if (_BoolCT2Server) Server_autoConnect(30000);       //断开连接后自动与服务器重连
    GSWarning(ParameterList._ALARM_INTERVAL);
    if(_BoolBee)Bee(50);
  }

  //时间片
  delay(ParameterList._LoopCycle);
}

/*
   可燃气体有害气体报警
*/
void GSWarning(const unsigned long Interval) {
  /*— — — — — — — — — ↓ 时间控制 请勿改动 ↓ — — — — — — — — — — */
  static unsigned int freq = 0;
  if (++freq * ParameterList._LoopCycle < Interval) {
    return;
  } else {
    freq = 0;
  }
  /*— — — — — — — — — ↑ 时间控制 请勿改动 ↑ — — — — — — — — — — */

  /*代码区域:开始*/
  String data = "GSWARNING ";
  bool s = 0;
  
  if (analogRead(MQ6PIN) > ParameterList._MQ6_ALARMVALUE) {
    data += "MQ6 ";
    s=1;
  }
  if (analogRead(MQ2PIN) > ParameterList._MQ2_ALARMVALUE) {
    data += "MQ2 ";
    s=1;
  }
  if(s){
    sendData_Socket(data);
    
  }
  _BoolBee = s;
  /*代码区域:结束*/
}

/*
   蜂鸣器
*/
void Bee(const unsigned long Interval) {
  /*— — — — — — — — — ↓ 时间控制 请勿改动 ↓ — — — — — — — — — — */
  static unsigned int freq = 0;
  if (++freq * ParameterList._LoopCycle < Interval) {
    return;
  } else {
    freq = 0;
  }
  /*— — — — — — — — — ↑ 时间控制 请勿改动 ↑ — — — — — — — — — — */

  /*代码区域:开始*/
  int state = digitalRead(BEEPIN);
  digitalWrite(BEEPIN,!state);
  /*代码区域:结束*/
}

/**
   与服务器断开连接自动重连
*/
void Server_autoConnect(const unsigned long Interval) {
  static unsigned int freq = 0;
  if (++freq * ParameterList._LoopCycle < Interval) {
    return;
  } else {
    freq = 0;
  }
  AT_Commands("AT+CTSERVER", "", 0);
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
String ReceiveSoftwareSerialData(long Timer, short isEcho) {
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
    if (isEcho)showInfo("SSerial", temp_s, isEcho);
  }
  return temp_s;
}

/**
  关键词检测
*/
void findKeyWords(char c) {
  const int N = 5;
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
      AT_Commands(receiveline.substring(0, i), receiveline.substring(i + 1), OUTPUTMODE_SOCKET);
    }
    receiveline = "\n";
    startChar = '\n';
  }

  String words[N] = {"+IPD", "CONNECT FAIL", "CLOSED", "GOT IP", "CONNECT"};
  for (int i = 0; i < N; i++) {
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
        case 0: //+IPD    接受到数据
          //开始接收数据
          receiveline = "";
          //开始符
          startChar = ':';
          break;
        case 2: //CLOSED  与服务器断开连接
          if (ParameterList._SERVER_AUTOCT) {
            _BoolCT2Server = true;
          }
          break;
        case 3: //GOT IP  获取到IP地址
          if (ParameterList._SERVER_AUTOCT) {
            AT_Commands("AT+CTSERVER", "", 0);
          }
          break;
        case 4: //CONNECT 与服务器建立连接
          _BoolCT2Server = false;
          break;
      }
    }
  }
  return;
}

/**
   格式化输出文本信息
*/
void showInfo(String title, String text, short PRINT_MODE) {
  text.replace("\n\n", "\n");
  text.replace("\n", "\n\t|");
  printline("[" + title + "-BEGIN]\n\t|" + text + _Terminator + "[" + title + "-END]", PRINT_MODE - 1);
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

String string_add_arrchar(String s, char *c) {
  while (*c != '\0') {
    s += *c++;
  }
  return s;
}












/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

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
    if (value.toInt() >= 0 && value.toInt() <= 3) {
      ParameterList._SS_ECHO = value.toInt();
      printline("OK", PRINT_MODE);
    } else {
      printline("ERROR", PRINT_MODE);
    }
    saveParameterBean();
    return;
  }
  if (item.equals("AT+BAUD")) {                         //设置硬件波特率
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
    printline(_VERSION, PRINT_MODE);
    return;
  }
  if (item.equals("AT+DEVICE")) {                      //查看设备类型
    printline("DEVICE " + _DEVICE, PRINT_MODE);
    return;
  }
  if (item.equals("AT+DATE")) {                         //查看烧录时间
    printline(_DATE, PRINT_MODE);
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
  if (item.equals("AT+DEVICEID")) {                             //设置设备ID
    if (!value.equals("")) {
      for (int i = 0; i < value.length() && i < 20; i++)
        ParameterList._DEVICE_ID[i] = value[i];
      printline("OK", PRINT_MODE);
      saveParameterBean();
    } else {
      printline(string_add_arrchar("DEVICEID ", ParameterList._DEVICE_ID), PRINT_MODE);
    }
    return;
  }
  if (item.equals("OK")) {                              //"OK"
    return;
  }

  /* - - - - - - - - - WIFI模块AT指令 - - - - - - - - - -*/
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
  if (item.equals("AT+SERVERAUTOCT")) {                        //设置启用服务器断开重连模式
    if (!value.equals("")) {
      ParameterList._SERVER_AUTOCT = value.toInt();
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      Serial.print(ParameterList._SERVER_AUTOCT);
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
  /* - - - - - - -温湿度模块控制AT指令 -  - - - - - - -*/

  if (item.equals("AT+ALARM")) {                              //设置警报间隔时间
    if (!value.equals("")) {
      ParameterList._ALARM_INTERVAL = value.toInt();
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._ALARM_INTERVAL), PRINT_MODE);
    }
    return;
  }

  if (item.equals("AT+GSVALUE")) {                              //获取当前可燃气体与烟雾的数值
    String data = "GS ";
    data += analogRead(MQ6PIN);    //可燃气体
    data += " ";
    data += analogRead(MQ2PIN); //烟雾
    printline(data, PRINT_MODE);
    return;
  }

  if (item.equals("AT+ALARMVALUE")) {                             //设置可燃气体与有害烟雾警戒值
    if (!value.equals("")) {
      int sp = value.indexOf(',');
      if (sp == -1) {
        printline("ERROR", PRINT_MODE);
        return;
      }
      int mq6 = value.substring(0, sp).toInt();
      int mq2 = value.substring(sp + 1).toInt();
      ParameterList._MQ6_ALARMVALUE = mq6;
      ParameterList._MQ2_ALARMVALUE = mq2;
      saveParameterBean();
      printline("OK", PRINT_MODE);
    } else {
      printline(String(ParameterList._MQ6_ALARMVALUE) + "," + String(ParameterList._MQ2_ALARMVALUE), PRINT_MODE);
    }
    return;
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - */

  //未被识别的命令
  printline("ERROR", PRINT_MODE);

}


