#pragma once
#include <cstdint>

enum class OrderType {
    Buy,
    Sell
};

class Order {
    public:
        using Price = int64_t;
        using OrderId = uint64_t;
        using Quantity = int;
        
    private:
        OrderId order_id;
        OrderType order_type;
        Price price;
        Quantity quantity;

    public:
        Order(OrderId id, OrderType type, Price p, Quantity q) : order_id(id), order_type(type), price(p), quantity(q) {}
        OrderId get_order_id() const { return order_id; }
        OrderType get_order_type() const {return order_type; }
        Price get_price() const { return price; }
        Quantity get_quantity() const { return quantity; }
};