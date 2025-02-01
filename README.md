# esphome-dlms-cosem
Подключение EspHome к счетчикам электроэнергии по протоколу DLMS/COSEM/СПОДЭС (Энергомера CE207/CE307/CE308, Милур 107S, Мир, Пульсар и многие другие) через RS-485 интерфейст или через оптопорт.

Инструкции по подключению esp32/esp8266 к счётчику можно увидеть в соседнем компоненте https://github.com/latonita/esphome-energomera-iec

# Функции
## Реализованы
- Подключение без аутентификации (NONE) и с низким уровнем (LOW - доступ с паролем).
- Поддержка базовых цифровых типов данных (int/float)
- Поддержка базовых текстовых данных (octet-string)

## На будущее
- Работа с данными типа datetime
- Работа с несколькими счетчиками на одной шине
- .. предлагайте ваши варианты

# Выбор адреса сервера и клиента
Необходимо в каждом случае смотреть инструкцию к счетчику. Клиентский адрес выбирается, как правило, 32, а серверный - 1.
## client_address
| Код | Уровень доступа            |
|-----|----------------------------|
| 16  | Публичный клиент           |
| 32  | Считыватель показаний      | 

Часто для уровня 32 требуется указать пароль:
```
auth: true
password: 12345678
```

## server_address
Как правило это 1 (логический адрес из инструкции к счетчику).
Если в инструкции указан еще и физический адрес - то надо делать по схеме:
```
   server_address = 16384 * лог.адрес + физический адрес
```

<details><summary>Расчет для Милур 107S</summary>

В инструкции указано:
| Параметр           | Значение |
|--------------------|------------------------------------------------|
| Логический адрес  | 1 |
| Физический адрес  | К четырем последним цифрам серийного номера прибавить 16 и результат перевести в HEX |

Про HEX забываем, это для их родного конфигуратора. Например, серийный номер счетчика `241100010880560`, т.е. последние цифры = `0560`. Добавляем `16` и получаем `576`.

Расчитываем адрес: 16384 * 1 + 576 = 16960
```
server_address: 16960
client_address: 32
auth: true
password: "789456"
```
</details>


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
