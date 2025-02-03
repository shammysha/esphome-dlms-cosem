# esphome-dlms-cosem
Подключение EspHome к счетчикам электроэнергии по протоколу DLMS/COSEM/СПОДЭС (Энергомера CE207/CE307/CE308, Милур 107S, Мир, Пульсар и многие другие) через RS-485 интерфейст или через оптопорт.

Инструкции по подключению esp32/esp8266 к счётчику можно увидеть в соседнем компоненте https://github.com/latonita/esphome-energomera-iec

# Функции
## Реализованы
- Подключение без аутентификации (NONE) и с низким уровнем (LOW - доступ с паролем).
- Поддержка базовых цифровых типов данных (int/float)
- Поддержка базовых текстовых данных (octet-string)
- Поддержка русских символов в ответах от счетчиков (Нартис И100-W112, "Тип устройства",)
- Задание логического и физического адресов
- Работа с несколькими счетчиками на одной шине


## Возможные задачи на будущее
- Работа с данными типа datetime
- Синхронизация времени
- Управление реле
- .. предлагайте ваши варианты

Для будущей разработки требуется наличие счетчика для проверки

# Настройка основного компонента (хаба)

Простейшая конфигурация выглядит вот так:
```yaml
dlms_cosem:
```
По-сложнее вот так:

```yaml
dlms_cosem:
    client_address: 32
    server_address: 
      logical_device: 1
      physical_device: 576
      address_length: 2
    auth: true
    password: "12345678"
    update_interval: 60s
    receive_timeout: 500ms
    delay_between_requests: 50ms
    flow_control_pin: GPIO12

```

- **client_address** (*Optional*) см.документацию на счетчик. от этого зависит уровень доступа. по-умолчанию 16
- **server_address** (*Optional*) HDLC адрес :) можно попробовать не указывать. Энергомеры, например, это едят. по-умолчанию 1. а вообще, адрес расчитывается по алгоритму, на основе логического и физического адресов, а также размера адреса. см. инструкцию на счетчик. либо рассчитваем сами ручками, либо указываем параметры ниже:

    - **logical_device** (*Optional*) номер логического устройста. по стандарту СПОДЭС должно быть как минимум устройсво номер 1. по-умолчанию 1.
    - **physcial_device** (**Required**) номер физического устройства. см. документацию на счетчик. может быть как фиксированной величиной, так и зависящей от серийного номера счетчика (как правило несколько последних цифр + число). может отличаться для разных интерфейсов (RS-485/Оптопорт/и т.д.)
    - **address_length** (*Optional*) длина адреса, 1, 2 или 4 байта. по-умолчанию 2.

- **auth** (*Optional*) необходимость авторизации. *true* если требуется указать пароль. по-умолчанию *false*
- **password** (*Optional*) пароль. используется вместе с параметром `auth`
- **update_interval** (*Optional*) периодичность опроса счетчика и публикации результатов. по-умолчанию `60s`
- **receive_timeout** (*Optional*) как долго ждем на наш запрос мы ждем ответ от счетчика. по-умолчанию `500ms`
- **delay_between_requests** (*Optional*) размер паузы между последовательными запросами к счетчику. по-умолчанию `50ms`
- **flow_control_pin** (*Optional*)  указываем, если 485 модуль требует сигнал направления передачи RE/DE 



## Чуть побольше о выборе адреса сервера и клиента
Для ряда счетчиков можно попробовать не указывать адреса, тогда будут использованы значения по-умолчанию (16, 1)
Необходимо в каждом случае смотреть инструкцию к счетчику. Клиентский адрес выбирается, как правило, 32, а серверный - 1. При этом практически во всех счетчиках для уровня 32 требуется указать пароль.

### Возможные client_address
СПОДЭС говорит нам:
| Код | Уровень доступа            | Разрешенные операции | Защита | 
|-----|----------------------------|----------------------| ------ |
| 16  | Публичный клиент           | чтения | нет |
| 32  | Считыватель показаний      | чтения, селективной выборки, выполнение определенных действий | пароль |
| 48  | Конфигуратор               | чтения, записи, селективной выборки, выполнение определенных действий | пароль или *шифрование* |

На данный момент поддерживается только чтение данных, выбор уровня зависит от счетчика.
Шифрование не подддерживается.


### server_address
Как правило это двухбайтный адрес, верхний байт - логический адрес, нижний - физический адрес. Берем из инструкции к счетчику.

<details><summary>Пример для Милур 107S</summary>

В инструкции указано:
| Параметр           | Значение |
|--------------------|------------------------------------------------|
| Логический адрес  | 1 |
| Физический адрес  | К четырем последним цифрам серийного номера прибавить 16 и результат перевести в HEX |

