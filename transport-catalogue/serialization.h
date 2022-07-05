#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_router.h"
#include "graph.h"
#include "transport_catalogue.pb.h"

#include <fstream>
#include <vector>
#include <stdexcept>     // for exception throw

namespace serialization
{
class Serializer
{
public:

	// �����������. �������� ������ �� ��� ��������� �������
	Serializer(transport_catalogue::TransportCatalogue& tc,
			   map_renderer::MapRenderer& mr,
			   router::TransportRouter* tr = nullptr);

	// ����������� ������ (����� ����� ������������)
	void Serialize(const std::string& filename);
	// ������������� ������ (����� ����� ��������������), ����� �������
	void Deserialize(const std::string& filename);
	// �������������� ������ ������� (��������� ������)
	void DeserealizeRouter(router::TransportRouter* tr);

private:

	transport_catalogue::TransportCatalogue& tc_;
	map_renderer::MapRenderer& mr_;
	router::TransportRouter* tr_ = nullptr;           //���������, �.�. ����� �� ������������
	proto_serialization::TransportCatalogue proto_all_settings_;

	// ������������ message Stop
	void SerializeStop();
	// ������������ message Distance
	void SerializeDistance();
	// ������������ message Route (Bus)
	void SerializeRoute();
	// ������������ message Color
	proto_serialization::Color SerializeColor(const svg::Color& color);
	// ������������ message RendererSettings
	void SerealizeRendererSettings();
	// ������������ message RouterSettings
	void SerealizeRouterSettings();

	// �������������� ��������
	void DeserializeCatalogue();
	// �������������� ���������
	void DeserializeRenderer();
	// �������������� �������� ����������
	map_renderer::RendererSettings DeserializeRendererSettings(const proto_serialization::RendererSettings& proto_renderer_settings);
	// �������������� �����
	svg::Color DeserializeColor(const proto_serialization::Color& color_ser);
};

} // namespace serialization
