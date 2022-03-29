// напишите решение с нуля
// код сохраните в свой git-репозиторий

#include "geo.h"
#include "input_reader.h"
#include "transport_catalogue.h"
#include "stat_reader.h"
#include <iostream>         // for cout

#include <sstream>

int main()
{
    using namespace std::string_literals;

    transport_catalogue::TransportCatalogue tc;


    /*
    // Закомментируйте эти 2 строки для использования тестового ввода ниже
    transport_catalogue::input_reader::ProcessInput(tc, std::cin);
    transport_catalogue::stat_reader::ProcessRequests(std::cout, tc, std::cin);
    */




    // Эмуляция пользовательского ввода
    std::stringstream ss;
    ss << "13\n"s;
    ss << "Stop Tolstopaltsevo : 55.611087, 37.20829, 3900m to Marushkino\n"s;
    ss << "Stop Marushkino : 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino\n"s;
    ss << "Bus 256 : Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"s;
    ss << "Bus 750: Tolstopaltsevo - Marushkino - Marushkino - Rasskazovka\n"s;
    ss << "Stop Rasskazovka : 55.632761, 37.333324, 9500m to Marushkino\n"s;
    ss << "Stop Biryulyovo Zapadnoye : 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam\n"s;
    ss << "Stop Biryusinka : 55.581065, 37.64839, 750m to Universam\n"s;
    ss << "Stop Universam : 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya\n"s;
    ss << "Stop Biryulyovo Tovarnaya : 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya\n"s;
    ss << "Stop Biryulyovo Passazhirskaya : 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye\n"s;
    ss << "Bus 828 : Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"s;
    ss << "Stop Rossoshanskaya ulitsa : 55.595579, 37.605757\n"s;
    ss << "Stop Prazhskaya : 55.611678, 37.603831\n"s;
    // Ввод данных завершен. Передаем введенные данные на обработку (разделение) в input_reader.h/.cpp
    transport_catalogue::input_reader::ProcessInput(tc, ss);

    // Эмуляция запросов
    std::stringstream ss_req;
    ss_req << "6\n"s;
    ss_req << "Bus 256\n"s;
    ss_req << "Bus 750\n"s;
    ss_req << "Bus 751\n"s;
    ss_req << "Stop Samara\n"s;
    ss_req << "Stop Prazhskaya\n"s;
    ss_req << "Stop Biryulyovo Zapadnoye\n"s;
    // Передаем данные запросов на обработку в stat_reader.h/.cpp
    transport_catalogue::stat_reader::ProcessRequests(std::cout, tc, ss_req);


    return 0;
}
