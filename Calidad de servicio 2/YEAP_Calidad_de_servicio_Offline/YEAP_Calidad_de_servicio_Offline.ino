#include <CayenneLPP.h>
#include <RTCZero.h>
#include <string.h>
#include <LoRa.h> 
#include <SerialRAM.h>

#define debugSerial SerialUSB
#define loraSerial  Serial1
#define loraReset 4
#define BUZZER_PIN 24
#define RESPONSE_LEN  100
#define NO_OFFLINE_DATA 255

uint8_t frameReceived[255];

#define RAM_Button1 0x100
#define RAM_Button2 0x101
#define RAM_Button3 0x102
#define RAM_Button4 0x103

SerialRAM ram;

// Variables
static const char hex[] = { "0123456789ABCDEF" };
static char response[RESPONSE_LEN];
CayenneLPP lpp(51);
static enum {
  INIT = 0,
  JOIN, JOINED, GET_BUTTON, SEND_BUTTON, SEND_PAYLOAD,SEND_KEEP_ALIVE,SEND_OFFLINE_DATA
} state;

#define button1 36  // J6-1
#define button2 34  // J6-2
#define button3 37  // J6-3
#define button4 35  // J6-4

/* Create an rtc object */
RTCZero rtc;

/* Change these values to set the current initial time */
unsigned char seconds = 0;
unsigned char minutes = 0;
unsigned char hours = 0;

/* Change these values to set the current initial date */
const byte day = 15;
const byte month = 6;
const byte year = 15;

/*
 * Vacia el buffer de recepcion desde el modulo LoRa
 */
void loraClearReadBuffer()
{
  while(loraSerial.available())
    loraSerial.read();
}

/*
 * Espera la respuesta del modulo LoRa y la imprime en el puerto de debug
 */
void loraWaitResponse(int timeout)
{
  size_t read = 0;

  loraSerial.setTimeout(timeout);
  
  read = loraSerial.readBytesUntil('\n', response, RESPONSE_LEN);
  if (read > 0) {
    response[read - 1] = '\0'; // set \r to \0
    debugSerial.println(response);
  }
  else
    debugSerial.println("Response timeout");
}

/*
 * Envia un comando al modulo LoRa y espera la respuesta
 */
void loraSendCommand(char *command)
{
  loraClearReadBuffer();
//Debug
//  debugSerial.println(command);
  loraSerial.println(command);
  loraWaitResponse(1000);
}

/*
 * Transmite un mensaje de datos por LoRa
 */
 
void loraSendData(int port, char *data, uint8_t dataSize)
{
  char cmd[100];
  char *p;
  int i;
  uint8_t c;

  sprintf(cmd, "mac tx cnf %d ", port);

  p = cmd + strlen(cmd);
  
  for (i=0; i<dataSize; i++) 
  {
    *p++ = *data++;
  }
  *p++ = 0;

  loraSendCommand(cmd);
  
  if (strcmp(response, "ok") == 0)
    loraWaitResponse(20000);
}

void pinStr( uint32_t ulPin, unsigned strength) // works like pinMode(), but to set drive strength
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN )
  {
    return ;
  }
  if(strength) strength = 1;      // set drive strength to either 0 or 1 copied
  PORT->Group[g_APinDescription[ulPin].ulPort].PINCFG[g_APinDescription[ulPin].ulPin].bit.DRVSTR = strength ;
}

