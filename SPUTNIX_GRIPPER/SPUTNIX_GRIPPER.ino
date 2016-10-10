#include <Servo.h>

#define RESET_PIN 12 // Reset-пин
#define SERVO_PIN 10 // Пин управляющий сервой
#define ANG_OPEN 177 // Угол открытия культяпы
#define ANG_CLOSED 63 // Угол закрытия культяпы
#define BUFF_SIZE 256 // Размер буффера принимаемых данных
#define DRIVER_ENABLE 9 // Пин управления преобразователем RS485/RS232
#define RECEIVER_ENABLE 8 // Пин управления преобразователем RS485/RS232

// ********************************************************************************************
// Defines connected with protocol

#define MANIPULATOR_NUMBER 1 // Порядковый номер манипулятора 

// Adresses
#define OBC_ADDRESS 0x0001
#define MISC_ARM_ADDRESS(n) (0x0270 + n) // Адрес 4 модулей манипулятора

// Message types
#define MISC_BASE 0x2000 // Базовый адрес сообщений касающихся передатчиков и полезных нагрузок
#define MISC_ARM_BASE (MISC_BASE + 0x0700) // Базовый адрес сообщений касающихся 4 модулей манипулятора

/* 1 Команда установить значение регистра состояния модуля манипулятора, длинна 1
   int8_t value */
#define MISC_ARM_CMD_SET_STATE(n)                     (MISC_ARM_BASE + 0x0000 + n) //0x2700

/* Запрос на перезагрузку модуля манипулятора, длинна - 0x00 */
#define MISC_ARM_CMD_RESET(n)                         (MISC_ARM_BASE + 0x0010 + n) //0x2710

/* Ответ с подтверждением состояния модуля манипулятора, длинна - 0x01 */
#define MISC_ARM_RESP_STATE(n)                        (MISC_ARM_BASE + 0x0020 + n) //0x2720 

// ********************************************************************************************

struct packet // Структура принимаемых/передаваемых пакетов
{
  uint16_t type = 0;
  uint16_t to = 0;
  uint16_t from = 0;
  uint8_t data = 0;
};

packet recieved_packet;
char incomingBytes[BUFF_SIZE] = {0};
char outcomingBytes[BUFF_SIZE] = {0};
char decodedBytes[BUFF_SIZE] = {0};
char encodedBytes[BUFF_SIZE] = {0};
uint16_t buff_ptr = 0; // Счетчик принятых байт

Servo gripper;

void setup()
{
  digitalWrite(RESET_PIN, HIGH); // Подтягиваем reset к +5V
  delay(200);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(DRIVER_ENABLE, OUTPUT);
  digitalWrite(DRIVER_ENABLE, HIGH); // Разрешаем передачу данных
  pinMode(RECEIVER_ENABLE, OUTPUT);
  digitalWrite(RECEIVER_ENABLE, LOW); // Разрешаем прием данных
  gripper.attach(SERVO_PIN);
  gripper.write(ANG_OPEN); // Открываем gripper при старте системы
  Serial1.begin(115200); // Инициализируем обмен со скоростью 115200 бод
  Serial1.flush(); // Очищаем буфер USART
  while (!Serial1) { // Ждем пока пройдет инициализация обмена
    ;
  }
}

