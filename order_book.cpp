#include "order_book.hpp"
#include <chrono>

OrderBook::Price OrderBook::get_best_bid() const {
    if (bid_book.empty()) {
        throw std::out_of_range("Bid book is empty.");
    }
    return bid_book.begin()->first;
}

OrderBook::Price OrderBook::get_best_ask() const {
    if (ask_book.empty()) {
        throw std::out_of_range("Ask book is empty.");
    }
    return ask_book.begin()->first;
}

OrderBook::Price OrderBook::get_spread() const {
    if (bid_book.empty() || ask_book.empty()) {
        return 0;
    }
    return get_best_ask() - get_best_bid();
}

std::list<Order>& OrderBook::get_list_with_price_level(Price p, OrderSide s) {
    if (s == OrderSide::Buy) {
        std::map<Price, std::list<Order>, std::greater<Price>>::iterator current_iterator = bid_book.find(p);
        if (current_iterator == bid_book.end()) {
            throw std::out_of_range("The list of this price level is not found.");
        }
        return current_iterator->second;
    }
    else {
        std::map<Price, std::list<Order>>::iterator current_iterator = ask_book.find(p);
        if (current_iterator == ask_book.end()) {
            throw std::out_of_range("The list of this price level is not found.");
        }
        return current_iterator->second;
    }
}

Order OrderBook::create_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q) {
    OrderId current_order_id = ++order_id_counter;
    Order::Time current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    order_journal.create_order_record(current_order_id, side, type, t_in_force, p, q, current_time);
    Order current_order(current_order_id, side, type, t_in_force, p, q, current_time);
    return current_order;
}

void OrderBook::place_order(Order& order) {
    if (order.get_order_side() == OrderSide::Buy) {
        auto [price_n_list, list_created] = bid_book.try_emplace(order.get_price());
        auto& current_list = price_n_list->second;
        current_list.emplace_back(order);
        order_book_search_map[order.get_order_id()] = std::prev(current_list.end());
    }
    else {
        auto [price_n_list, list_created] = ask_book.try_emplace(order.get_price());
        auto& current_list = price_n_list->second;
        current_list.emplace_back(order);
        order_book_search_map[order.get_order_id()] = std::prev(current_list.end());
    }
}

void OrderBook::cancel_order(OrderId id) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    Order& current_order = *(current_iterator->second);
    // If the order is already completely filled or canceled, do nothing
    if (current_order.get_status() == Status::Filled || current_order.get_status() == Status::Canceled) {
        return;
    }
    // Set status of the order to canceled in the order book and the order journal
    current_order.set_status(Status::Canceled);
    const OrderId current_order_id = current_order.get_order_id();
    order_journal.set_status_in_order_journal(current_order_id, Status::Canceled);
    // Set canceled time of the order in the order book and order journal
    Order::Time current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    current_order.set_canceled_time(current_time);
    order_journal.set_canceled_time_in_order_journal(current_order_id, current_time);
    // Remove order from a book depending on the order side
    if (current_order.get_order_side() == OrderSide::Buy) {
        const Price current_price = current_order.get_price();
        // Remove order
        remove_order_from_order_book(current_order_id);
        // Remove record of order from the order book search map
        remove_record_from_order_book_search_map(current_order_id);
        std::map<Price, std::list<Order>, std::greater<Price>>::iterator current_map_iterator = bid_book.find(current_price);
        // If list at that price is empty on the bid book after the cancellation, remove the list of that price level on the bid book
        remove_list_if_no_order(bid_book, current_map_iterator);
    }
    else {
        const Price current_price = current_order.get_price();
        // Remove order
        remove_order_from_order_book(current_order_id);
        // Remove record of order from the order book search map
        remove_record_from_order_book_search_map(current_order_id);
        std::map<Price, std::list<Order>>::iterator current_map_iterator = ask_book.find(current_price);
        // If list at that price is empty on the ask book after the cancellation, remove the list of that price level on the ask book
        remove_list_if_no_order(ask_book, current_map_iterator);
    }
}

const Order OrderBook::search_order_book(OrderId id) const {
    std::unordered_map<OrderId, std::list<Order>::iterator>::const_iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        throw std::out_of_range("The order is not found.");
    }
    return *(current_iterator->second);
}

void OrderBook::match_order(Order order) {
    // Determine its order type
    if (order.get_order_type() == OrderType::Stop || order.get_order_type() == OrderType::StopLimit) {
        // Handle if stop or stop-limit type
        handle_stop_order(order);
    }
    else if (order.get_order_type() == OrderType::Limit || order.get_order_type() == OrderType::Market) {
        // Decide whether it is Buy or Sell if it is limit or market order type
        if (order.get_order_side() == OrderSide::Buy) {
            match_buy_order(order);
        }
        else {
            match_sell_order(order);
        }
    }
}

void OrderBook::submit_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q) {
    Order current_order = create_order(side, type, t_in_force, p, q);
    match_order(current_order);
}

void OrderBook::set_order_status(OrderId id, Status s) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_status(s);
    order_journal.set_status_in_order_journal(id, s);
}

void OrderBook::set_order_completed_time(OrderId id, Time t) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_completed_time(t);
    order_journal.set_completed_time_in_order_journal(id, t);
}

void OrderBook::set_order_canceled_time(OrderId id, Time t) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_canceled_time(t);
    order_journal.set_canceled_time_in_order_journal(id, t);
}

void OrderBook::add_order_filled_quantity(OrderId id, Quantity q) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->add_filled_quantity(q);
    order_journal.add_filled_quantity_in_order_journal(id, q);
}

void OrderBook::remove_record_from_order_book_search_map(OrderId id) {
    if (order_book_search_map.find(id) == order_book_search_map.end()) {
        return;
    }
    order_book_search_map.erase(id);
}

void OrderBook::remove_order_from_order_book(OrderId id) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    Order& current_order = *(current_iterator->second);
    std::list<Order>& current_list = get_list_with_price_level(current_order.get_price(), current_order.get_order_side());
    current_list.erase(current_iterator->second);
}

void OrderBook::match_buy_order(Order& order) {
    // Buy side, check time in force
    if (order.get_time_in_force() == TimeInForce::GTC) {
        // Check if the ask book is empty. If yes, direct emplace a new order in the bid book. If not, check if the order could be filled.
        if (ask_book.empty()) {
            place_order(order);
        }
        else {
            /* While the price of order is higher or equal to the best ask and the quantity of the order is larger 
            than the quantity of the first order in the queue at the best ask, continue to fill the order. */
            Quantity remaining_quantity = order.get_quantity();
            while (!ask_book.empty() && order.get_price() >= ask_book.begin()->first && remaining_quantity >= ask_book.begin()->second.begin()->get_quantity()) {
                std::map<Price, std::list<Order>>::iterator current_iterator = ask_book.begin();
                std::list<Order>& current_list = current_iterator->second;
                Order& current_order = current_list.front();
                Quantity trade_quantity = current_order.get_quantity();
                Price current_price = current_iterator->first;
                // Create a new trade record in the trade journal
                Trade::TradeId current_trade_id = trade_journal.create_trade(current_order.get_order_id(), order.get_order_id(), current_price, trade_quantity);
                Time current_completed_time = trade_journal.get_trade_completed_time(current_trade_id);
                // Set completed time in both order book and order journal for maker order
                current_order.set_completed_time(current_completed_time);
                // Add filled quantity in both order book and order journal for both orders
                current_order.add_filled_quantity(trade_quantity);
                order.add_filled_quantity(trade_quantity);
                // Subtract the remaining quantity for the taker order
                remaining_quantity -= trade_quantity;
                // Check if all quantity is filled for the taker order. If yes, set its status as Filled and set completed time. If not, set status as partially filled.
                if (remaining_quantity == 0 && order.get_quantity() == order.get_filled_quantity()) {
                    order.set_status(Status::Filled);
                    order.set_completed_time(current_completed_time);
                }
                else {
                    order.set_status(Status::PartiallyFilled);
                }
                // Remove the maker order from the order book
                current_list.pop_front();
                // Remove the maker order from the order book search map
                remove_record_from_order_book_search_map(current_order.get_order_id());
                // Remove the list at the trade price from the ask book if no order left at that price
                remove_list_if_no_order(ask_book, current_iterator);
            }
            if (!ask_book.empty() && order.get_price() >= ask_book.begin()->first && remaining_quantity < ask_book.begin()->second.begin()->get_quantity()) {
                // Fill the remaining quantity of the taker order
                std::map<Price, std::list<Order>>::iterator current_iterator = ask_book.begin();
                Order& current_order = current_iterator->second.front();
                Quantity trade_quantity = remaining_quantity;
                Price current_price = current_iterator->first;
                // Create a new trade record in the trade journal
                Trade::TradeId current_trade_id = trade_journal.create_trade(current_order.get_order_id(), order.get_order_id(), current_price, trade_quantity);
                Time current_completed_time = trade_journal.get_trade_completed_time(current_trade_id);
                // Set completed time in both order book and order journal for taker order
                order.set_completed_time(current_completed_time);
                // Add filled quantity in both order book and order journal for both orders
                current_order.add_filled_quantity(trade_quantity);
                order.add_filled_quantity(trade_quantity);
                // Subtract the remaining quantity for the taker order
                remaining_quantity -= trade_quantity;
                // Taker order is completely filled, set status to filled
                order.set_status(Status::Filled);
                return;
            }
            if (remaining_quantity > 0) {
                place_order(order);
            }
        }
    }
    else {
        return;
    }
}