void setup()
{
  pinMode(PIN_LED_13, OUTPUT);
  while (!debugSerial && (millis() < 10000)) ;
  {
    digitalWrite( PIN_LED_13, LOW );
    delay(1000);
    digitalWrite( PIN_LED_13, HIGH );
    delay(1000);
  }
  debugSerial.begin(115200) ;
  loraSerial.begin(57600) ;
  pinMode(loraReset, OUTPUT);
  digitalWrite(loraReset, LOW);
  delay(1000);
  digitalWrite(loraReset, HIGH);

  pinMode(BUZZER_PIN, OUTPUT);
  pinStr(BUZZER_PIN,HIGH);
  pinMode(25, OUTPUT);
  pinStr(25,HIGH);
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);
  pinMode(button4, INPUT_PULLUP);

 // LoRa
  LoRa.hwInit() ;
  loraSerial.begin(LoRa.getDefaultBaudRate()) ;
  LoRa.initLoRaStream(loraSerial) ;
  LoRa.swInit() ;

  ram.begin();
  
  blinkLED( 100, 100, 3);
  pulseBuzzer(100,100,2);

  
  debugSerial.println("Calidad de Servicio - Version OFFLINE");
  delay(1000);
  loraSerial.println("sys reset");
  delay(1000);
  rtc.begin(); // initialize RTC

  // Set the time
  rtc.setHours(hours);
  rtc.setMinutes(minutes);
  rtc.setSeconds(seconds);

  // Set the date
  rtc.setDay(day);
  rtc.setMonth(month);
  rtc.setYear(year);
  state = GET_BUTTON;
} 

char val, debounce;
unsigned long timeDebounce, timeLastJoin = 0, timeKeepAlive = 0, timeSendOfflineData = 0;
String payload;
char payload_array[100];
char now_min, now_sec, current_channel;
char now_hour, ErrorVar, payload_size_val;
char *response_ptr;


