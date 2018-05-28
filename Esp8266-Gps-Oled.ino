#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "SH1106.h"
#include "FS.h"

/*----------------------------------------------------------------------------*/
// Gps
/*----------------------------------------------------------------------------*/
static const int RXPin = D6, TXPin = D5;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;

SoftwareSerial ss(RXPin, TXPin);

void init_gps()
{
  ss.begin(GPSBaud);

  Serial.println(F("Esp8266-Gps-Oled.ino"));
  Serial.println(F("Prueba de concepto de GPS + OLED"));
  Serial.print(F("TinyGpsPlus Ver: "));
  Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("por Jose Figueredo"));
  Serial.println();
  Serial.println(F("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum"));
  Serial.println(F("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail"));
  Serial.println(F("----------------------------------------------------------------------------------------------------------------------------------------"));
}

void print_gps_to_serial()
{
  printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
  printFloat(gps.hdop.hdop(), gps.hdop.isValid(), 6, 1);
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
  printInt(gps.location.age(), gps.location.isValid(), 5);
  printDateTime(gps.date, gps.time);
  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
  printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
  printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
  printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.deg()) : "*** ", 6);

  printInt(gps.charsProcessed(), true, 6);
  printInt(gps.sentencesWithFix(), true, 10);
  printInt(gps.failedChecksum(), true, 9);
  Serial.println();
}

/*----------------------------------------------------------------------------*/
// Oled
/*----------------------------------------------------------------------------*/
SH1106 display(0x3c, D1, D2);

int screenW = 128;
int screenH = 32;
int centerX = screenW / 2;
int centerY = ((screenH - 16) / 2) + 16;

// http://4umi.com/web/javascript/xbm.php?n=x&width=32&height=32&code=hex&color=gold&tile=none&zoom=2&c=000000000000000000000000000240000001800000118800001188008008110180c81101808813018088110100918900009189000081810000c24300004002000040020000400200006006000020040000200400002004000020040000300c0000500800009008000090090000581a000028140000181800000c300000000000
const uint8_t antena_bits[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x40, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x11, 0x88, 0x00,
    0x00, 0x11, 0x88, 0x00, 0x80, 0x08, 0x11, 0x01, 0x80, 0xc8, 0x11, 0x01,
    0x80, 0x88, 0x13, 0x01, 0x80, 0x88, 0x11, 0x01, 0x00, 0x91, 0x89, 0x00,
    0x00, 0x91, 0x89, 0x00, 0x00, 0x81, 0x81, 0x00, 0x00, 0xc2, 0x43, 0x00,
    0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
    0x00, 0x60, 0x06, 0x00, 0x00, 0x20, 0x04, 0x00, 0x00, 0x20, 0x04, 0x00,
    0x00, 0x20, 0x04, 0x00, 0x00, 0x20, 0x04, 0x00, 0x00, 0x30, 0x0c, 0x00,
    0x00, 0x50, 0x08, 0x00, 0x00, 0x90, 0x08, 0x00, 0x00, 0x90, 0x09, 0x00,
    0x00, 0x58, 0x1a, 0x00, 0x00, 0x28, 0x14, 0x00, 0x00, 0x18, 0x18, 0x00,
    0x00, 0x0c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00};

void init_oled()
{
  display.init();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(centerX, centerY, "Iniciando...");
  display.display();
}

void activate_oled()
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 12, "Buscando");
  display.drawString(0, 36, "satélites...");
  display.drawXbm(screenW - 42, centerY / 2, screenH, screenH, antena_bits);
  display.display();
}

void print_oled(String msgA, String msgB, String msgC)
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(centerX, 0, msgA);
  display.drawString(centerX, 24, msgB);
  display.drawString(centerX, 48, msgC);
  display.display();
  yield();
}

char buff[10];
float latF = 0.0f;
float lngF = 0.0f;
String lat = "";
String lng = "";

void oled_print_gps_coords()
{
  latF = gps.location.lat();
  lngF = gps.location.lng();

  if (latF != 0 && lngF != 0)
  {
    lat = dtostrf(latF, 4, 6, buff);
    lng = dtostrf(lngF, 4, 6, buff);
    print_oled(F("Coordenadas:"), lat, lng);
  }
}

/*----------------------------------------------------------------------------*/
// FS
/*----------------------------------------------------------------------------*/
unsigned long startMillis = 0;
unsigned long currentMillis;
const unsigned long period = 15 * 1000;
String file_name = "/f.txt";
String dateTime = "";

void fs_init()
{
  SPIFFS.begin();
}

void fs_format()
{
  SPIFFS.format();
  Serial.println("Spiffs formateado");
}

void fs_read_file()
{
  // Serial.println(">> fs_read_file");
  File f = SPIFFS.open(file_name, "r");
  if (!f)
  {
    Serial.println("Error al abrir el archivo para leer");
  }
  else
  {
    Serial.println("Contenido del archivo: ");
    while (f.available())
    {
      String line = f.readStringUntil('\n');
      Serial.print(line);
    }
    Serial.println("EOF");
    f.close();
  }
  // Serial.println("<< fs_read_file");
}

void fs_print_gps_coords()
{
  // Serial.println(">> fs_print_gps_coords");
  currentMillis = millis();
  if (currentMillis - startMillis >= period && gps.satellites.isValid() && gps.satellites.value() > 0)
  {
    File f = SPIFFS.open(file_name, "a");
    Serial.println("-- period");
    if (f)
    {
      print_gps_to_serial();
      if (latF != 0 && lngF != 0)
      {
        printDateTimeToString(gps.date, gps.time);
        String line = dateTime + "," + lat + "," + lng + ";";
        Serial.print("-- ");
        Serial.println(line);
        f.println(line);
      }
      f.close();
    }
    startMillis = currentMillis;
  }
  // Serial.println("<< fs_print_gps_coords");
}

/*----------------------------------------------------------------------------*/
// Setup
/*----------------------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);
  // Serial.println(">> setup");
  // fs_format();
  fs_init();
  init_gps();
  yield();
  init_oled();
  delay(1000);
  fs_read_file();
  delay(1000);
  activate_oled();
  // Serial.println("<< setup");
}

/*----------------------------------------------------------------------------*/
// Loop
/*----------------------------------------------------------------------------*/
void loop()
{

  // print_gps_to_serial();

  oled_print_gps_coords();

  fs_print_gps_coords();

  gps_delay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No hay datos GPS: chequear los cables"));
  }
}

/*----------------------------------------------------------------------------*/
// Helpers
/*----------------------------------------------------------------------------*/
static void gps_delay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');
  }
  gps_delay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  Serial.print(sz);
  gps_delay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
  }

  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  gps_delay(0);
}

static void printDateTimeToString(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    dateTime = "00/00/00-";
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d-", d.year(), d.month(), d.day());
    dateTime = sz;
  }

  if (!t.isValid())
  {
    dateTime = dateTime + "00:00:00";
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d`1 q", t.hour(), t.minute(), t.second());
    dateTime = dateTime + sz;
  }

  Serial.println("dateTime: " + dateTime);
  gps_delay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
    Serial.print(i < slen ? str[i] : ' ');
  gps_delay(0);
}
