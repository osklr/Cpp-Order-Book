#include "trade_journal.hpp"
#include <chrono>
#include <stdexcept>

// Create trade to the trade journal, return trade id
Trade::TradeId TradeJournal::create_trade(OrderId maker_id, OrderId taker_id, Price p, Quantity q) {
    TradeId current_trade_id = ++trade_id_counter;
    Time current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    trade_journal.emplace_back(current_trade_id, current_time, maker_id, taker_id, p, q);
    trade_search_book[current_trade_id] = trade_journal.size() - 1;
    return current_trade_id;
}

// Search information of a trade in trade journal with trade id
Trade TradeJournal::search_trade(TradeId id) const {
    std::unordered_map<TradeId, VectorIndex>::const_iterator current_iterator = trade_search_book.find(id);
    if (current_iterator != trade_search_book.end()) {
        return trade_journal[current_iterator->second];
    }
    throw std::out_of_range("No trade is found.");
}

Trade::Time TradeJournal::get_trade_completed_time(TradeId id) const {
    Trade current_trade = search_trade(id);
    return current_trade.get_timestamp();
}