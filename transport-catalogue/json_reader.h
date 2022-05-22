/*
 * Назначение модуля: конвертация в/из json исходных данных, данных запросов и ответов 
 * справочника на запросы. Вся работа с данными в формате json происходит в json_reader 
 * (а также main.cpp - вывод в потоки), вне json_reader'а данные обрабатываются во 
 * внутренних форматах транспортного справочника. Это позволяет добавить обработку данных 
 * в другом формате (напримео, XML), не меняя request_handler / transport_catalogue
 * 
 *   request_handler - интерфейс ("Фасад) транспортного справочника (двоичные данные)
 *       json_reader - интерфейс работы с данными формата json + добавление данных
 */

#pragma once

#include "request_handler.h"        // "Фасад" транспортного каталога
#include "json_builder.h"
#include "json.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <iostream>                  // для std::cin (isteam) и std::cout (osteam)
#include <sstream>                   // для ostringstream
#include <vector>                    // для вектора этапов обработки входящих данных

namespace json_reader
{

// ---------------Generic I/O-------------------------

// Обрабатывает все данные формата JSON для указанного справочника, обработчика и потоков
void ProcessJSON(transport_catalogue::TransportCatalogue&, map_renderer::MapRenderer&, 
                 std::istream&, std::ostream&);
// Считывает входные данные в формате JSON из потока и заполняет справочник
//void LoadAsJSON(transport_catalogue::TransportCatalogue&, std::istream&);
// Отправляет запросы к справочнику через RequestHandler и выводит JSON-ответ в поток
//void QueryAsJSON(transport_catalogue::RequestHandler&, std::ostream&);
//void QueryAsJSON(const json::Array&, transport_catalogue::RequestHandler&, std::ostream&);

//------------Process json input data section-------------------

// Функция добавляет данные в справочник, вызывая специализированные функции для разных типов данных
void AddToDB(transport_catalogue::TransportCatalogue&, const json::Array&);
// Функция добавляет данные об остановке (название, координаты)
void AddStopData(transport_catalogue::TransportCatalogue&, const json::Dict&);
// Функция добавляет данные об остановке (расстояния)
void AddStopDistance(transport_catalogue::TransportCatalogue&, const json::Dict&);
// Функция добавляет данные о маршруте
void AddRouteData(transport_catalogue::TransportCatalogue&, const json::Dict&);

//------------------Process settings-------------------

const svg::Color ConvertJSONColorToSVG(const json::Node&);
void ReadRendererSettings(map_renderer::MapRenderer&, const json::Dict&);
void ReadRouterSettings(router::TransportRouter&, const json::Dict&);

//--------------Requests section parsing-------------------

// Функция осуществляет разбор секции запросов JSON, назначая соответствующий обработчик
void ParseRawJSONQueries(transport_catalogue::RequestHandler&, router::TransportRouter&, const json::Array&, std::ostream&);
// Функция обрабатывает запросы типа "Stop" (маршруты через остановку)
const json::Node ProcessStopQuery(transport_catalogue::RequestHandler&, const json::Dict&);
// Функция обрабатывает запросы типа "Bus"
const json::Node ProcessBusQuery(transport_catalogue::RequestHandler&, const json::Dict&);
// Функция обрабатывает запросы типа "Map" (текст svg-файла)
const json::Node ProcessMapQuery(transport_catalogue::RequestHandler&, const json::Dict&);
// Функция обрабатывает запросы типа "Route" (построение маршрута между произвольными остановками)
const json::Node ProcessRouteQuery(router::TransportRouter&, const json::Dict&);
}
