#include "order_book.hpp"
#include <chrono>
#include <stdexcept>
#include <limits>
#include <string>

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

    // Update order on the order book
    current_order.set_status(Status::Canceled);
    Order::Time current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    current_order.set_canceled_time(current_time);

    // Sync history from live to the order journal before removal
    sync_order_to_journal(current_order);

    // Save fields needed after erase, then remove from the live book only
    const OrderId current_order_id = current_order.get_order_id();
    const Price current_price = current_order.get_price();
    const OrderSide current_side = current_order.get_order_side();

    remove_order_from_order_book(current_order_id);
    remove_record_from_order_book_search_map(current_order_id);

    if (current_side == OrderSide::Buy) {
        remove_list_if_no_order(bid_book, bid_book.find(current_price));
    }
    else {
        remove_list_if_no_order(ask_book, ask_book.find(current_price));
    }
}

const Order OrderBook::search_order_book(OrderId id) const {
    std::unordered_map<OrderId, std::list<Order>::iterator>::const_iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        throw std::out_of_range("The order is not found.");
    }
    return *(current_iterator->second);
}

const Order OrderBook::search_order_in_order_journal(OrderId id) const {
    return order_journal.search_order_record(id);
}

const Trade& OrderBook::search_trade_in_trade_journal(TradeId id) const {
    return trade_journal.search_trade(id);
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

Order::OrderId OrderBook::submit_order(OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q) {
    Order current_order = create_order(side, type, t_in_force, p, q);
    match_order(current_order);
    return current_order.get_order_id();
}

void OrderBook::set_order_status_from_order_book(OrderId id, Status s) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_status(s);
}

void OrderBook::set_order_completed_time_from_order_book(OrderId id, Time t) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_completed_time(t);
}

void OrderBook::set_order_canceled_time_from_order_book(OrderId id, Time t) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->set_canceled_time(t);
}

void OrderBook::subtract_order_remaining_quantity_from_order_book(OrderId id, Quantity q) {
    std::unordered_map<OrderId, std::list<Order>::iterator>::iterator current_iterator = order_book_search_map.find(id);
    if (current_iterator == order_book_search_map.end()) {
        return;
    }
    std::list<Order>::iterator current_order_iterator = current_iterator->second;
    current_order_iterator->subtract_remaining_quantity(q);
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
    const TimeInForce tif = order.get_time_in_force();

    // GTC, IOC, and FOK supported for matching
    if (tif != TimeInForce::GTC && tif != TimeInForce::IOC && tif != TimeInForce::FOK) {
        return;
    }

    if (ask_book.empty()) {
        if (tif == TimeInForce::GTC) {
            place_order(order);           // GTC: rest on book
        } else {
            cancel_unrested_order(order);  // IOC/FOK: nowhere to match, then cancel
        }
        return;
    }

    // FOK: match only if full size is available; otherwise kill with no trades
    if (tif == TimeInForce::FOK && !can_fully_fill_buy(order)) {
        cancel_unrested_order(order);
        return;
    }

    /* While the price of order is higher or equal to the best ask and the quantity of the order is greater than  
    or equal to the quantity of the first order in the queue at the best ask, continue to fill the order. */
    while (!ask_book.empty() && order.get_price() >= ask_book.begin()->first
        && order.get_remaining_quantity() >= ask_book.begin()->second.begin()->get_remaining_quantity()) {

        auto current_iterator = ask_book.begin();
        std::list<Order>& current_list = current_iterator->second;
        Order& current_order = current_list.front(); // maker on the order book

        Quantity trade_quantity = current_order.get_remaining_quantity();
        Price current_price = current_iterator->first;
        OrderId maker_order_id = current_order.get_order_id();

        // Record the match event on trade journal
        Trade::TradeId current_trade_id =
            trade_journal.create_trade(maker_order_id, order.get_order_id(), current_price, trade_quantity);
        Time trade_time = trade_journal.get_trade_completed_time(current_trade_id);

        // Apply fill to maker on the order book
        apply_fill_to_maker_on_order_book(maker_order_id, trade_quantity, Status::Filled, trade_time);

        // Decide taker status from remaining quantity after this fill, then apply fill to taker
        Status taker_status = (order.get_remaining_quantity() - trade_quantity == 0) ? Status::Filled : Status::PartiallyFilled;
        apply_fill_to_taker(order, trade_quantity, taker_status, trade_time);

        // Sync history from the live orders to the order journal
        sync_order_to_journal(current_order); // maker
        sync_order_to_journal(order); // taker

        // Remove filled maker from the live book
        current_list.pop_front();
        remove_record_from_order_book_search_map(maker_order_id);
        remove_list_if_no_order(ask_book, current_iterator);
    }
    // Taker leftover is smaller than the front maker: finish the taker,
    // partially fill the maker, leave the maker resting on the book.
    if (!ask_book.empty()
        && order.get_price() >= ask_book.begin()->first
        && order.get_remaining_quantity() > 0
        && order.get_remaining_quantity() < ask_book.begin()->second.begin()->get_remaining_quantity()) {

        auto current_iterator = ask_book.begin();
        Order& current_order = current_iterator->second.front(); // maker on the order book

        // Trade size is all remaining taker quantity as the maker is larger
        Quantity trade_quantity = order.get_remaining_quantity();
        Price current_price = current_iterator->first;
        OrderId maker_order_id = current_order.get_order_id();

        // Record the match event in the trade journal
        Trade::TradeId current_trade_id =
            trade_journal.create_trade(maker_order_id, order.get_order_id(), current_price, trade_quantity);
        Time trade_time = trade_journal.get_trade_completed_time(current_trade_id);

        // Apply partial fill to maker on the order book
        apply_fill_to_maker_on_order_book(maker_order_id, trade_quantity, Status::PartiallyFilled, trade_time);

        // Apply complete fill to taker
        apply_fill_to_taker(order, trade_quantity, Status::Filled, trade_time);

        // Sync history from live orders to the order journal
        sync_order_to_journal(current_order); // maker which is still on the order book
        sync_order_to_journal(order); // taker

        // No pop to the maker as it still has remaining quantity
        return;
    }
    // Leftover handling differs by Time in force
    if (order.get_remaining_quantity() > 0) {
        if (tif == TimeInForce::GTC) {
            place_order(order);
        } else {
            // IOC leftover or FOK kill: do not rest
            cancel_unrested_order(order);
        }
    }
}

