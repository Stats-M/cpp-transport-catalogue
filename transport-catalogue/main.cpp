#include "request_handler.h"    // "фасад" транспортного справочника
#include "json_reader.h"        // работа с данными в формате json
#include "json_builder.h"
#include "map_renderer.h"

#include <iostream>             // для std::cin (isteam) и std::cout (osteam)

// Для отладки установить настройку в IDE
// Свойства конфигурации / Отладка / Аргументы команды:
// "<input.json 1>stdout.txt"

using namespace std;

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




    // СЕКЦИЯ ТЕСТОВ

    // Не должно компилироваться
    /*
    json::Builder{}.StartDict().Build();  // правило 3
    json::Builder{}.StartDict().Key("1"s).Value(1).Value(1);  // правило 2
    json::Builder{}.StartDict().Key("1"s).Key(""s);  // правило 1
    json::Builder{}.StartArray().Key("1"s);  // правило 4
    json::Builder{}.StartArray().EndDict();  // правило 4
    json::Builder{}.StartArray().Value(1).Value(2).EndDict();  // правило 5
    
    // Должно компилироваться
    json::Print(
        json::Document{
            json::Builder{}
            .StartDict()
                .Key("key1"s).Value(123)
                .Key("key2"s).Value("value2"s)
                .Key("key3"s).StartArray()
                    .Value(456)
                    .StartDict().EndDict()
                    .StartDict()
                        .Key(""s)
                        .Value(nullptr)
                    .EndDict()
                    .Value(""s)
                .EndArray()
            .EndDict()
            .Build()
        },
        std::cout
    );
    std::cout << endl;

    json::Print(
        json::Document{
            json::Builder{}
            .Value("just a string"s)
            .Build()
        },
        std::cout
    );
    std::cout << endl;
    */

    // КОНЕЦ СЕКЦИИ ТЕСТОВ


     // Создаем справочник
    transport_catalogue::TransportCatalogue tc;
    // Создаем рендерер карт
    map_renderer::MapRenderer mr;
    // Вызываем обработчик JSON с требуемыми параметрами-ссылками
    json_reader::ProcessJSON(tc, mr, std::cin, std::cout);
}
