#include "request_handler.h"    // "фасад" транспортного справочника
#include "json_reader.h"        // работа с данными в формате json
#include "json_builder.h"
#include "map_renderer.h"

#include <iostream>             // для std::cin (isteam) и std::cout (osteam)
#include <fstream>              // для сериализации в файл
#include <string_view>

// Для отладки установить настройку в IDE
// Свойства конфигурации / Отладка / Аргументы команды:
// "<input.json 1>stdout.txt"

using namespace std;

/*
// СТАРЫЙ main()
int main()
{
    // Примерная структура программы:
    //
    // Считать JSON из stdin
    // Построить на его основе JSON базу данных транспортного справочника
    // Выполнить запросы к справочнику, находящиеся в массиве "stat_requests", построив JSON-массив
    // с ответами.
    // Вывести в stdout ответы в виде JSON

    // Создаем справочник
    transport_catalogue::TransportCatalogue tc;
    // Создаем рендерер карт
    map_renderer::MapRenderer mr;
    // Вызываем обработчик JSON с требуемыми параметрами-ссылками
    json_reader::ProcessJSON(tc, mr, std::cin, std::cout);
}
*/

void PrintUsage(std::ostream& stream = std::cerr)
{
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

// НОВЫЙ main()
int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv)
    {
        // Создаем справочник
        transport_catalogue::TransportCatalogue tc;
        // Создаем рендерер карт
        map_renderer::MapRenderer mr;
        // Вызываем JSON обработчик запросов "make_base" с требуемыми параметрами-ссылками
        json_reader::ProcessBaseJSON(tc, mr, std::cin);
    }
    else if (mode == "process_requests"sv)
    {
        // Создаем справочник
        transport_catalogue::TransportCatalogue tc;
        // Создаем рендерер карт
        map_renderer::MapRenderer mr;
        // Вызываем JSON обработчик запросов "process_requests" с требуемыми параметрами-ссылками
        json_reader::ProcessRequestJSON(tc, mr, std::cin, std::cout);
    }
    else
    {
        PrintUsage();
        return 1;
    }
}