void OrderBook::match_sell_order(Order& order) {
    const TimeInForce tif = order.get_time_in_force();

    // GTC, IOC, and FOK supported for matching
    if (tif != TimeInForce::GTC && tif != TimeInForce::IOC && tif != TimeInForce::FOK) {
        return;
    }

    if (bid_book.empty()) {
        if (tif == TimeInForce::GTC) {
            place_order(order);           // GTC: rest on book
        } else {
            cancel_unrested_order(order);  // IOC/FOK: nowhere to match, then cancel
        }
        return;
    }

    // FOK: match only if full size is available, otherwise kill with no trades
    if (tif == TimeInForce::FOK && !can_fully_fill_sell(order)) {
        cancel_unrested_order(order);
        return;
    }
    
    /* While the price of order is lower or equal to the best bid and the quantity of the order is greater than
    or equal to the quantity of the first order in the queue at the best bid, continue to fill the order. */
    while (!bid_book.empty() && order.get_price() <= bid_book.begin()->first
        && order.get_remaining_quantity() >= bid_book.begin()->second.begin()->get_remaining_quantity()) {

        auto current_iterator = bid_book.begin();
        std::list<Order>& current_list = current_iterator->second;
        Order& current_order = current_list.front(); // maker on the order book

        Quantity trade_quantity = current_order.get_remaining_quantity();
        Price current_price = current_iterator->first;
        OrderId maker_order_id = current_order.get_order_id();

        // Record the match event on trade journal
        Trade::TradeId current_trade_id =
            trade_journal.create_trade(maker_order_id, order.get_order_id(), current_price, trade_quantity);
        Time trade_time = trade_journal.get_trade_completed_time(current_trade_id);

        // Apply fill to maker on the order book
        apply_fill_to_maker_on_order_book(maker_order_id, trade_quantity, Status::Filled, trade_time);

        // Decide taker status from remaining quantity after this fill, then apply fill to taker
        Status taker_status = (order.get_remaining_quantity() - trade_quantity == 0) ? Status::Filled : Status::PartiallyFilled;
        apply_fill_to_taker(order, trade_quantity, taker_status, trade_time);

        // Sync history from the live orders to the order journal
        sync_order_to_journal(current_order); // maker
        sync_order_to_journal(order); // taker

        // Remove filled maker from the live book
        current_list.pop_front();
        remove_record_from_order_book_search_map(maker_order_id);
        remove_list_if_no_order(bid_book, current_iterator);
    }
    // If taker leftover is smaller than the front maker, then finish the taker,
    // partially fill the maker, leave the maker resting on the book.
    if (!bid_book.empty()
        && order.get_price() <= bid_book.begin()->first
        && order.get_remaining_quantity() > 0
        && order.get_remaining_quantity() < bid_book.begin()->second.begin()->get_remaining_quantity()) {

        auto current_iterator = bid_book.begin();
        Order& current_order = current_iterator->second.front(); // maker on the order book

        // Trade size is all remaining taker quantity as the maker is larger
        Quantity trade_quantity = order.get_remaining_quantity();
        Price current_price = current_iterator->first;
        OrderId maker_order_id = current_order.get_order_id();

        // Record the match event in the trade journal
        Trade::TradeId current_trade_id =
            trade_journal.create_trade(maker_order_id, order.get_order_id(), current_price, trade_quantity);
        Time trade_time = trade_journal.get_trade_completed_time(current_trade_id);

        // Apply partial fill to maker on the order book
        apply_fill_to_maker_on_order_book(maker_order_id, trade_quantity, Status::PartiallyFilled, trade_time);

        // Apply complete fill to taker
        apply_fill_to_taker(order, trade_quantity, Status::Filled, trade_time);

        // Sync history from live orders to the order journal
        sync_order_to_journal(current_order); // maker which is still on the order book
        sync_order_to_journal(order); // taker

        // Do not pop the maker as it still has remaining quantity
        return;
    }
    if (order.get_remaining_quantity() > 0) {
        if (tif == TimeInForce::GTC) {
            place_order(order);
        } else {
            cancel_unrested_order(order);
        }
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

const std::map<OrderBook::Price, std::list<Order>, std::greater<OrderBook::Price>>& OrderBook::get_bid_book() const {
    return bid_book;
}

const std::map<OrderBook::Price, std::list<Order>>& OrderBook::get_ask_book() const {
    return ask_book;
}

void OrderBook::apply_fill_to_taker(Order& taker, Quantity trade_qty, Status new_status, Time t) {
    taker.subtract_remaining_quantity(trade_qty);
    taker.set_status(new_status);
    if (new_status == Status::Filled) {
        taker.set_completed_time(t);
    }
}

void OrderBook::apply_fill_to_maker_on_order_book(OrderId maker_id, Quantity trade_qty, Status new_status, Time t) {
    subtract_order_remaining_quantity_from_order_book(maker_id, trade_qty);
    set_order_status_from_order_book(maker_id, new_status);
    if (new_status == Status::Filled) {
        set_order_completed_time_from_order_book(maker_id, t);
    }
}

void OrderBook::sync_order_to_journal(const Order& live_order) {
    OrderId id = live_order.get_order_id();
    order_journal.set_status_in_order_journal(id, live_order.get_status());
    order_journal.set_remaining_quantity_in_order_journal(id, live_order.get_remaining_quantity());
    order_journal.set_completed_time_in_order_journal(id, live_order.get_completed_time());
    order_journal.set_canceled_time_in_order_journal(id, live_order.get_canceled_time());
}

void OrderBook::cancel_unrested_order(Order& order) {
    if (order.get_remaining_quantity() == 0) {
        return; // fully filled, therefore nothing to cancel
    }
    order.set_status(Status::Canceled);
    Order::Time current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    order.set_canceled_time(current_time);
    // Live directly sync to journal as order was never rested on the order book
    sync_order_to_journal(order);
}

// check the sum of ask size at prices the buy can fill in order to validate if a fully fill is possible. Read-only.
bool OrderBook::can_fully_fill_buy(const Order& order) const {
    Quantity needed = order.get_remaining_quantity();

    for (auto level = ask_book.begin(); level != ask_book.end(); ++level) {
        // Cannot take asks above the buy's limit
        if (level->first > order.get_price()) {
            break;
        }
        for (const Order& maker : level->second) {
            Quantity available = maker.get_remaining_quantity();
            if (available >= needed) {
                return true; // enough liquidity found
            }
            needed -= available;
        }
    }
    return needed == 0;
}

// check the sum of bid size at prices the sell can fill in order to validate if a fully fill is possible. Read-only.
bool OrderBook::can_fully_fill_sell(const Order& order) const {
    Quantity needed = order.get_remaining_quantity();

    for (auto level = bid_book.begin(); level != bid_book.end(); ++level) {
        // Cannot take bids below the sell's limit
        if (level->first < order.get_price()) {
            break;
        }
        for (const Order& maker : level->second) {
            Quantity available = maker.get_remaining_quantity();
            if (available >= needed) {
                return true;
            }
            needed -= available;
        }
    }
    return needed == 0;
}