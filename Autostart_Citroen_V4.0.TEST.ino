#include <SoftwareSerial.h>
SoftwareSerial SIM800(7, 6);        // для новых плат начиная с версии RX,TX
#include <DallasTemperature.h>      // подключаем библиотеку чтения датчиков температуры
OneWire oneWire(4);                 // и настраиваем  пин 4 как шину подключения датчиков DS18B20
DallasTemperature sensors(&oneWire);

#define FIRST_P_Pin  8              // на реле первого положения замка зажигания
#define SECOND_P     9              // на реле зажигания, через транзистор с 9-го пина ардуино
#define STARTER_Pin  12             // на реле стартера, через транзистор с 12-го пина ардуино
#define WEBASTO_pin  11             // на реле вебасто или подогрева седушек
#define ACTIV_Pin    13             // на светодиод (моргалку)
#define BAT_Pin      A0             // на батарею, через делитель напряжения 39кОм / 11 кОм
#define Feedback_Pin A1             // на провод от замка зажигания
#define STOP_Pin     A2             // на концевик педали тормоза для отключения режима прогрева
#define PSO_Pin      A3             // на прочие датчики через делитель 39 kOhm / 11 kΩ
#define REL_Pin      10             // на дополнительное реле

/*  ----------------------------------------- ИНДИВИДУАЛЬНЫЕ НАСТРОЙКИ !!!---------------------------------------------------------   */
String call_phone= "+375290000000"; // телефон входящего вызова  
String SMS_phone = "+375290000000"; // телефон куда отправляем СМС  s
String MAC = "80-01-AA-00-00-00";   // МАС-Адрес устройства для индентификации на сервере narodmon.ru (придумать свое 80-01-XX-XX-XX-XX-XX)
String SENS = "VasjaPupkin";        // Название устройства (придумать свое Citroen 566 или др. для отображения в программе и на карте)
String APN = "internet.mts.by";     // тчка доступа выхода в интернет вашего сотового оператора
String USER = "mts";                // имя выхода в интернет вашего сотового оператора
String PASS = "mts";                // пароль доступа выхода в интернет вашего сотового оператора
bool  n_send = true;                // отправка данных на народмон по умолчанию включена (true), отключена (false)
bool  sms_report = true;            // отправка SMS отчета по умолчанию овключена (true), отключена (false)
float Vstart = 13.50;               // поорог распознавания момента запуска по напряжению
/*  ----------------------------------------- ДАЛЕЕ НЕ ТРОГАЕМ --------------------------------------------------------------------   */
String SERVER = "narodmon.ru";     // сервер народмона (на октябрь 2017) 
String PORT = "8283";              // порт народмона   (на октябрь 2017) 
String pin= "";                    // строковая переменная набираемого пинкода
String at = "";
float TempDS0, TempDS1, TempDS2 ;  // переменные хранения температуры
float Vbat;                        // переменная хранящая напряжение бортовой сети
float VbatStart;                   // переменная хранящая напряжение бортовой сети
float m = 66.91;                   // делитель для перевода АЦП в вольты для резистров 39/11kOm
unsigned long Time1 = 0; 
int k = 0;
int count;                         // количество совершенных попыток старnа с последнего запуска
int interval = 10;                 // интервал отправки данных на народмон сразу после загрузки
int Timer = 0;                     // таймер времени прогрева двигателя по умолчанию = 0
int Timer2 = 1080;                 // часовой таймер (60 sec. x 60 min. / 10 = 360 )
bool heating = false;              // переменная состояния режим прогрева двигателя
bool SMS_send = false;             // флаг разовой отправки СМС


void setup() {
  pinMode(ACTIV_Pin,   OUTPUT);    // указываем пин на выход (светодиод)
  pinMode(SECOND_P,    OUTPUT);    // указываем пин на выход доп реле зажигания
  pinMode(STARTER_Pin, OUTPUT);    // указываем пин на выход доп реле стартера
  pinMode(REL_Pin,     OUTPUT);    // указываем пин на выход для доп нужд.
  pinMode(FIRST_P_Pin, OUTPUT);    // указываем пин на выход для доп реле первого положения замка зажигания
  pinMode(WEBASTO_pin, OUTPUT);    // указываем пин на выход для доп реле вебасто
  pinMode(3, INPUT_PULLUP);        // указываем пин на вход для тревожного датчика с внутричипной подтяжкой к +3.3V
  Serial.begin(9600);              //скорость порта
  SIM800.begin(9600);              //скорость связи с модемом
  Serial.println("загрузка V4.0 | MAC:"+MAC+" | NAME:"+SENS+" | APN:"+APN+" | TEL:"+call_phone+" | 25/11/2017"); 
  delay (2000);
  SIM800_reset();
  attachInterrupt(1, callback, FALLING);  // включаем прерывание при переходе 1 -> 0 на D3, или 0 -> 1 на ножке оптопары
             }

