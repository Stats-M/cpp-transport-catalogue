### cpp-transport-catalogue
# Проект: транспортный справочник (в разработке)

Система хранения транспортных маршрутов и обработки запросов к ней
- Входные данные и ответ в JSON-формате. Выходной JSON-файл может содержать визуализацию карты маршрута(ов) в формате SVG-файла.  
- Поиск кратчайшего маршрута. 
- Сериализация базы данных и настроек справочника при помощи Google Protobuf. 
- Объекты JSON поддерживают цепочки вызовов при конструировании, превращая ошибки применения данных формата JSON в ошибки компиляции.

### Использованные идеомы, технологии и элементы языка
- OOP: inheritance, abstract interfaces, final classes
- Unordered map/set
- STL smart pointers
- std::variant and std:optional
- JSON load / output
- SVG image format embedded inside XML output
- Curiously Recurring Template Pattern (CRTP)
- Facade design pattern
- C++17 with С++20 Ranges emulation
- Directed Weighted Graph data structure for Router module
- Google Protocol Buffers for data serialization
- Static libraries .LIB/.A
- CMake generated project and dependency files

# STOP STOP STOP STOP STOP
Information below is obsolete due to large scale architechture changes. Please wait until instructions below will be updated.

[obsolete] Формат ввода базы данных
========================
В первой строке стандартного потока ввода содержится число N — количество запросов на обновление базы данных, затем — по одному на строке — вводятся сами запросы. Запросы бывают:

```
Stop X: latitude, longitude, D1m to stop1, D2m to stop2, ...
```
Добавляет информацию об остановке с названием X и координатами latitude (широта) и longitude (долгота) на земной поверхности. Название остановки может состоять из нескольких слов. Используйте двоеточие как признак окончания названия остановки. Широта задаётся в градусах от -90.0 (южный полюс) до +90.0 (северный полюс). Положительные широты расположены севернее экватора, отрицательные — южнее. Долгота задаётся в градусах от -180.0 до +180.0, положительные значения соответствуют восточной долготе, а отрицательные — западной. Нулевой меридиан проходит через Гринвичскую королевскую обсерваторию в Лондоне, а координаты останкинской телевышки равны 55.8199081 северной широты и 37.6116028 восточной долготы. Широта и долгота разделяются запятой, за которой следует пробел.
После широты и долготы содержится список расстояний от этой остановки до соседних с ней остановок. Расстояния задаются в метрах. Предполагается, что расстояние от X до stop# равно расстоянию от stop# до X, если только расстояние от stop# до X не задано явно при добавлении остановки stop#.
Гарантируется, что остановка X определена не более чем в одном запросе Stop.
```
Bus X: описание маршрута
```
Запрос на добавление автобусного маршрута X. Название маршрута может состоять из нескольких слов и отделяется от описания символом двоеточия. Описание маршрута может задаваться в одном из двух форматов (см. пример):
stop1 - stop2 - ... stopN: автобус следует от stop1 до stopN и обратно с указанными промежуточными остановками.
stop1 > stop2 > ... > stopN > stop1: кольцевой маршрут с конечной stop1.

[obsolete] Формат запросов к базе данных
=============================
Запросы к базе данных подаются в cin после запросов на создание базы. В первой строке вводится количество запросов, затем — по одному в строке — вводятся сами запросы. Запросы в этой задаче бывают:

```
Bus X
Вывести информацию об автобусном маршруте X в следующем формате:
Bus X: R stops on route, U unique stops, L route length, C curvature.
```
Здесь
X — название маршрута. Оно совпадает с названием, переданным в запрос Bus.
R — количество остановок в маршруте автобуса от stop1 до stop1 включительно.
U — количество уникальных остановок, на которых останавливается автобус. Одинаковыми считаются остановки, имеющие одинаковые названия.
L — фактическая длина маршрута в метрах. Задается расстоянием между остановками.
С — извилистость, то есть отношение фактической длины маршрута к географическому расстоянию. Фактическая длина маршрута — это L. Географическое расстояние — это длина маршрута, которая вычисляется по поверхности земного шара по прямой между двумя точками.

```
Stop X
Вывести информацию об остановке X в следующем формате:
Stop X: buses bus1 bus2 ... busN
```
bus1 bus2 ... busN — список автобусов, проходящих через остановку.

[obsolete] Пример 
======
```
Ввод
13
Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino
Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino
Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye
Bus 750: Tolstopaltsevo - Marushkino - Marushkino - Rasskazovka
Stop Rasskazovka: 55.632761, 37.333324, 9500m to Marushkino
Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam
Stop Biryusinka: 55.581065, 37.64839, 750m to Universam
Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya
Stop Biryulyovo Tovarnaya: 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya
Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye
Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye
Stop Rossoshanskaya ulitsa: 55.595579, 37.605757
Stop Prazhskaya: 55.611678, 37.603831

6
Bus 256
Bus 750
Bus 751
Stop Samara
Stop Prazhskaya
Stop Biryulyovo Zapadnoye 
```

```
Вывод
Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature
Bus 750: 7 stops on route, 3 unique stops, 27400 route length, 1.30853 curvature
Bus 751: not found
Stop Samara: not found
Stop Prazhskaya: no buses
Stop Biryulyovo Zapadnoye: buses 256 828 
```
