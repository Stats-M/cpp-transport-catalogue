#pragma once
#include "transport_catalogue.h"    // структуры данных

#include <utility>          // for std::pair<>
#include <string>
#include <string_view>      // for string manipulation via sv
#include <istream>          // for istream
#include <iostream>         // for cout  (в input_reader нужна для эмуляции ввода для эха строк через cout)
#include <vector>

// напишите решение с нуля
// код сохраните в свой git-репозиторий

// В будущих проектах после снятия ограничений перенести в отдельный файл типа utility.h/.cpp
namespace detail
{
// Преобразует 1 строку в 2, разделенные count по порядку символом-разделителем
std::pair<std::string_view, std::string_view> Split(std::string_view, char, int count = 1);
// Удаляет пробелы (переводы строк, табуляцию и т.п.) из начала строки
std::string_view Lstrip(std::string_view);
// Удаляет пробелы (переводы строк, табуляцию и т.п.) с конца строки
std::string_view Rstrip(std::string_view);
// Удаляет пробелы (переводы строк, табуляцию и т.п.) со всех сторон строки
std::string_view TrimString(std::string_view);
}

namespace transport_catalogue::input_reader
{

void ProcessInput(TransportCatalogue&, std::istream&);     // Производит построчное чтение исходных данных и формирование вектора запросов
void ProcessInputQueries(TransportCatalogue&, std::vector<InputQuery>&);    // Обрабатывает запросы в соответствии с приоритетом

Stop ProcessQueryAddStop(std::string&);           // Обрабатывает запрос на добавление остановки
void ProcessQueryAddStopsDistance(TransportCatalogue&, std::string&);      // Обрабатывает запрос на добавление расстояний
Route ProcessQueryAddRoute(TransportCatalogue&, std::string&);             // Обрабатывает запрос на добавление маршрута
}