void SIM800_reset() {                                         // Call Ready
/*  --------------------------------------------------- ПРЕДНАСТРОЙКА МОДЕМА SIM800L ------------------------------------------------ */   
// digitalWrite(RESET_Pin, LOW),delay(2000);
// digitalWrite(RESET_Pin, HIGH), delay(5000);
// SIM800.println("AT+CFUN=1,1");
  SIM800.println("ATE1"),                    delay(50);        // отключаем режим ЭХА
  SIM800.println("AT+CLIP=1"),               delay (50);       // включаем определитель номера
  SIM800.println("AT+DDET=1"),               delay (50);       // включаем DTMF декдер
  SIM800.println("AT+CMGF=1"),               delay (50);       // Включаем Текстовый режим СМС
  SIM800.println("AT+CSCS=\"gsm\""),         delay (50);       // Выбираем кодировку СМС
  SIM800.println("AT+CNMI=2,1,0,0,0") ,      delay (100);      // Разрешаем прием входящих СМС
  SIM800.println("AT+VTD=1"),                delay (50);       // Задаем длительность DTMF ответа
  SIM800.println("AT+CMEE=1"),               delay (50);       // Инфо о ошибках 
  SIM800.println("AT+CMGDA=\"DEL ALL\""),    delay (30);       // Удаляем все СМС полученные ранее
               } 

void callback(){                                               // обратный звонок при появлении напряжения на входе IN1
    SIM800.println("ATD"+call_phone+";"),    delay(5000); 
               }


