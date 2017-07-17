void serialClrScr()
{
  if (VT100_MODE)
  {
    Serial.print (char(27));
    Serial.print ("[2J");
    Serial.print (char(27));
    Serial.print ("[H");
  }
}

boolean serialValidateDate(byte d, byte m, word y)
{
  byte    mArr[12] = {31,0,31,30,31,30,31,31,30,31,30,31};
  boolean ok=false;
  
  if (m==2)
  {
    if ((y % 4)==0)
    {
      if ((d>0) and (d<=29))
        ok = true;
    }
    else
    {
      if ((d>0) and (d<=28))
        ok = true;
    }
  }
  else
  {
    if ((d>0) and (d<=mArr[m-1]))
      ok = true;
  }
  
  return ok;
}

void serialSendDOW(byte dow)
{
  char* str[] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
  
  Serial.print(str[dow-1]);
  Serial.print(", ");
}

void serialSendMenu()
{
  serialClrScr();
  Serial.println ("UTFT_Analog_Clock (C)2010-2012 Henning Karlsen");
  Serial.println ("----------------------------------------------");
  Serial.println ("T : Set time");
  Serial.println ("D : Set date");
  Serial.println ("R : Read current time and date");
  Serial.println ("? : Print menu");
  Serial.println ("Q : Quit Serial Mode");
  Serial.println ();
}

void serialSetTime()
{
  char buf[6];
  char tmp;
  int  cnt=0;
  int  h,m,s;
  
  Serial.print("Enter time in 24-hour format [hhmmss]: ");
  while (cnt<6)
  {
    while(Serial.available()==0) {};
    tmp = Serial.read();
    if ((tmp>='0') and (tmp<='9'))
    {
      buf[cnt]=tmp;
      cnt++;
      Serial.print(tmp);
    }
  }
  Serial.println();
  h = ((buf[0]-'0')*10) + (buf[1]-'0');
  m = ((buf[2]-'0')*10) + (buf[3]-'0');
  s = ((buf[4]-'0')*10) + (buf[5]-'0');
  if ((h>23) or (m>59) or (s>59))
    Serial.println("ERROR: Invalid time");
  else
  {
    rtc.setTime(h,m,s);
    Serial.print("New time set to ");
    if (h<10)
      Serial.print("0");
    Serial.print(h,DEC);
    Serial.print(":");
    if (m<10)
      Serial.print("0");
    Serial.print(m,DEC);
    Serial.print(":");
    if (s<10)
      Serial.print("0");
    Serial.print(s,DEC);
    Serial.println("...");
  }
}

void serialSetDate()
{
  char buf[8];
  char tmp;
  int  cnt=0;
  int  d,m,y;
  
  Serial.print("Enter date [ddmmyyyy]: ");
  while (cnt<8)
  {
    while(Serial.available()==0) {};
    tmp = Serial.read();
    if ((tmp>='0') and (tmp<='9'))
    {
      buf[cnt]=tmp;
      cnt++;
      Serial.print(tmp);
    }
  }
  Serial.println();
  d = ((buf[0]-'0')*10) + (buf[1]-'0');
  m = ((buf[2]-'0')*10) + (buf[3]-'0');
  y = ((buf[4]-'0')*1000) + ((buf[5]-'0')*100) + ((buf[6]-'0')*10) + (buf[7]-'0');
  if ((y<2000) or (y>2099))
    Serial.println("ERROR: Invalid time");
  else
    if ((m<1) or (m>12))
      Serial.println("ERROR: Invalid time");
    else
      if (!serialValidateDate(d,m,y))
        Serial.println("ERROR: Invalid time");
      else
      {
        rtc.setDate(d,m,y);
        rtc.setDOW(calcDOW(d,m,y));
        Serial.print("New date set to ");
        serialSendDOW(calcDOW(d,m,y));
        if (d<10)
          Serial.print("0");
        Serial.print(d,DEC);
        Serial.print(".");
        if (m<10)
          Serial.print("0");
        Serial.print(m,DEC);
        Serial.print(".");
        Serial.print(y,DEC);
        Serial.println("...");
      }
}

void serialReadTimeDate()
{
  Serial.print("Time: ");
  Serial.println(rtc.getTimeStr());
  Serial.print("Date: ");
  Serial.print(rtc.getDOWStr(FORMAT_SHORT));
  Serial.print(", ");
  Serial.println(rtc.getDateStr());
}

void serialMode()
{
  boolean quitMode = false;
  char    cmd;
  
  myGLCD.clrScr();
  myGLCD.setFont(BigFont);
  myGLCD.setColor(255,255,255);
  myGLCD.setBackColor(0,0,0);
  myGLCD.print("SERIAL MODE", CENTER, (myGLCD.getDisplayYSize()/2)-8);
  
  while (Serial.available()>0)
    Serial.read();
    
  serialSendMenu();
  Serial.print ("> ");
  
  while (quitMode==false)
  {
    if (Serial.available()>0)
    {
      cmd = uCase(Serial.read());
      Serial.println(cmd);
      switch (cmd)
      {
        case 'T': serialSetTime();
                  break;
        case 'D': serialSetDate();
                  break;
        case 'R': serialReadTimeDate();
                  break;
        case '?': serialSendMenu();
                  break;
        case 'Q': Serial.println("Exiting serial mode...");
                  Serial.println();
                  quitMode = true;
                  break;
        default:  Serial.println("ERROR: Unknown command");
                  break;
      }
      if (quitMode==false)
        Serial.print("> ");
    }
  }
  while (Serial.available()>0)
    Serial.read();
  drawDisplay();
  printDate();
  Serial.println("Send any character to enter serial mode again...");
  Serial.println();
}


