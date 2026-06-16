#include "order_book.hpp"

OrderBook::OrderId OrderBook::create_order(OrderType type, Price p, Quantity q) {
    OrderId current_id = ++order_id_counter;
    if (type == OrderType::Buy) {
        if (ask_book.empty()) {
            auto [price_n_list, list_created] = bid_book.try_emplace(p);
            auto& current_list = price_n_list->second;
            current_list.emplace_back(current_id, type, p, q);
            search_book[current_id] = --current_list.end();
        }
        else {
            while (p >= ask_book.begin()->first && q >= ask_book.begin()->second.begin()->get_quantity()) {
                
            }
        }
    }
    else {
        if (bid_book.empty()) {
            auto [price_n_list, list_created] = ask_book.try_emplace(p);
            auto& current_list = price_n_list->second;
            current_list.emplace_back(current_id, type, p, q);
            search_book[current_id] = --current_list.end();
        }
        else {

        }
    }
}

void OrderBook::cancel_order(OrderId id) {

}

void OrderBook::modify_order(OrderId id) {

}

OrderBook::Price OrderBook::get_best_bid() const {
    return bid_book.begin()->first;
}

OrderBook::Price OrderBook::get_best_ask() const {
    return ask_book.begin()->first;
}

OrderBook::Price OrderBook::get_spread() const {
    if (bid_book.empty() || ask_book.empty()) {
        return 0;
    }
    return get_best_ask() - get_best_bid();
}