void loop()
{
  if (Serial1.available() > 0) // Если есть данные в буфере
  {
    incomingBytes[buff_ptr++] = Serial1.read(); // Читаем по одному байту
    if (buff_ptr > BUFF_SIZE) buff_ptr = 0; // Если достигли конца буфера, начинаем записывать с начала
    if  (incomingBytes[buff_ptr - 1] == 0x00) // Если считали весь пакет
    {
      uint8_t numDecoded = decodeCOBS(incomingBytes, buff_ptr, decodedBytes); // Декодируем данные
      recieved_packet.type = (uint16_t)(decodedBytes[0] << 8 | decodedBytes[1]);
      recieved_packet.to = (uint16_t)(decodedBytes[2] << 8 | decodedBytes[3]);
      recieved_packet.from = (uint16_t)(decodedBytes[4] << 8 | decodedBytes[5]);
      //
      //      Serial.print("Type = 0x");
      //      Serial.println(recieved_packet.type, HEX);
      //      Serial.print("To = 0x");
      //      Serial.println(recieved_packet.to, HEX);
      //      Serial.print("From = 0x");
      //      Serial.println(recieved_packet.from, HEX);
      //      Serial.print("Number of decoded bytes = ");
      //      Serial.println(numDecoded);
      //      Serial.println("Decoded message:");
      //
      //      for (int i = 0; i < numDecoded - 1; i++)
      //      {
      //        Serial.print("i = ");
      //        Serial.println(i);
      //        Serial.print(decodedBytes[i], HEX);
      //        Serial.println(" ");
      //      }

      switch (recieved_packet.type) // Проверяем тип принятого сообщения
      {
        case MISC_ARM_CMD_SET_STATE(MANIPULATOR_NUMBER): // Если принято сообщение об изменении состояния захвата
          {
            if ((recieved_packet.to == MISC_ARM_ADDRESS(MANIPULATOR_NUMBER)) || (recieved_packet.to == 0xFFFF)) // Проверяем, что сообщение адресованно нам
            {
              recieved_packet.data = (uint8_t) decodedBytes[6];
              if (recieved_packet.data == 1) // Запрос на открытие захвата
              {
                gripper.write(ANG_OPEN);
                //                Serial.println();
                //                Serial.println("Gripper is open");
              }
              else // Запрос на закрытие захвата
              {
                gripper.write(ANG_CLOSED);
                //                Serial.println();
                //                Serial.println("Gripper is closed");
              }
              // Формируем ответное сообщение
              // Переписать, спользуя define (!!!или используя структуру пакета !!!)
              outcomingBytes[0] = 0x27;
              outcomingBytes[1] = 0x20 + MANIPULATOR_NUMBER;
              outcomingBytes[2] = 0x00;
              outcomingBytes[3] = 0x01;
              outcomingBytes[4] = 0x02;
              outcomingBytes[5] = 0x70 + MANIPULATOR_NUMBER;
              outcomingBytes[6] = recieved_packet.data;

              //            for (int i = 0; i <= 6; i++)
              //            {
              //              Serial.print(outcomingBytes[i], HEX);
              //              Serial.print(" ");
              //            }
              //            Serial.println();

              uint8_t numEncoded = encodeCOBS(outcomingBytes, 7, encodedBytes); // Кодируем данные
              Serial1.write(encodedBytes, numEncoded + 1); // Отправляем ответ // +1 чтобы заканчивать c 0x00???

              //            for (int i = 0; i <= numEncoded; i++)
              //            {
              //              Serial.print(encodedBytes[i], HEX);
              //              Serial.print(" ");
              //            }
            }
            break;
          }
        case MISC_ARM_CMD_RESET(MANIPULATOR_NUMBER): // Запрос на перезагрузку модуля
          {
            if ((recieved_packet.to == MISC_ARM_ADDRESS(MANIPULATOR_NUMBER)) || (recieved_packet.to == 0xFFFF)) // Проверяем, что сообщение адресованно нам
            {
              //              Serial.print("Reset");
              //              delay(500);
              digitalWrite(RESET_PIN, LOW); // Подтягивем reset-пин к GND
            }
            break;
          }
        default:
          break;
      }
      buff_ptr = 0; // Сбрасываем счетчик принятых байт для приема нового сообщения
    }
  }
}

uint8_t encodeCOBS(const char* incomingData, int numberOfBytes, char* encodedData) // Кодирование COBS
{
  uint8_t read_index = 0;
  uint8_t write_index = 1;
  uint8_t code_index = 0;
  uint8_t code = 1;

  while (read_index < numberOfBytes)
  {
    if (incomingData[read_index] == 0)
    {
      encodedData[code_index] = code;
      code = 1;
      code_index = write_index++;
      read_index++;
    }
    else
    {
      encodedData[write_index++] = incomingData[read_index++];
      code++;

      if (code == 0xFF)
      {
        encodedData[code_index] = code;
        code = 1;
        code_index = write_index++;
      }
    }
  }

  encodedData[code_index] = code;

  return write_index;
}

uint8_t decodeCOBS(const char* incomingData, int numberOfBytes, char* decodedData) // Декодирование COBS
{
  uint8_t read_index  = 0;
  uint8_t write_index = 0;
  uint8_t code;
  uint8_t i;

  while (read_index < numberOfBytes)
  {
    code = incomingData[read_index];

    if (read_index + code > numberOfBytes && code != 1)
    {
      return 0;
    }

    read_index++;

    for (i = 1; i < code; i++)
    {
      decodedData[write_index++] = incomingData[read_index++];
    }

    if (code != 0xFF && read_index != numberOfBytes)
    {
      decodedData[write_index++] = '\0';
    }
  }
  return write_index;
}

