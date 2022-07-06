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

	// Конструктор. Получает ссылки на все зависимые объекты
	Serializer(transport_catalogue::TransportCatalogue& tc,
			   map_renderer::MapRenderer& mr,
			   router::TransportRouter* tr = nullptr);

	// Сериализует данные (точка входа сериализации)
	void Serialize(const std::string& filename);
	// Десериализует данные (точка входа десериализации), кроме роутера
	void Deserialize(const std::string& filename);
	// Десериализация данных роутера (последним этапом)
	void DeserializeRouter(router::TransportRouter* tr);

private:

	transport_catalogue::TransportCatalogue& tc_;
	map_renderer::MapRenderer& mr_;
	router::TransportRouter* tr_ = nullptr;           //Указатель, т.к. может не существовать
	proto_serialization::TransportCatalogue proto_all_settings_;

	// Сериализация message Stop
	void SerializeStop();
	// Сериализация message Distance
	void SerializeDistance();
	// Сериализация message Route (Bus)
	void SerializeRoute();
	// Сериализация message Color
	proto_serialization::Color SerializeColor(const svg::Color& color);
	// Сериализация message RendererSettings
	void SerializeRendererSettings();
	// Сериализация message RouterSettings
	void SerializeRouterSettings();

	// Десериализация каталога
	void DeserializeCatalogue();
	// Десериализация рендерера
	void DeserializeRenderer();
	// Десериализация настроек рендеринга
	map_renderer::RendererSettings DeserializeRendererSettings(const proto_serialization::RendererSettings& proto_renderer_settings);
	// Десериализация цвета
	svg::Color DeserializeColor(const proto_serialization::Color& color_ser);
};

} // namespace serialization
