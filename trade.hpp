#pragma once
#include <cstdint>

class Trade {
    public:
        using TradeId = uint64_t;
        using Time = uint64_t;
        using OrderId = uint64_t;
        using Price = int64_t;
        using Quantity = uint32_t;

    private:
        TradeId trade_id;
        Time timestamp;
        OrderId maker_order_id;
        OrderId taker_order_id;
        Price price;
        Quantity quantity;

    public:
        Trade(TradeId id, Time t, OrderId maker_id, OrderId taker_id, Price p, Quantity q) : trade_id(id), timestamp(t), maker_order_id(maker_id), taker_order_id(taker_id), price(p), quantity(q) {}

        // Get functions
        TradeId get_trade_id() const { return trade_id; }
        Time get_timestamp() const {return timestamp; }
        OrderId get_maker_order_id() const { return maker_order_id; }
        OrderId get_taker_order_id() const { return taker_order_id; }
        Price get_price() const { return price; }
        Quantity get_quantity() const { return quantity; }
};