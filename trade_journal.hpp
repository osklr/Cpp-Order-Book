#pragma once
#include "trade.hpp"
#include <vector>
#include <unordered_map>

class TradeJournal {
    public:
        using TradeId = Trade::TradeId;
        using Time = Trade::Time;
        using OrderId = Trade::OrderId;
        using Price = Trade::Price;
        using Quantity = Trade::Quantity;
        using VectorIndex = size_t;
    
    private:
        std::vector<Trade> trade_journal;
        std::unordered_map<TradeId, VectorIndex> trade_search_book;
        TradeId trade_id_counter{0};

    public:
        TradeJournal() = default;

        TradeId create_trade(OrderId maker_id, OrderId taker_id, Price p, Quantity q);
        Trade search_trade(TradeId id) const;
        Time get_trade_completed_time(TradeId id) const;
};