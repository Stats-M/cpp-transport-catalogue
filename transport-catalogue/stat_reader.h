#pragma once
#include "transport_catalogue.h"    // структуры данных
#include "input_reader.h"           // методы работы со строками

#include <utility>          // for std::pair<>
#include <string>
#include <string_view>      // for string manipulation via sv
#include <sstream>          // для stringstream
#include <iomanip>         // для управления выводом (манипуляторы)
#include <istream>          // for istream
#include <iostream>         // for cout

// напишите решение с нуля
// код сохраните в свой git-репозиторий

namespace transport_catalogue::stat_reader
{

std::ostream& operator<<(std::ostream&, const Stop*); 
std::ostream& operator<<(std::ostream& os, const Route* route);

void ProcessRequests(TransportCatalogue&, std::istream&);          // Производит чтение и разбор запросов к справочнику
void ExecuteRequest(TransportCatalogue&, RequestQuery&);           // Отправляет запрос и осуществляет вывод ответа

}
