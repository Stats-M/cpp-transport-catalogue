#pragma once

// �������� ���������� ����������� SVG: https://www.svgviewer.dev/

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <string_view>
#include <algorithm>
#include <optional>
#include <variant>

namespace svg
{

// ���� � ������� RGB
class Rgb
{
public:
    Rgb() = default;

    Rgb(uint8_t red, uint8_t green, uint8_t blue) :
        red(red), green(green), blue(blue)
    {}

    ~Rgb() = default;

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

// ���� � ������� RGBA
class Rgba
{
public:
    Rgba() = default;

    Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity) :
        red(red), green(green), blue(blue), opacity(opacity)
    {}

    ~Rgba() = default;

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0L;  // G-��������� 100% ��������������
};


using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;

// ��������� ��� ������ Color (��� Variant) ����� std::visit()
struct OstreamSolutionPrinter
{
    std::ostream& out;

    void operator()(std::monostate) const
    {}
    void operator()(std::string str) const
    {
        out << str;
    }
    void operator()(Rgb rgb) const
    {
        using namespace std::literals;
        out << "rgb("s << std::to_string(rgb.red) << ","s << std::to_string(rgb.green) << ","s << std::to_string(rgb.blue) << ")"s;
    }
    void operator()(Rgba rgba) const
    {
        using namespace std::literals;
        out << "rgba("s << std::to_string(rgba.red) << ","s << std::to_string(rgba.green) << ","s << std::to_string(rgba.blue) << ","s << rgba.opacity << ")"s;
    }
};

// ������� � ������������ ����� ��������� �� �������������� inline,
// �� ������� ���, ��� ��� ����� ����� �� ��� ������� ����������,
// ������� ���������� ���� ���������.
// � ��������� ������ ������ ������� ���������� ����� ������������ ���� ����� ���� ���������
inline const Color NoneColor{ "none" };

// ��� ����� ����� �����
enum class StrokeLineCap
{
    BUTT,
    ROUND,
    SQUARE,
};

// ��� ����� ���������� �����
enum class StrokeLineJoin
{
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

// �������� ������� ��������� ������������� ����� ����� ����� � �����
std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap);

// �������� ������� ��������� ������������� ����� ���������� ����� � �����
std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join);