void loop() {
  if (SIM800.available()) {                                               // если что-то пришло от SIM800 в SoftwareSerial Ардуино
    while (SIM800.available()) k = SIM800.read(), at += char(k),delay(1); // набиваем в переменную at
    Serial.println(at);                                                   // Возвращаем ответ можема в монитор порта
    
    if (at.indexOf("+CLIP: \""+call_phone+"\",") > -1 /* && at.indexOf("+CMGR:") == -1 */) {
                                                     SIM800.println("ATA"), delay(1000);
                                                     SIM800.println("AT+VTS=\"3,5,7\"");
                                                     pin= "";
/*            
    } else if (at.indexOf("+CLIP: \""+SMS_phone+"\",") > -1 && at.indexOf("+CMGR:") == -1 ) {
                                                     delay(50), SIM800.println("ATH0");
                                                     Timer = 60, enginestart(3);
 */           
     } else if (at.indexOf("+DTMF: ")  > -1)        {String key = at.substring(at.indexOf("+DTMF: ")+7, at.indexOf("+DTMF: ")+8);
                                                     pin = pin + key;
                                                     if (pin.indexOf("*") > -1 ) pin= "", SIM800.println ("AT+VTS=\"9,7,9\""); 
                                               //    Serial.print (" Starting pin:"), Serial.println(pin);        

                 
      } else if (at.indexOf("+CMTI: \"SM\",") > -1) {int i = at.substring(at.indexOf("\"SM\",")+5, at.indexOf("\"SM\",")+6).toInt();
                                                    delay(2000), SIM800.print("AT+CMGR="), SIM800.println(i); // читаем СМС 
//                     // delay(20), SIM800.println("AT+CMGDA=\"DEL ALL\""), delay(20); //  и удаляем их все
      
      } else if (at.indexOf("123start") > -1   )    {/*Timer = at.substring(at.indexOf("123start")+8, at.indexOf("123start")+10).toInt() *6;*/ enginestart(5);
      } else if (at.indexOf("123webasto") > -1 )    {Timer = 120, webasto();
      } else if (at.indexOf("123stop") > -1 )       {Timer=0, heatingstop();     
   //   } else if (at.indexOf("narodmon=off") > -1 )    {n_send = false;  
   //   } else if (at.indexOf("narodmon=on") > -1 )     {n_send = true;  
   //   } else if (at.indexOf("sms=off") > -1 )         {sms_report = false;  
   //   } else if (at.indexOf("sms=on") > -1 )          {sms_report = true;     
    } else if (at.indexOf("SMS Ready") > -1 )         {SIM800_reset();       

    /*  -------------------------------------- ВХОДИМ В ИНТЕРНЕТ И ОТПРАВЛЯЕМ ПАКЕТ ДАННЫХ НА СЕРВЕР--------------------------------- */
    } else if (at.indexOf("AT+CGATT=1\r\r\nOK\r\n") > -1 )         {SIM800.println("AT+CSTT=\""+APN+"\""),     delay (500);   // установливаем точку доступа  
    } else if (at.indexOf("AT+CSTT=\""+APN+"\"\r\r\nOK\r\n") > -1 ){SIM800.println("AT+CIICR"),                delay (2000);  // устанавливаем соеденение   
    } else if (at.indexOf("AT+CIICR") > -1 )                       {SIM800.println("AT+CIFSR"),                delay (1000);  // возвращаем IP-адрес модуля    
    } else if (at.indexOf("AT+CIFSR") > -1 )                       {SIM800.println("AT+CIPSTART=\"TCP\",\""+SERVER+"\",\""+PORT+"\""), delay (1000);    
    } else if (at.indexOf("CONNECT OK\r\n") > -1 )                 {SIM800.println("AT+CIPSEND"),              delay (1200) ;  // меняем статус модема
    } else if (at.indexOf("AT+CIPSEND\r\r\n>") > -1 )              {                             // "набиваем" пакет данными 
                         SIM800.print("#" +MAC+ "#" +SENS);                                      // заголовок пакета с MAC адресом и именем устройства      
                         SIM800.print("\n#Temp1#"),                SIM800.print(TempDS0);        // Температура с датчика №1      
                         SIM800.print("\n#Temp2#"),                SIM800.print(TempDS1);        // Температура с датчика №2      
                         SIM800.print("\n#Temp3#"),                SIM800.print(TempDS2);        // Температура с датчика №3      
                         SIM800.print("\n#Vbat#"),                 SIM800.print(Vbat);           // Напряжение аккумулятора
                         SIM800.print("\n#Uptime#"),               SIM800.print(millis()/1000);  // Время непрерывной работы
                         SIM800.println("\n##");                                                 // закрываем пакет
                         SIM800.println((char)26), delay (100);                                  // отправляем пакет
     } 
at = "";  // очищаем переменную

}
/*  --------------------------------------------------- НЕПРЕРЫВНАЯ ПРОВЕРКА СОБЫТИЙ ----------------------------------------------- */   

     if (Serial.available()) {             //если в мониторе порта ввели что-то
    while (Serial.available()) k = Serial.read(), at += char(k), delay(1);
    SIM800.println(at), at = "";  //очищаем
                             }

if (pin.indexOf("123") > -1 ) pin= "", SIM800.println ("AT+VTS=\"1,2,3\""), enginestart(3);  
if (pin.indexOf("456") > -1 ) pin= "", SIM800.println ("AT+VTS=\"4,5,6\""), enginestart(5);  
if (pin.indexOf("147") > -1 ) pin= "", SIM800.println ("AT+VTS=\"1,4,7\""), Timer = 120, webasto();
if (pin.indexOf("789") > -1 ) pin= "", SIM800.println ("AT+VTS=\"A,B,D\""), Timer=0, heatingstop();  
if (pin.indexOf("#")   > -1 ) pin= "", SIM800.println("ATH0"),              SMS_send = true;        
if (millis()> Time1 + 10000) detection(), Time1 = millis();       // выполняем функцию detection () каждые 10 сек 
if (heating == true && digitalRead(STOP_Pin)==1) heatingstop();   //если нажали на педаль тормоза в режиме прогрева

}

void detection(){                           // условия проверяемые каждые 10 сек  
    sensors.requestTemperatures();          // читаем температуру с трех датчиков
    TempDS0 = sensors.getTempCByIndex(0);   // датчик на двигатель
    TempDS1 = sensors.getTempCByIndex(1);   // датчик в салон
    TempDS2 = sensors.getTempCByIndex(2);   // датчик на улицу 
    
    if (TempDS0 == -127) TempDS0 = 86;
    if (TempDS1 == -127) TempDS1 = 87;
    if (TempDS2 == -127) TempDS2 = 88;
    
    Vbat = analogRead(BAT_Pin);             // замеряем напряжение на батарее
    Vbat = Vbat / m ;                       // переводим попугаи в вольты
    
    Serial.print("Vbat= "),Serial.print(Vbat), Serial.print (" V.");  
    Serial.print(" || Temp1 : "), Serial.print(TempDS0);
    Serial.print(" || Temp2 : "), Serial.print(TempDS1);
    Serial.print(" || Temp3: "),  Serial.print(TempDS2);  
    Serial.print(" || Interval : "), Serial.print(interval);
    Serial.print(" || Timer ="), Serial.println (Timer);
       

  
    if (SMS_send == true && sms_report == true) { SMS_send = false;          // если фаг SMS_send равен 1 высылаем отчет по СМС
        SIM800.println("AT+CMGS=\""+SMS_phone+"\""), delay(100);
        SIM800.print("Privet "+SENS+".!");
        SIM800.print("\n Temp.Dvig: "),  SIM800.print(TempDS0);
        SIM800.print("\n Temp.Salon: "), SIM800.print(TempDS1);
        SIM800.print("\n Temp.Ulica: "), SIM800.print(TempDS2); 
        SIM800.print("\n Uptime: "),     SIM800.print(millis()/3600000),        SIM800.print("H.");
        SIM800.print("\n Vbat     : "),  SIM800.print(Vbat),                    SIM800.print("V.");
        if (digitalRead(Feedback_Pin) == HIGH)  SIM800.print("\n VbatStart: "), SIM800.print(VbatStart), SIM800.print("V.");
        if (heating == true)             SIM800.print("\n Timer "),             SIM800.print(Timer/6),   SIM800.print("min.");
        if (n_send ==  true)             SIM800.print("\n narodmon.ru ON ");        
        SIM800.print("\n Popytok:"), SIM800.print(count);
        SIM800.print((char)26);                 }
   
    if (heating == true && Timer == 12 ) SMS_send = true; 
    if (Timer > 0 ) Timer--;                                 // если таймер больше ноля  SMS_send = true;
    if (heating == true && Timer <1)    heatingstop(); 
    if (heating == true && Vbat < 11.0) heatingstop(); 
  
    //  Автозапуск при понижении температуры ниже -18 градусов, при -25 смс оповещение каждых 3 часа
    if (Timer2 == 2 && TempDS0 < -18) Timer2 = 1080, Timer = 120, enginestart(3);  
        Timer2--;                                                 // вычитаем еденицу
    if (Timer2 < 0) Timer2 = 1080;                                // продлеваем таймер на 3 часа (60x60x3/10 = 1080)
    if (heating == false) digitalWrite(ACTIV_Pin, HIGH), delay (50), digitalWrite(ACTIV_Pin, LOW);  // моргнем светодиодом
    interval--;
    if (interval <1 && n_send == true) interval = 30, SIM800.println ("AT+CGATT=1"), delay (200);    // подключаемся к GPRS 
    if (interval == 28 && n_send == true ) SIM800.println ("AT+CIPSHUT");    
             
}             

void enginestart(int n_count ) {                                      // программа запуска двигателя
 /*  ----------------------------------------- ПРЕДНАСТРОЙКА ПЕРЕД ЗАПУСКОМ ---------------------------------------------------------*/
    detachInterrupt(1);                                    // отключаем аппаратное прерывание, что бы не мешало запуску
int StTime = map(TempDS0, 20, -15, 1000, 5000);            // при -15 крутим стартером 5 сек, при +20 крутим 1 сек
    StTime = constrain(StTime, 1000, 6000);                // ограничиваем диапазон работы стартера от 1 до 6 сек
    Timer =  map(TempDS0, 50, -25, 30, 150);               // при -25 крутим прогреваем 30 мин, при +50 - 5 мин
    Timer =  constrain(Timer, 30, 180);                    // ограничиваем таймер в зачениях от 5 до 30 минут
    count = 0;                                             // переменная хранящая число попыток запуска
    VbatStart = Vbat;                                      // запоминамем напряжение перед стартом
// если напряжение АКБ больше 10 вольт, зажигание выключено, температуры выше -25 град, счетчик числа попыток  меньше 5

 while (Vbat > 10.00 && digitalRead(Feedback_Pin) == LOW && TempDS0 > -25 && count < n_count){ 
 count++;    // увеличиваем на еденицу число оставшихся потыток запуска   
Serial.print("count="), Serial.print(count),Serial.print(" | StTime ="),Serial.print(StTime);

 digitalWrite(SECOND_P,     LOW),   delay (2000);        // выключаем зажигание на 2 сек. на всякий случай   
 digitalWrite(FIRST_P_Pin, HIGH),   delay (1000);        // включаем реле первого положения замка зажигания 
 digitalWrite(SECOND_P,    HIGH),   delay (4000);        // включаем зажигание  и ждем 4 сек.

// если уже была одна неудачная попытк запуска то прогреваем свечи накаливания 8 сек
 if (count > 2) digitalWrite(SECOND_P, LOW),  delay (2000), digitalWrite(SECOND_P, HIGH), delay (8000);
 
// если уже более чем 3 неудачных попытк запуска то прогреваем свечи накаливания 8 сек дполнительно  
 if (count > 4) digitalWrite(SECOND_P, LOW), delay (10000), digitalWrite(SECOND_P, HIGH), delay (8000);

// если не нажата педаль тормоза то включаем реле стартера на время StTime
    if (digitalRead(STOP_Pin) == LOW) {
                                      digitalWrite(STARTER_Pin, HIGH); // включаем реле стартера
                                      } else {
                                      heatingstop();  
                                      break; 
                                      } 
    
 delay (StTime);                                          // выдерживаем время StTime
 digitalWrite(STARTER_Pin, LOW),    delay (6000);         // отключаем реле, выжидаем 6 сек.

// замеряем напряжение АКБ 3 раза и высчитываем среднее арифмитическое 3х замеров    
 Vbat =        analogRead(BAT_Pin), delay (300);       // замеряем напряжение АКБ 1 раз
 Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  2-й раз 
 Vbat = Vbat + analogRead(BAT_Pin), delay (300);       // через 0.3 сек.  3-й раз
 Vbat = Vbat / m / 3 ;                                 // переводим попугаи в вольты и плучаем срееднне 3-х замеров

// пределяем запустил ся ли двигатель     
// if (digitalRead(DDM_Pin) != LOW)                  // если детектировать по датчику давления масла 
// if (digitalRead(DDM_Pin) != HIGH)                 // если детектировать по датчику давления масла 

 if (Vbat > Vstart) {                                // если детектировать по напряжению зарядки     
                    
                    Serial.print (" -> Vbat > Vstart = "), Serial.println(Vbat); 
                    heating = true, digitalWrite(ACTIV_Pin, HIGH);
                    SIM800.println("ATH0");          // вешаем трубку
                    break;                           // считаем старт успешным, выхдим из цикла запуска двигателя
              
               }else{ 
                
                    Serial.print (" - > Vbat < Vstart = "), Serial.println(Vbat); 
                    StTime = StTime + 200;           // увеличиваем время следующего старта на 0.2 сек.
                    heatingstop();                   // уменьшаем на еденицу число оставшихся потыток запуска
                    }
      }
Serial.println ("out >");
 if (count > 1) SMS_send = true;        // отправляем смс СРАЗУ только в случае незапуска c первой попытки
 if (heating == true) digitalWrite(REL_Pin, HIGH) /*, delay(100),digitalWrite(REL_Pin, LOW)*/; // включаем подогрев седений 
 attachInterrupt(1, callback, FALLING);                    // включаем прерывание на обратный звонок
 }

void webasto() {
    if (heating == true) heatingstop(), delay (10000);
    digitalWrite(WEBASTO_pin, LOW), heating= true, Serial.println ("Webasto ON");               
               }

void heatingstop() {                                   
    digitalWrite(SECOND_P,    LOW),   delay (300);       // выключаем зажигание
    digitalWrite(FIRST_P_Pin, LOW),   delay (300);       // выключаем первое положение замка зажигания       
    digitalWrite(WEBASTO_pin, LOW),   delay (300);       // выключаем вебасто       
    digitalWrite(REL_Pin,     LOW),   delay (300);       // выключаем доп реле 
    digitalWrite(ACTIV_Pin,   LOW),   delay (300);       // гасим светодиод
    Serial.println("Heating Stop"),   delay(3000),          heating= false; 
                   }