void loop()
{

      if( debounce && ((millis() - timeDebounce)>2000) )
      {
        debounce = 0;
      }

      if( debugSerial.read() == '&' )
      {
        debugSerial.println("************************************");
        debugSerial.println("**           BYPASS MODE          **");
        debugSerial.println("************************************");
        
        
        while(1)
        {
           if (debugSerial.available()) {
        loraSerial.write(debugSerial.read());}
        
      if (loraSerial.available()) {
        debugSerial.write(loraSerial.read());}
      
        }
      }
          
/*      if (debugSerial.available()) {
        loraSerial.write(debugSerial.read());}
        
      if (loraSerial.available()) {
        debugSerial.write(loraSerial.read());}
*/

     static int i, dr;
     static char one_pass = 0;
     
  switch (state) {
    case INIT: 
      debugSerial.println("Reinicializacion...");
      digitalWrite(loraReset, LOW);
      delay(1000);
      digitalWrite(loraReset, HIGH);
      delay(1000);
      loraSendCommand("sys reset");
      loraSendCommand("sys get hweui");
      loraSendCommand("mac set appeui 1111111111111111");
      loraSendCommand("mac set appkey 11111111111111111111111111111111");
      loraSendCommand("mac set adr on");
      loraSendCommand("mac set class c");
      state = JOIN;
      break;
 
    case JOIN:
      loraSendCommand("mac join otaa");
      loraWaitResponse(10000);
      if (strcmp(response, "accepted") == 0) {
        loraSendCommand("mac get adr");
        i = 0;
        state = SEND_PAYLOAD;
        timeLastJoin = millis();
      }
      break;

    case SEND_KEEP_ALIVE:

      sendKeepAlive();

      timeKeepAlive = millis();
      current_channel = 2;
      state = SEND_PAYLOAD;
      break;

    case GET_BUTTON:

      // Si nunca se envió el keep alive o si pasaron 30 min desde el último, envio un nuevo Keep Alive
      if( !timeKeepAlive || ((millis() - timeKeepAlive) > 1800000) )
      {
        state = SEND_KEEP_ALIVE;
        break;
      }

      
      if( !timeSendOfflineData || ((millis() - timeSendOfflineData) > 60000) )    // Manda los datos offline (si los hay) cada 1 minuto
      {
        debugSerial.println("Envia datos offline");
        state = SEND_OFFLINE_DATA;
        break;
      }

       if( check_LoRaRX() )
       {
          if( strstr (response,"mac_rx") != NULL )
          {
            response_ptr = strstr (response,"mac_rx");
          
            debugSerial.println(response_ptr[7]);
            if (response_ptr[7] == '6') 
            {
              hours = ((response_ptr[9] - '0')*10) + (response_ptr[10] - '0');
              minutes = ((response_ptr[11] - '0')*10) + (response_ptr[12] - '0');
              seconds = ((response_ptr[13] - '0')*10) + (response_ptr[14] - '0');
              debugSerial.println("Nueva hora recibida");
              
              // Set the time
              rtc.setHours(hours);
              rtc.setMinutes(minutes);
              rtc.setSeconds(seconds);
            }
          }
       }

      if( getButton_State() )
      {
        pulseBuzzer(100,1,1);
        debugSerial.println( (unsigned long) (millis() - timeSendOfflineData), DEC );
        
      }

      break;
    case SEND_OFFLINE_DATA:
      
      timeSendOfflineData = millis();

      if( AllButtonPayload() )
      {
        debugSerial.println("Hay datos para enviar");
        current_channel = 18;
        state = SEND_PAYLOAD;
      }
      else
        state = GET_BUTTON;
      break;
      
    case SEND_PAYLOAD:
        debugSerial.print("\r\nEnviando datos... >> ");
        
        for( i = 0; i <= payload_size_val; i++)
        {
          debugSerial.print(String(payload[i], DEC ));
          debugSerial.write(',');
        }
          
        debugSerial.print("\r\nCanal: ");
        debugSerial.print(current_channel, DEC);

//        payload.toCharArray( payload_array, sizeof(payload) );
//        loraSendData(current_channel, payload_array, sizeof(payload));
        
        ErrorVar = LoRa.sendReqAck(current_channel, (const uint8_t*)payload_array, payload_size_val, 5 );
        
        debugSerial.print("\r\nTransmitido - ErrorVar = ");
        debugSerial.print(ErrorVar, DEC);
        switch ( ErrorVar )
        {
          case NoError:
                        debugSerial.println("Datos enviados");
                        blinkLED( 150, 50, 2);
                        
                         /*-- Recepcion de comando --*/
                        
                        ErrorVar =LoRa.receive((uint8_t*) frameReceived, 4);
                        
                        if (ErrorVar) 
                        {
                          debugSerial.println("\r\nSe recibio un paquete desde la red LoRa!");
                          for (i=0;i<4;i++) 
                          {
                            debugSerial.print( frameReceived[i], HEX );
                          }
            
                          if( frameReceived[0] == 0x55 )
                          {
                            hours = frameReceived[1];
                            minutes = frameReceived[2];
                            seconds = frameReceived[3];
                            debugSerial.println("Nueva hora recibida");
                            
                            // Set the time
                            rtc.setHours(hours);
                            rtc.setMinutes(minutes);
                            rtc.setSeconds(seconds);
                          }
                        } 

                        if( current_channel == 18 )
                        {
                          clearButtonCounters();
                        }
                        
                        state = GET_BUTTON;
                        break;

          case NoResponse:
          case InternalError:
          case NetworkFatalError:
          case Timeout:
          case NotConnected:
                        debugSerial.println("Re - Inicio");
                        state = INIT;
                        break;

          case PayloadSizeError:
                        loraSendCommand("mac tx uncnf 4 00");
                        debugSerial.println("Dummy Byte sent");
                        delay(2000);
                        break;

          case Busy:
                        debugSerial.println("Busy");
                        delay(2000);
                        break;

          case NoAcknowledgment:
                        debugSerial.println("No ACK");
                        state = GET_BUTTON;
                        break;
        }
      break;
  }
  
}