// ����� PathProps ������ ��������� ��������� �������� Owner, ������� ����� 
// ��� ������, ���������� ����� ����������. ��� ���� ��������� method chaining, 
// ��������� ������ �� ������� ��������� ����������� ����. 
// ��������, ��� ������ PathProps::SetFillColor � ������ Circle ������ ������������ ������ Circle&.
// Curiously Recurring Template Pattern (CRTP)
template <typename Owner>
class PathProps
{
public:
    // ����� �������� �������� fill � ���� �������. 
    // �� ��������� �������� �� ���������.
    Owner& SetFillColor(Color color)
    {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    // ����� �������� �������� stroke � ���� �������. 
    // �� ��������� �������� �� ���������.
    Owner& SetStrokeColor(Color color)
    {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    // ����� �������� �������� stroke-width � ������� �����. 
    // �� ��������� �������� �� ���������.
    Owner& SetStrokeWidth(double width)
    {
        width_ = width;
        return AsOwner();
    }
    // ����� �������� �������� stroke-linecap � ��� ����� ����� �����.
    // �� ��������� �������� �� ���������.
    Owner& SetStrokeLineCap(StrokeLineCap line_cap)
    {
        line_cap_ = line_cap;
        return AsOwner();
    }
    // ����� �������� �������� stroke-linejoin � ��� ����� ���������� �����. 
    // �� ��������� �������� �� ���������.
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join)
    {
        line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const
    {
        using namespace std::literals;

        if (fill_color_)
        {
            // out << " fill=\""sv << *fill_color_ << "\""sv;  // Obsolete
            out << " fill=\""sv;
            visit(OstreamSolutionPrinter{ out }, *fill_color_);
            out << "\""sv;
        }
        if (stroke_color_)
        {
            // out << " stroke=\""sv << *stroke_color_ << "\""sv;  // Obsolete
            out << " stroke=\""sv;
            visit(OstreamSolutionPrinter{ out }, *stroke_color_);
            out << "\""sv;
        }
        if (width_)
        {
            out << " stroke-width=\""sv << *width_ << "\""sv;
        }
        if (line_cap_)
        {
            out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
        }
        if (line_join_)
        {
            out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner()
    {
        // static_cast ��������� ����������� *this � Owner&,
        // ���� ����� Owner � ��������� PathProps
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> width_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> line_join_;
};


struct Point
{
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y)
    {}
    double x = 0.0;
    double y = 0.0;
};

/*
 * ��������������� ���������, �������� �������� ��� ������ SVG-��������� � ���������.
 * ������ ������ �� ����� ������, ������� �������� � ��� ������� ��� ������ ��������
 */
struct RenderContext
{
    RenderContext(std::ostream& out)
        : out(out)
    {}

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent)
    {}

    RenderContext Indented() const
    {
        return { out, indent_step, indent + indent_step };
    }

    void RenderIndent() const
    {
        for (int i = 0; i < indent * indent_step; ++i)
        {
            out.put(' ');
        }
    }

    std::ostream& out;
    //int indent_step = 4;
    int indent_step = 1;
    int indent = 0;
};


/*
 * ����������� ������� ����� Object ������ ��� ���������������� ��������
 * ���������� ����� SVG-���������
 * ��������� ������� "��������� �����" ��� ������ ����������� ����
 */
class Object
{
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};


// ---------- IObjectContainer ------------------

// ObjectContainer ����� ��������� ��� ������� � ���������� SVG-��������. 
// ����� ���� ��������� Drawable-������� ����� ��������������� ����, �������� 
// � ��������� SVG-���������. 
// ������ ������, ObjectContainer � ��� ����������� �����, � �� ���������, ��� ��� 
// ��������� ����� Add, ����������� ����������� Object �� ��������, �� ��������� 
// ������� �����������: � C++ ��������� ������ �� ����� ���� ������������.
class ObjectContainer
{
public:
    // ���������� �� ������ ����� ������������ ������ ObjectContainer::AddPtr, 
    // ������������ unique_ptr<Object>&& .
    template <typename T>
    void Add(T obj)
    {
        AddPtr(std::make_unique<T>(obj));
    }

protected:
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};

// ---------- IDrawable ------------------

// ��������� Drawable ����������� ������ � ���������, ������� ����� ����������, 
// ��������� SVG-����������. ��� ����� � �� ���� ����� Draw, ����������� ������ 
// �� ��������� ObjectContainer.
class Drawable
{
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};


/*
 * ����� Circle ���������� ������� <circle> ��� ����������� �����
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle>
{
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * ����� Polyline ���������� ������� <polyline> ��� ����������� ������� �����
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline>
{
public:
    // ��������� ��������� ������� � ������� �����
    Polyline& AddPoint(Point point);

    /*
     * ������ ������ � ������, ����������� ��� ���������� �������� <polyline>
     */
private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points_;
};

/*
 * ����� Text ���������� ������� <text> ��� ����������� ������
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text>
{
public:
    // ����� ���������� ������� ����� (�������� x � y)
    Text& SetPosition(Point pos);

    // ����� �������� ������������ ������� ����� (�������� dx, dy)
    Text& SetOffset(Point offset);

    // ����� ������� ������ (������� font-size)
    Text& SetFontSize(uint32_t size);

    // ����� �������� ������ (������� font-family)
    Text& SetFontFamily(std::string font_family);

    // ����� ������� ������ (������� font-weight)
    Text& SetFontWeight(std::string font_weight);

    // ����� ��������� ���������� ������� (������������ ������ ���� text)
    Text& SetData(std::string data);

    // ������ ������ � ������, ����������� ��� ���������� �������� <text>
private:
    std::string Encode(std::string data) const;
    void RenderObject(const RenderContext& context) const override;
    Point position_;
    Point offset_;
    uint32_t size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class Document : public ObjectContainer
{
public:

    Document() = default;

    /*
     ����� Add ��������� � svg-�������� ����� ������-��������� svg::Object.
     ������ �������������:
     Document doc;
     doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
    */
    template <typename Obj>
    void Add(Obj obj)
    {
        objects_.emplace_back(std::make_unique<Obj>(std::move(obj)));
    }

    // ��������� � svg-�������� ������-��������� svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj) override
    {
        objects_.push_back(std::move(obj));
    }

    // ������� � ostream svg-������������� ���������
    void Render(std::ostream& out) const;

    // ������ ������ � ������, ����������� ��� ���������� ������ Document
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg