//現在時刻を取得するプログラム　
//参考　https://wak-tech.com/archives/833#i-6
//JSONのあれこれを調べるには下記を使うと良い
//https://arduinojson.org/v6/assistant/
//エポックタイムをUTCに変換するための良い感じのライブラリがないので、変換してくれるAPIで代用する
//https://unixtime.co.za/

//httpのAPIは簡単にGETできる
//httpsのAPIはちょっと大変かも

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
//エポックタイムを変換するため。
//しかしうまく使えなかったAPIでなんとする。
#include <TimeLib.h>

//初期設定の為のアクセスポイント名
#define AP_NAME "test"

//任意場所の経度と緯度
//35.691610, 139.712713
#define API_URL "http://api.open-notify.org/iss-pass.json?lat=35.6918&lon=139.71&alt=10&n=10"
#define API_URL_epock "https://showcase.api.linx.twenty57.net/UnixTime/fromunixtimestamp?unixtimestamp="

//Unixtime to datetime
#define year 31536000
#define day 86400
#define hour 3600
#define second 60

//jsonを扱うための初期化
//下記の数字は大きめにとる
//JSONの値が取れてない時の下記の容量が足りていないことが多い
StaticJsonDocument<600> doc;

//プッシュスイッチ
const int SW = 0;
int valueSW = HIGH;

void clickSw(){
  Serial.println("clickSw");
  valueSW  = digitalRead(SW);
}

//unix time
//UNIX時間とは、UTC時刻における1970年1月1日午前0時0分0秒（UNIXエポック）
//からの経過秒数を計算したもの
//JST(日本時間の場合)、これに9時間足したものになる。

//https://hackaday.io/page/4786-epoch-to-time-date-converter 参考
void unixTimeToDaytime(int unixtime, int timezone_h, int timezone_m){
//  int year_month[12]={0,31,59,90,120,151,181,212,243,274,304,334};
//  int leapyear_month[12]={0,31,60,91,121,152,182,213,244,275,305,335};
  int year_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  int leapyear_month[12]={31,29,31,30,31,30,31,31,30,31,30,31};
  
  int oneYear = 31536000;
  int oneDay = 86400;
  int oneHour = 3600;
  int oneMinute = 60;
  
  int fixedUnixTime = unixtime + timezone_h * 60 * 60 + timezone_m * 60;
  int ElapsedSecond = fixedUnixTime % 60;
  fixedUnixTime /= 60;
  int ElapsedMinute = fixedUnixTime % 60;
  fixedUnixTime /= 60;
  int ElapsedHours = fixedUnixTime % 24;
  fixedUnixTime /= 24;

  int ElapsedDays = fixedUnixTime;

  int ElapsedYears = 1970+(ElapsedDays/365);

  //1970年からの閏年を計算
  int i;
  int leap_days = 0;
  for(i=1972; i<ElapsedYears; i+=4){
    if(((i%4==0)&&(i%100!=0)) || (i%400==0)){
      leap_days++;
    }
  }

  int fixedElapsedYears = 1970+((ElapsedDays-leap_days)/365);
  //0日は何から1を足している。
  int day_of_year = ((ElapsedDays-leap_days)%365)+1;
  int temp_days;
  if(((fixedElapsedYears%4==0)&&(fixedElapsedYears%100!=0)) || (fixedElapsedYears%400==0)){
      year_month[1]=29;
      int leap_year_ind = 1;
  }else{
    year_month[1]=28;
    temp_days=0;
  }
  
  int ElapsedMonths;
  int month_days;
  for(ElapsedMonths=0; ElapsedMonths<=11; ElapsedMonths++){
    if(day_of_year<=temp_days){
      break;
    }
    temp_days = temp_days + year_month[ElapsedMonths];
  }
  temp_days = temp_days - year_month[ElapsedMonths-1]; 
  int ElapsedDate = day_of_year - temp_days;

  Serial.println("ElapsedYears : ");
  Serial.println(ElapsedYears);
  Serial.println("ElapsedMonths : ");
  Serial.println(ElapsedMonths);
  Serial.println("ElapsedDate : ");
  Serial.println(ElapsedDate);
//  Serial.println("ElapsedDays : ");
//  Serial.println(ElapsedDays);
  Serial.println("ElapsedHours : ");
  Serial.println(ElapsedHours);
  Serial.println("ElapsedMinute : ");
  Serial.println(ElapsedMinute);
  
//  int ElapsedYears = fixedUnixTime / oneYear;
//  int temp = fixedUnixTime % oneYear;
//  int ElapsedDays = temp / oneDay;
//  temp = temp % oneDay;
//  int ElapsedHours = temp / oneHour;
//  temp = temp % oneHour;
////  Serial.println("temp is : ");
////  Serial.println(temp);
//  int ElapsedMinute = temp / oneMinute;
//  Serial.println("ElapsedYears : ");
//  Serial.println(ElapsedYears);
//  Serial.println("ElapsedDays : ");
//  Serial.println(ElapsedDays);
//  Serial.println("ElapsedHours : ");
//  Serial.println(ElapsedHours);
//  Serial.println("ElapsedMinute : ");
//  Serial.println(ElapsedMinute);

//  int i = 1;
//  if (ElapsedDays%4 == 0){
//    if (ElapsedDays%100 == 0){
//      if(ElapsedDays%400 == 0){
//        Serial.println("leap year");
//        while(ElapsedDays > 31){
//          ElapsedDays = ElapsedDays - leapyear_month[i];
//          i += 1;
//        }
//      }else{
//        Serial.println("normal year"); 
//        while(ElapsedDays > 31){
//          ElapsedDays = ElapsedDays - year_month[i];
//          i += 1;
//        }   
//      }
//    }else{
//      Serial.println("leap year");
//      while(ElapsedDays > 31){
//      ElapsedDays = ElapsedDays - leapyear_month[i];
//      i += 1;
//      }
//    }
//  }else{
//    Serial.println("normal year");
//    while(ElapsedDays > 31){
//      ElapsedDays = ElapsedDays - year_month[i];
//      i += 1;
//    }
//  }
//  Serial.println("Month : ");
//  Serial.println(i);
//  Serial.println("day : ");
//  Serial.println(ElapsedDays);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect(AP_NAME)) {
    Serial.println("timeout");
    delay(3000);
    //初期設定が成功シない場合 reset して初期設定をリトライする
    ESP.restart();
    delay(5000);
    return;
  }
  //ルーターとの接続に成功
  Serial.println("connected...");
  delay(1000);

  //ボタン割り込み設定
  pinMode(SW, INPUT_PULLUP);
  attachInterrupt(SW, clickSw, CHANGE );

  //正確な時刻の取得
  ////NTPの設定
  configTime(9*3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");

}

//時刻を書くのするオブジェクト
struct tm timeInfo;
//文字格納
char s[20];

void loop() {
  if(valueSW == LOW){
    HTTPClient http;
    http.begin(API_URL);
    int httpCode = http.GET();

    //返答がある場合
    if(httpCode > 0){
      String payload = http.getString();
      //char json[] = payload;
      //JsonObject object = doc.as();
      deserializeJson(doc, payload);
      //Serial.println(httpCode);
      //Serial.println(payload);

      const char* message = doc["message"]; // "success"
      JsonObject request = doc["request"];
      int request_altitude = request["altitude"]; 
      Serial.println(request_altitude);
      
      //JSONがさらに入れ子上になっているので、まずJSON Arrayに格納する
      //そこから２次元配列の要領で引き出す
      JsonArray response = doc["response"];
      int response_0_duration = response[0]["duration"]; // 439
      long response_0_risetime = response[0]["risetime"]; // 1596505697 
      Serial.print("response_0_duration : ");
      Serial.println(response_0_duration);
      Serial.print("response_0_risetime : ");
      Serial.println(response_0_risetime);

      unixTimeToDaytime(response_0_risetime, 9, 0);

//      char risetime =  unixTimeToDaytime(response_0_risetime);
//      Serial.println(risetime);
      HTTPClient https;
      String API_URL_epock2 = API_URL_epock + String(response_0_risetime);
      https.begin("https://showcase.api.linx.twenty57.net/UnixTime/fromunixtimestamp?unixtimestamp=1549892280");
      int httpCode2 = https.GET();

      //返答がある場合
      if(httpCode2 > 0){
        String payload2 = https.getString();
        deserializeJson(doc, payload2);
  //      const char* message = payload;
        Serial.println("Risetime is : ");
        Serial.println(payload2);
        const char* message = doc["Datetime"];
      }else{
        Serial.println("None");
      }
  

      
    }else{
      Serial.println("Error");
    }
  }


  //正確な現在時刻を取得
  getLocalTime(&timeInfo);
  sprintf(s, " %04d/%02d/%02d %02d:%02d:%02d",
          timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
          timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);//人間が読める形式に変換
  delay(1000);
  Serial.println(s);//時間をシリアルモニタへ出力
  delay(1000);

}
