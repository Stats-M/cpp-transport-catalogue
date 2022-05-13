#pragma once

#include "json.h"

namespace json
{

class Builder;
class BaseContext;
class KeyContext;
class DictItemContext;
class ArrayItemContext;
class KeyValueContext;
class ArrayValueContext;



class Builder
{
public:
    // ����������� ����� ���� �������� ��������� � ����� ����� ������ �� root_
    //Builder();

    KeyContext Key(std::string);
    BaseContext Value(Node::Value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    BaseContext EndDict();
    BaseContext EndArray();

    //protected:
    json::Node Build();

private:
    // �������������� ������
    // ������ ��� ���� Node::Value, � �� Node, �.�. � ��� Node �������� ����������� �� variant
    //Node::Value root_ = nullptr;

    // ��������� �� ��� Node
    Node root_ = nullptr;

    // ���� ���������� �� ������� JSON, ������� ��� �� ���������
    std::vector<Node*> nodes_stack_;

    // ������� ���� �������
    std::string key_;
    // ������� ������� ����� � key_. �.�. key_ ����� ���� "", ��������
    // ������ key_ �� ������� �� �������� ��� ���� �������� �������
    bool key_opened_ = false;

    // ��������. ������������� � ������������
    //BaseContext& base_context_;
};



class BaseContext
{
public:
    BaseContext(Builder& builder) : builder_(builder)
    {}

    KeyContext Key(std::string);
    BaseContext Value(Node::Value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    BaseContext EndDict();
    BaseContext EndArray();
    json::Node Build();

    //    Builder& GetBuilder()
    //    {
    //        return builder_;
    //    }

private:
    Builder& builder_;
};



class KeyContext : public BaseContext
{
public:
    KeyContext(Builder& builder) : BaseContext(builder)
    {}

    KeyValueContext Value(Node::Value);
    ArrayItemContext StartArray();
    DictItemContext StartDict();
    KeyContext Key(std::string) = delete;
    BaseContext EndDict() = delete;
    BaseContext EndArray() = delete;
    json::Node Build() = delete;
};



class ArrayItemContext : public BaseContext
{
public:
    ArrayItemContext(Builder& builder) : BaseContext(builder)
    {}

    ArrayValueContext Value(Node::Value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    KeyContext Key(std::string) = delete;
    BaseContext EndDict() = delete;
    json::Node Build() = delete;
};



class DictItemContext : public BaseContext
{
public:
    DictItemContext(Builder& builder) : BaseContext(builder)
    {}

    KeyContext Key(std::string);
    DictItemContext StartDict() = delete;
    ArrayItemContext StartArray() = delete;
    BaseContext Value(Node::Value) = delete;
    BaseContext EndArray() = delete;
    json::Node Build() = delete;
};



class KeyValueContext : public BaseContext
{
public:
    KeyValueContext(Builder& builder) : BaseContext(builder)
    {}
    KeyValueContext(BaseContext base_context) : BaseContext(base_context)
    {}

    KeyContext Key(std::string);
    DictItemContext StartDict() = delete;
    ArrayItemContext StartArray() = delete;
    BaseContext Value(Node::Value) = delete;
    BaseContext EndArray() = delete;
    json::Node Build() = delete;
};



class ArrayValueContext : public BaseContext
{
public:
    ArrayValueContext(Builder& builder) : BaseContext(builder)
    {}
    ArrayValueContext(BaseContext base_context) : BaseContext(base_context)
    {}

    ArrayValueContext Value(Node::Value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    BaseContext EndArray();
    BaseContext EndDict() = delete;
    KeyContext Key(std::string) = delete;
    json::Node Build() = delete;
};

}