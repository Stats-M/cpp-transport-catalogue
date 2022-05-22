/*
 * Назначение модуля: обработчик запросов к базе, содержащего логику,
 * которую не хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 *    request_handler - интерфейс("Фасад) транспортного справочника (двоичные данные)
 *        json_reader - интерфейс работы с данными формата json + добавление данных
 */

#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"

#include <unordered_set>    // для типа данных в RequestHandler
#include <optional>         // для типа данных в RequestHandler
#include <string_view>      // для типа данных в RequestHandler
#include <map>              // для передачи словаря всех маршрутов

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

namespace transport_catalogue
{
// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)
class RequestHandler
{
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(const TransportCatalogue& tc, map_renderer::MapRenderer& mr) : tc_(tc), mr_(mr)
    {}

    // Возвращает информацию о маршруте
    const std::optional<RouteStatPtr> GetRouteInfo(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через остановку
    //const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;
    const std::optional<StopStatPtr> GetBusesForStop(const std::string_view& stop_name) const;

    // Возвращает SVG документ, сформированный map_renderer
    svg::Document GetMapRender() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportCatalogue& tc_;

    map_renderer::MapRenderer& mr_;
};
}