Про HEX забываем, это для их родного конфигуратора. Например, серийный номер счетчика `241100010880560`, т.е. последние цифры = `0560`. Добавляем `16` и получаем `576`.

</details>

# Настройка сенсоров
- `text_sensor` - текстовые данные в формате "как пришли от счетчика"
- `sensor` - числовые данные, float

OBIS коды берем из стандарта СПОДЭС и из инструкций к счетчику

## Sensor
```yaml
sensor:
# 2 Current - Iph Ток фазы 1.0.11.7.0.255
  - platform: dlms_cosem
    name: Phase Current
    obis_code: 1.0.11.7.0.255
    multiplier: 1.0             # предварительная мультипликация значения (до отработки filters:) по-умолчанию 1
    dont_publish: false         # не публиковать значение, будет видно только в логах. по-умолчанию false
    
    # далее идут стандартные параметры сенсоров
    unit_of_measurement: A
    accuracy_decimals: 1
    device_class: current
    state_class: measurement
```

## Text sensor
```yaml
text_sensor:
  - platform: dlms_cosem
    name: Serial Number
    obis_code: 0.0.96.1.0.255
    cp1251: false               # включить преобразование русских символов CP1251 -> UTF-8 для отображения в home assistant. по-умолчанию false
    dont_publish: false         # не публиковать значение, будет видно только в логах. по-умолчанию false
    # далее идут стандартные параметры сенсоров
    entity_category: diagnostic
```
Ряд счетчиков выдают свои названия или некоторые другие поля на русском языке в кодировке cp1251, которую требуется перевести в UTF-8 для правильного отображения в Home Assistant.
Например, РиМ выдает значение 0.0.96.1.1.255 (Тип ПУ) как "РиМ489.38". Аналогично, Нартис выдает название на русском.

## Binary sensor
```yaml

binary_sensor: 
  - platform: dlms_cosem
    connection:
      name: Connection
    transmission: 
      name: Transmission

```
**Connection** включается в начале сессии, отключается по окончанию.
**Transmission** включается на период каждого отдельного запроса. можно подключить, например, к светодиоду для индикации активности

```yaml
binary_sensor: 
  - platform: dlms_cosem
    transmission: 
      name: Transmission
      on_press:
        output.turn_on: conn_led
      on_release:
        output.turn_off: conn_led

output:
  - platform: gpio
    id: conn_led
    pin: GPIO04
    inverted: true
```

# Lambda

- `update_server_address(logical_device, physical_device, addr_len)` - обновление адреса и внеочередной запуск опроса.

Данную функцию, например, можно использовать в lambda для сканирования адресов устройства совместно с вышеупомянутым бинарным сенсором `Connection`:
```yaml
globals:
  - id: logaddr
    type: uint16_t
    initial_value: '1'
    
  - id: phyaddr
    type: uint16_t
    initial_value: '1'

  - id: servaddr
    type: uint16_t
    initial_value: '1' 

binary_sensor: 
  - platform: dlms_cosem
    connection: 
      name: Connection
      #.....
      on_release:
        - lambda: |-
            if (id(phyaddr) < 255) {
              id(phyaddr)++;
            } else {
              id(logaddr)++;
              id(phyaddr) = 1;
            }
            id(servaddr) = id(energo_01)->update_server_address(id(logaddr), id(phyaddr), 2);
        - lambda: |-
            ESP_LOGI("main", "Logical address: %d, physical address: %d, server address: %d", (int) id(logaddr), (int) id(phyaddr), (int) id(servaddr));

interval: # выводим в лог текущие настройки, чтобы не искать последнее изменение по логу
  interval: 10s
  then:
    - lambda: |-
        ESP_LOGI("main", "Logical address: %d, physical address: %d, server address: %d", (int) id(logaddr), (int) id(phyaddr), (int) id(servaddr));
```

# Особенности счетчиков

## Нартис И100-W112

- Передает тип ПУ на русском языке. Для текстового сенсора `0.0.96.1.1.255` необходимо установить `cp1251: true`
- Иногда пароли и явки в инструкции отличаются от реальных. Пример рабочих параметро с одного из счетчиков:

    * Пароль администрирования: 0000000100000001
    * Пароль чтения: 00000001
    * Логический адрес: 1
    * Физический адрес: 17
    * Размер адреса: 2

## РиМ489.38 и другие из серии
- Передает тип ПУ на русском языке. Для текстового сенсора `0.0.96.1.1.255` необходимо установить `cp1251: true`

# Примеры конфигураций

## Однофазный счетчик (ПУ категории D) 
Используется список параметров ПУ категории D из стандарта СПОДЭС. Они применяются в однофазных ПУ потребителей.

Пример файла конфигурации, протестированого на Энергомера CE207-SPds.

