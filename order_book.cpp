#include "order_book.hpp"
#include "trade_journal.hpp"
#include "order_journal.hpp"
#include <chrono>

OrderBook::OrderId OrderBook::create_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q) {
    OrderId current_order_id = ++order_id_counter;
    Order::Time current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    Order current_order(current_order_id, side, type, t_in_force, p, q, current_time);
    
    return current_order_id;
}



void OrderBook::cancel_order(OrderId id) {

}

void OrderBook::modify_order(OrderId id) {

}

void OrderBook::match_order(Order order) {
    // Decide whether it is Buy or Sell
    if (order.get_order_type() == OrderType::Stop || order.get_order_type() == OrderType::StopLimit) {
        if (order.get_order_side() == OrderSide::Buy) {
            // If buy, then check if the ask book is empty. If yes, direct emplace a new order in the bid book. If not, check if the order could be filled.
            if (ask_book.empty()) {
                auto [price_n_list, list_created] = bid_book.try_emplace(order.get_price());
                auto& current_list = price_n_list->second;
                current_list.emplace_back(order.get_order_id(), order.get_order_type(), order.get_price(), order.get_quantity());
                order_book_search_map[order.get_order_id()] = --current_list.end();
            }
            else {
                /* While the price of order is higher or equal to the best ask and the quantity of the order is larger 
                than the quantity of the first order in the queue at the best ask, continue to fill the order. */
                while (order.get_price() >= ask_book.begin()->first && order.get_quantity() >= ask_book.begin()->second.begin()->get_quantity()) {
                    create_trade();
                }
            }
        }
        else {
            // If Sell, then check if the bid book is empty. If yes, direct emplace a new order in the ask book. If not, check if the order could be filled.
            if (bid_book.empty()) {
                auto [price_n_list, list_created] = ask_book.try_emplace(order.get_price());
                auto& current_list = price_n_list->second;
                current_list.emplace_back(order.get_order_id(), order.get_order_type(), order.get_price(), order.get_quantity());
                order_book_search_map[order.get_order_id()] = --current_list.end();
            }
            else {

            }
        }
    }
    else if (order.get_order_type() == OrderType::Limit || order.get_order_type() == OrderType::Market) {

    }
}

Order OrderBook::search_order_book(OrderId id) const {

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


