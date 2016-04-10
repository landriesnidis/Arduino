#include <EEPROM.h>

#define _Terminator                   '\n'                      //终止符
#define _Separator                    '='                       //分隔符
#define _CommandArraySize             10                        //串口缓存数组长度

boolean _BoolAT = false;

/**
   EEPROM-参数表
*/
struct ParameterBean {
  unsigned long _Timestamp =           50;                           //每0.05秒循环
  unsigned int _SerialBufferSize =    1024;                         //串口缓存字符长度
  unsigned long _BaudRate =           9600;                         //波特率
  const char _Version[30] =           "Multitasking_template 1.0.0";//版本信息
  const char _Date[11] =              "2016-03-25";                 //烧录时间
} ParameterList;

/**
   存储参数表
*/
void saveParameterBean() {
  EEPROM.put(sizeof(boolean), ParameterList);
  delay(100);
}

//设置命令队列
String CommandQueue[_CommandArraySize];
unsigned int CommandQueue_num = 0;

/**
   初始化
*/
void setup() {

  boolean sign;
  EEPROM.get(0, sign);

  if (!sign) {
    sign = true;
    EEPROM.put(0, sign);
    saveParameterBean();
  } else {
    EEPROM.get(sizeof(boolean), ParameterList);
  }

  delay(100);
  Serial.begin(ParameterList._BaudRate);   //设置串口波特率
  delay(100);
  Serial.print("START");
  Serial.print(_Terminator);

}

void loop() {

  //  if (!Serial)
  ReceiveSerialData();

  if (CommandQueue_num > 0)
    AT_Analyze();

  if (!_BoolAT) {
    Function1(1000);
    Function2(2000);
  }



  delay(ParameterList._Timestamp);
}

/**
   例子：通用时间戳函数
*/
void Function1(const unsigned long Interval) {
  //*******************************************************
  static unsigned int freq = 0;                         //*
  if (++freq * ParameterList._Timestamp < Interval) {   //*
    return;                                             //*
  } else {                                              //*
    freq = 0;                                           //*
  }                                                     //*
  //*******************************************************

  //代码区域:开始
  Serial.print("Function1:Done.");
  Serial.print(_Terminator);
  //代码区域:结束
}

/**
   例子：通用时间戳函数
*/
void Function2(const unsigned long Interval) {
  //*******************************************************
  static unsigned int freq = 0;                         //*
  if (++freq * ParameterList._Timestamp < Interval) {   //*
    return;                                             //*
  } else {                                              //*
    freq = 0;                                           //*
  }                                                     //*
  //*******************************************************

  //代码区域:开始
  Serial.print("Function2:Done.");
  Serial.print(_Terminator);
  //代码区域:结束
}

/**
   软复位
*/
void( *resetFunc) (void) = 0;

/**
   接收串口通信数据
*/
void ReceiveSerialData() {
  static String temp_s = "";
  char temp_c;

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
   添加串口缓存数据
*/
boolean addSerialData(String comdata) {
  if (CommandQueue_num >= _CommandArraySize) {
    return false;
  } else {
    CommandQueue_num++;
    CommandQueue[CommandQueue_num - 1] = comdata;
    return true;
  }
}

/**
   取出一项串口数据
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
   AT指令解析
*/
void AT_Analyze() {
  String command = getSerialData();
  String item = "", value = "";
  int index = -1;
  while (!command.equals("")) {
    index = command.indexOf(_Separator);
    if (index == -1) {
      AT_Commands(command, "");
      return;
    }
    item = command.substring(0, index);
    value = command.substring(index + 1);
    command = getSerialData();
    AT_Commands(item, value);
  }
}

/**
   AT指令集
*/
void AT_Commands(String item, String value) {

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

  //未被识别的命令
  Serial.print("ERROR");
  Serial.print(_Terminator);
}

