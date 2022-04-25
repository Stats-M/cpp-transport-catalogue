#include "request_handler.h"    // "фасад" транспортного справочника
#include "json_reader.h"        // работа с данными в формате json
#include "map_renderer.h"

#include <iostream>             // для std::cin (isteam) и std::cout (osteam)

// Свойства конфигурации / Отладка / Аргументы команды:
// "<input.json 1>stdout.txt"

int main()
{
    /*
     * Примерная структура программы:
     *
     * Считать JSON из stdin
     * Построить на его основе JSON базу данных транспортного справочника
     * Выполнить запросы к справочнику, находящиеся в массиве "stat_requests", построив JSON-массив
     * с ответами.
     * Вывести в stdout ответы в виде JSON
     */

    // Создаем справочник
    transport_catalogue::TransportCatalogue tc;
    // Создаем рендерер карт
    map_renderer::MapRenderer mr;
    // Вызываем обработчик JSON с требуемыми параметрами-ссылками
    json_reader::ProcessJSON(tc, mr, std::cin, std::cout);
}
