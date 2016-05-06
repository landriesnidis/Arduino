/*
  #更新说明 v1.0.0

  #功能介绍
  1.初始化EEPROM中用以模板标记的变量;
  
  #备注
  1.当前目录下的模板代码烧录前都需要先执行该程序;
  2.适用模板名称:Multitasking_template、EndDevice_Wifi(包括以该模板延伸的代码)等.

  #作者信息
  ID:Landriesnidis
  E-mail:Landriesnidis@yeah.net
  Github:https://github.com/landriesnidis
*/

#include <EEPROM.h>
void setup() {
  EEPROM.put(0, false);
}

void loop() {

}
