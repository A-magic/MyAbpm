
HardwareSerial mySerial(2);

const char *hex = "FE 78 50 3C 00 00";

byte Readmv[6] = {254, 120, 80, 60, 00, 00}; //校准命令
byte readco[6] = {253, 00, 00, 00, 00, 00};  //读取命令
byte Eraseco[6] = {250, 00, 00, 00, 00, 00}; //擦除校准信息
char SensorData[6];                          //串口数据缓冲区


void bp_get_value()
{

  char comdata[18]; //串口数据缓冲区
  while (mySerial.available() > 0)
  {
    mySerial.readBytes(comdata, 6);
    delay(2);
  }
  Serial.print("返回值：");

  for (int i = 0; i < 7; i++)
  {
    Serial.print(comdata[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  if (comdata[4] == 0x00 && comdata[5] == 0x00)
  {

    switch (comdata[0])
    {
    case 0xFE:
      switch (comdata[3])
      {
      case 0x00:
        Serial.println("校准成功!");
        break;
      case 0x01:
        Serial.println("校准中。。。。。。");
        bp_get_value();
        break;
      case 0x02:
        Serial.println("校准失败！！！立即重新发送校准命令");
        // bp_send_order(Readmv);
        break;
      default:
        break;
      }
      break;
    case 0xFD:

      for (size_t i = 0; i < 6; i++)
      {
        SensorData[i] = comdata[i];
      }

      Serial.println("读取到体征参数");
      break;
    case 0xFB:
      Serial.println("可能是预留信息，待查");
      break;
    case 0xFA:
      Serial.println("校准信息擦除成功");
      break;
    default:
      Serial.println("数据头错误");
      bp_get_value();
      break;
    }
  }
  else
  {
    Serial.println("数据格式错误");
  }
}

void bp_send_order(byte *myOrders)
{

  mySerial.write(&myOrders[0], 6); //向血压探测器发送校准命令
  bp_get_value();
}

void bp_check_order(byte *myOrders)
{
  switch (myOrders[0])
  {
  case 0xFE:
    Serial.println("发送校准命令======>>>");
    bp_send_order(Eraseco); //先清除一遍，再进行校准
    bp_send_order(Readmv);
    break;
  case 0xFD:
    Serial.println("发送读参命令======>>>");
    bp_send_order(readco);
    break;
  case 0xFA:
    Serial.println("清除校准命令======>>>");
    bp_send_order(Eraseco);
    break;
  default:
    Serial.println("命令错误！");
    break;
  }
}