char getButton_State( void )
{
      unsigned char gbs_counter;
      
      if( !debounce )
      {
        if( (digitalRead( button1 )== LOW) )
        {
          gbs_counter = ram.read( RAM_Button1 );
          ram.write( RAM_Button1, gbs_counter + 1);
          timeDebounce = millis();
          debounce = 1;
          debugSerial.println("Boton 1");
          return 1;
        }
        else
        {
          if( (digitalRead( button2 )== LOW) )
            {
              gbs_counter = ram.read( RAM_Button2 );
              ram.write( RAM_Button2, gbs_counter + 1);
              timeDebounce = millis();
              debounce = 1;
          debugSerial.println("Boton 2");
              return 1;
             }
          else
          {
            if( (digitalRead( button3 )== LOW) )
              {
                gbs_counter = ram.read( RAM_Button3 );
                ram.write( RAM_Button3, gbs_counter + 1);
                timeDebounce = millis();
                debounce = 1;
          debugSerial.println("Boton 3");
                return 1;
              }
            else
            {
              if( (digitalRead( button4 )== LOW) )
              {
                  gbs_counter = ram.read( RAM_Button4 );
                  ram.write( RAM_Button4, gbs_counter + 1);
                  timeDebounce = millis();
                  debounce = 1;
          debugSerial.println("Boton 4");
                  return 1;
              }
              else
              {
                return 0;
              }
            }
          }
        }
      } // End if( !debounce)

      return 0;
}

bool check_LoRaRX( void )
{
  if( loraSerial.available() )
  {
    size_t read = 0;
  
    loraSerial.setTimeout(2000);
    read = loraSerial.readBytesUntil('\n', response, RESPONSE_LEN);
    if (read > 0) 
    {
      response[read - 1] = '\0'; // set \r to \0
      debugSerial.println(response);
      return 1;
    }
  }
  return 0;
}

void blinkLED( uint8_t ton, uint8_t toff, uint8_t count )
{
  do
  {
    digitalWrite( PIN_LED_13, LOW);
    delay( ton );
    digitalWrite( PIN_LED_13, HIGH);
    delay( toff );
    count--;
  }while( count );
}

void pulseBuzzer( uint8_t ton, uint8_t toff, uint8_t count )
{
  do
  {
    digitalWrite( BUZZER_PIN, HIGH);
    delay( ton );
    digitalWrite( BUZZER_PIN, LOW);
    delay( toff );
    count--;
  }while( count );
}

void sendKeepAlive (void)
{
  unsigned long timer_temporal;
  unsigned char temp_dia, temp_hora, temp_min;
  
  timer_temporal = millis();

  temp_dia = timer_temporal / 86400000;
  temp_hora = ((timer_temporal % 86400000) / 3600000);
  temp_min = (( (timer_temporal % 86400000) % 3600000) / 60000);

  payload_array[0] = temp_dia;
  payload_array[1] = temp_hora;
  payload_array[2] = temp_min;
  payload_size_val = 3;
/*  
  if( temp_dia < 10 )
  {
    payload = String( 0, DEC);
    payload.concat( String( temp_dia, DEC )); // Dias
  }
  else
    payload = String( temp_dia, DEC ); // Dias

  if( temp_hora < 10 )
    payload.concat( String( 0, DEC ) );
  
  payload.concat( String( temp_hora , DEC) );
  
  if( temp_min < 10 )
    payload.concat( String( 0, DEC ) );
    
  payload.concat( String( temp_min , DEC) );
*/
}