void OrderBook::match_sell_order(Order& order) {
    // Sell side, check time in force
    if (order.get_time_in_force() == TimeInForce::GTC) {
        // Check if the bid book is empty. If yes, direct emplace a new order in the ask book. If not, check if the order could be filled.
        if (bid_book.empty()) {
            place_order(order);
        }
        else {
            /* While the price of order is lower or equal to the best bid and the quantity of the order is larger 
            than the quantity of the first order in the queue at the best bid, continue to fill the order. */
            Quantity remaining_quantity = order.get_quantity();
            while (!bid_book.empty() && order.get_price() <= bid_book.begin()->first && remaining_quantity >= bid_book.begin()->second.begin()->get_quantity()) {
                std::map<Price, std::list<Order>, std::greater<Price>>::iterator current_iterator = bid_book.begin();
                std::list<Order>& current_list = current_iterator->second;
                Order& current_order = current_list.front();
                Quantity trade_quantity = current_order.get_quantity();
                Price current_price = current_iterator->first;
                // Create a new trade record in the trade journal
                Trade::TradeId current_trade_id = trade_journal.create_trade(current_order.get_order_id(), order.get_order_id(), current_price, trade_quantity);
                Time current_completed_time = trade_journal.get_trade_completed_time(current_trade_id);
                // Set completed time in both order book and order journal for maker order
                current_order.set_completed_time(current_completed_time);
                // Add filled quantity in both order book and order journal for both orders
                current_order.add_filled_quantity(trade_quantity);
                order.add_filled_quantity(trade_quantity);
                // Subtract the remaining quantity for the taker order
                remaining_quantity -= trade_quantity;
                // Check if all quantity is filled for the taker order. If yes, set its status as Filled and set completed time. If not, set status as partially filled.
                if (remaining_quantity == 0 && order.get_quantity() == order.get_filled_quantity()) {
                    order.set_status(Status::Filled);
                    order.set_completed_time(current_completed_time);
                }
                else {
                    order.set_status(Status::PartiallyFilled);
                }
                // Remove the maker order from the order book
                current_list.pop_front();
                // Remove the maker order from the order book search map
                remove_record_from_order_book_search_map(current_order.get_order_id());
                // Remove the list at the trade price from the bid book if no order left at that price
                remove_list_if_no_order(bid_book, current_iterator);
            }
            if (!bid_book.empty() && order.get_price() <= bid_book.begin()->first && remaining_quantity < bid_book.begin()->second.begin()->get_quantity()) {
                // Fill the remaining quantity of the taker order
                std::map<Price, std::list<Order>, std::greater<Price>>::iterator current_iterator = bid_book.begin();
                Order& current_order = current_iterator->second.front();
                Quantity trade_quantity = remaining_quantity;
                Price current_price = current_iterator->first;
                // Create a new trade record in the trade journal
                Trade::TradeId current_trade_id = trade_journal.create_trade(current_order.get_order_id(), order.get_order_id(), current_price, trade_quantity);
                Time current_completed_time = trade_journal.get_trade_completed_time(current_trade_id);
                // Set completed time in both order book and order journal for taker order
                order.set_completed_time(current_completed_time);
                // Add filled quantity in both order book and order journal for both orders
                current_order.add_filled_quantity(trade_quantity);
                order.add_filled_quantity(trade_quantity);
                // Subtract the remaining quantity for the taker order
                remaining_quantity -= trade_quantity;
                // Taker order is completely filled, set status to filled
                order.set_status(Status::Filled);
                return;
            }
            if (remaining_quantity > 0) {
                place_order(order);
            }
        }
    }
    else {
        return;
    }
}

void OrderBook::handle_stop_order(Order& order) {
    return; // Deferred
}

void OrderBook::finalize_filled_order(Order& order) {
    return; // Deferred
}

void OrderBook::finalize_canceled_order(Order& order) {
    return; // Deferred
}


