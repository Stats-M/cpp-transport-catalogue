#pragma once
#include "transport_catalogue.h"    // структуры данных

#include <utility>          // for std::pair<>
#include <string>
#include <string_view>      // for string manipulation via sv
#include <istream>          // for istream
#include <iostream>         // for cout

// напишите решение с нуля
// код сохраните в свой git-репозиторий

namespace transport_catalogue::stat_reader
{

// Производит чтение запросов к справочнику
void ProcessRequest(TransportCatalogue&, std::istream&);

}