char getOfflineData ( void )
{
  unsigned char sendOfflineData_val, ram_data1,ram_data2,ram_data3,ram_data4;
  unsigned char sod_hour = 0, sod_min = 0; 
  
  for( sendOfflineData_val = 0; sendOfflineData_val <= 188; sendOfflineData_val += 4 )
  {
    
    ram_data1 = ram.read( sendOfflineData_val );      // Leo datos del Boton 1
    ram_data2 = ram.read( sendOfflineData_val + 1 );  // Leo datos del Boton 2
    ram_data3 = ram.read( sendOfflineData_val + 2 );  // Leo datos del Boton 3
    ram_data4 = ram.read( sendOfflineData_val + 3 );  // Leo datos del Boton 4

    if( ram_data1 || ram_data2 || ram_data3 || ram_data4 )
    {
      // Si alguno de los botones tenia datos en esa hora
      
      insertTimePayload( sod_hour, sod_min, 0 );  // Inserto la hora en el payload

      // Y agrego el boton en cuestion
      if( ram_data1 )
      {
        payload.concat( "01" );
        return (sendOfflineData_val);
      }

      if( ram_data2 )
      {
        payload.concat( "02" );
        return (sendOfflineData_val + 1);
      }

      if( ram_data3 )
      {
        payload.concat( "03" );
        return (sendOfflineData_val + 2);
      }

      if( ram_data4 )
      {
        payload.concat( "04" );
        return (sendOfflineData_val + 3);   // con el dato de la direccion, puedo borrar la posicion una vez trasnmitido el mensaje
      }      
    }

    if( sod_min == 30 )
    {
      sod_min = 0;
      sod_hour++;
    }
    else
      sod_min = 30;
      
  }

  return NO_OFFLINE_DATA;    
}

char AllButtonPayload ( void )
{
  unsigned char abp_val, ram_data1,ram_data2,ram_data3,ram_data4;
  unsigned char sod_hour = 0, sod_min = 0; 
  
  ram_data1 = ram.read( RAM_Button1 );  // Leo datos del Boton 1
  ram_data2 = ram.read( RAM_Button2 );  // Leo datos del Boton 2
  ram_data3 = ram.read( RAM_Button3 );  // Leo datos del Boton 3
  ram_data4 = ram.read( RAM_Button4 );  // Leo datos del Boton 4

  if( ram_data1 || ram_data2 || ram_data3 || ram_data4 )
  {
    // Y agrego el boton en cuestion
    payload_array[0] = ram_data1;
    payload_array[1] = ram_data2;
    payload_array[2] = ram_data3;
    payload_array[3] = ram_data4;
    payload_size_val = 4;
      
/*    
    payload = String( ram_data1, HEX );
    payload.concat( String( ram_data2, HEX ) );
    payload.concat( String( ram_data3, HEX ) );
    payload.concat( String( ram_data4, HEX ) );

    if( ram_data1 < 0x0F )
    {
      payload = String( 0, HEX );
      payload.concat( String(ram_data1, HEX) );
    }
    else
      payload = String( ram_data1, HEX );
    
    if( ram_data2 < 0x0F )
    {
      payload.concat( String(0, HEX) );
    }         

    payload.concat( String(ram_data2 , HEX) );
    
    if( ram_data3 < 0x0F )
    {
      payload.concat( String(0, HEX) );
    }         

    payload.concat( String(ram_data3 , HEX) );

    if( ram_data4 < 0x0F )
    {
      payload.concat( String(0, HEX) );
    }         

    payload.concat( String(ram_data4 , HEX) );
*/
    return 1;
  } 
  else
    return 0;    
}


void insertTimePayload( unsigned char itp_hour, unsigned char itp_min, unsigned char itp_sec)
{
    if( itp_hour < 10 )
    {
      payload = String( 0, DEC );
      payload.concat( String(itp_hour, DEC) );
    }
    else
      payload = String( itp_hour, DEC );
    
    if( itp_min < 10 )
    {
      payload.concat( String(0, DEC) );
    }         

    payload.concat( String(itp_min, DEC) );
    
    if( itp_sec < 10 )
    {
      payload.concat( String(0, DEC) );
    }         

    payload.concat( String(itp_sec, DEC) );
}

void clearButtonCounters( void )
{
  ram.write( RAM_Button1, 0x00 );
  ram.write( RAM_Button2, 0x00 );
  ram.write( RAM_Button3, 0x00 );
  ram.write( RAM_Button4, 0x00 );
}


