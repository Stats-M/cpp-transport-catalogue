#pragma once
#include "transport_catalogue.h"    // структуры данных

#include <utility>          // for std::pair<>
#include <string>
#include <string_view>      // for string manipulation via sv
#include <istream>          // for istream
#include <iostream>         // for cout  (в input_reader нужна для эмуляции ввода для эха строк через cout)

// напишите решение с нуля
// код сохраните в свой git-репозиторий

namespace transport_catalogue::input_reader
{

// Производит построчное чтение исходных данных согласно заданной схеме
// и формирует список запросов на обновление каталога
void ProcessInput(TransportCatalogue&, std::istream&);

}
