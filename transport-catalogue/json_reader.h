/*
 * ����� ����� ���������� ��� ���������� ������������� ����������� ������� �� JSON,
 * � ����� ��� ��������� �������� � ���� � ������������ ������� ������� � ������� JSON
 * 
 * ���������� ������: ����������� �/�� json �������� ������, ������ �������� � ������� 
 * ����������� �� �������. ��� ������ � ������� � ������� json ���������� � json_reader 
 * (� ����� main.cpp - ����� � ������), ��� json_reader'� ������ �������������� �� 
 * ���������� �������� ������������� �����������. ��� ��������� �������� ��������� ������ 
 * � ������ ������� (��������, XML), �� ����� request_handler / transport_catalogue
 * 
 *   request_handler - ��������� ("�����) ������������� ����������� (�������� ������)
 *       json_reader - ��������� ������ � ������� ������� json + ���������� ������
 */

#pragma once

#include "request_handler.h"        // "�����" ������������� ��������
#include "json.h"
#include "map_renderer.h"

#include <iostream>                  // ��� std::cin (isteam) � std::cout (osteam)
#include <sstream>                   // ��� ostringstream

namespace json_reader
{

// ---------------Generic I/O-------------------------

// ������������ ��� ������ ������� JSON ��� ���������� �����������, ����������� � �������
void ProcessJSON(transport_catalogue::TransportCatalogue&, transport_catalogue::RequestHandler&, 
                 map_renderer::MapRenderer&, std::istream&, std::ostream&);
// ��������� ������� ������ � ������� JSON �� ������ � ��������� ����������
//void LoadAsJSON(transport_catalogue::TransportCatalogue&, std::istream&);
// ���������� ������� � ����������� ����� RequestHandler � ������� JSON-����� � �����
//void QueryAsJSON(transport_catalogue::RequestHandler&, std::ostream&);
//void QueryAsJSON(const json::Array&, transport_catalogue::RequestHandler&, std::ostream&);

//------------------Process data-------------------

// ������� ��������� ������ � ����������, ������� ������������������ ������� ��� ������ ����� ������
void AddToDB(transport_catalogue::TransportCatalogue&, const json::Array&);
// ������� ��������� ������ �� ��������� (��������, ����������)
void AddStopData(transport_catalogue::TransportCatalogue&, const json::Dict&);
// ������� ��������� ������ �� ��������� (����������)
void AddStopDistance(transport_catalogue::TransportCatalogue&, const json::Dict&);
// ������� ��������� ������ � ��������
void AddRouteData(transport_catalogue::TransportCatalogue&, const json::Dict&);

//------------------Process settings-------------------

const svg::Color ConvertColor_JSON2SVG(const json::Node&);
void ReadRendererSettings(map_renderer::MapRenderer&, const json::Dict&);

//--------------Processing requests-------------------

// ������� ������������ ������� � �����������, ������� ������������������ ������� ��� ������ ����� ��������
void ProcessQueriesJSON(transport_catalogue::RequestHandler&, const json::Array&, std::ostream&);
// ������� ������������ ������� ���� "Stop" (�������� ����� ���������)
const json::Node ProcessStopQuery(transport_catalogue::RequestHandler&, const json::Dict&);
// ������� ������������ ������� ���� "Bus"
const json::Node ProcessRouteQuery(transport_catalogue::RequestHandler&, const json::Dict&);
// ������� ������������ ������� ���� "Map" (����� svg-�����)
const json::Node ProcessMapQuery(transport_catalogue::RequestHandler&, const json::Dict&);

}