```
esphome:
  name: energomera-ce207-spds
  friendly_name: Energomera-ce207-spds

esp32:
  board: esp32dev
  framework:
    type: arduino

#...

external_components:
  - source: github://latonita/esphome-dlms-cosem
    components: [dlms_cosem]
    refresh: 1s

uart:
  - id: bus_1
    rx_pin: GPIO16
    tx_pin: GPIO17
    baud_rate: 9600
    data_bits: 8
    parity: NONE
    stop_bits: 1 

dlms_cosem:
  id: energo_01
  client_address: 32
  server_address: 1
  auth: true
  password: "12345678"
  update_interval: 60s
  receive_timeout: 1s
sensor:
# 2 Current - Iph Ток фазы 1.0.11.7.0.255
  - platform: dlms_cosem
    name: Phase Current
    obis_code: 1.0.11.7.0.255
    unit_of_measurement: A
    accuracy_decimals: 1
    device_class: current
    state_class: measurement
# 3 Current - In Ток нулевого провода 1.0.91.7.0.255
  - platform: dlms_cosem
    name: Neutral Current
    obis_code: 1.0.91.7.0.255
    unit_of_measurement: A
    accuracy_decimals: 1
    device_class: current
    state_class: measurement
# 4 Voltage - V Напряжение фазы 1.0.12.7.0.255
  - platform: dlms_cosem
    name: Phase Voltage
    obis_code: 1.0.12.7.0.255
    unit_of_measurement: V
    accuracy_decimals: 1
    device_class: voltage
    state_class: measurement
# 5 Power Factor - PF Коэффициент мощности 1.0.13.7.0.255
  - platform: dlms_cosem
    name: Power Factor
    obis_code: 1.0.13.7.0.255
    unit_of_measurement: ''
    accuracy_decimals: 2
    device_class: power_factor
    state_class: measurement
# 6 Frequency Частота сети 1.0.14.7.0.255
  - platform: dlms_cosem
    name: Grid Frequency
    obis_code: 1.0.14.7.0.255
    unit_of_measurement: Hz
    accuracy_decimals: 1
    device_class: frequency
    state_class: measurement
# 7 Apparent Power Полная мощность 1.0.9.7.0.255
  - platform: dlms_cosem
    name: Apparent Power
    obis_code: 1.0.9.7.0.255
    unit_of_measurement: W
    accuracy_decimals: 1
    device_class: power
    state_class: measurement
# 8 Signed Active Power (+Import; -Export) Активная мощность 1.0.1.7.0.255
  - platform: dlms_cosem
    name: Active Power
    obis_code: 1.0.1.7.0.255
    unit_of_measurement: W
    accuracy_decimals: 1
    device_class: power
    state_class: measurement
# 9 Signed Reactive Power (+Import; -Export) Реактивная мощность 1.0.3.7.0.255
  - platform: dlms_cosem
    name: Reactive Power
    obis_code: 1.0.3.7.0.255
    unit_of_measurement: W
    accuracy_decimals: 1
    device_class: power
    state_class: measurement
# 10 Cumulative Active Energy (Import) Активная энергия, импорт 1.0.1.8.0.255
  - platform: dlms_cosem
    name: Active Energy
    obis_code: 1.0.1.8.0.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
# 10.1 Cumulative Active Energy (Import) tariff 1
  - platform: dlms_cosem
    name: Active Energy T1
    obis_code: 1.0.1.8.1.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
# 10.2 Cumulative Active Energy (Import) tariff 2
  - platform: dlms_cosem
    name: Active Energy T2
    obis_code: 1.0.1.8.2.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
# 11 Cumulative Active Energy (Export) Активная энергия, экспорт 1.0.2.8.0.255
  - platform: dlms_cosem
    name: Active Energy Export
    obis_code: 1.0.2.8.0.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
# 12 Cumulative Reactive Energy (Import) Реактивная энергия, импорт 1.0.3.8.0.255
  - platform: dlms_cosem
    name: Reactive Energy
    obis_code: 1.0.3.8.0.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
# 13 Cumulative Reactive Energy (Export) Реактивная энергия, экспорт 1.0.4.8.0.255
  - platform: dlms_cosem
    name: Reactive Energy Export
    obis_code: 1.0.4.8.0.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
text_sensor:
  - platform: dlms_cosem
    name: Serial Number
    obis_code: 0.0.96.1.0.255
    entity_category: diagnostic
  - platform: dlms_cosem
    name: Type
    obis_code: 0.0.96.1.1.255
    cp1251: true                     # ряд счетчиков выдают данный параметр на русском языке
    entity_category: diagnostic
  - platform: dlms_cosem
    name: Metrology Software Version
    obis_code: 0.0.96.1.2.255
    entity_category: diagnostic
  - platform: dlms_cosem
    name: Manufacturer
    obis_code: 0.0.96.1.3.255
    entity_category: diagnostic

```
