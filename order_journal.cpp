#include "order_journal.hpp"
#include <stdexcept>

// Create the record and put it into the order journal and the order search book
void OrderJournal::create_order_record(OrderId id, OrderSide side, OrderType type, TimeInForce t_in_force, Price p, Quantity q, Time current_t) {
    order_journal.emplace_back(id, side, type, t_in_force, p, q, current_t);
    order_journal_search_book[id] = order_journal.size() - 1;
}

// Search the order with a order id in the search book
const Order& OrderJournal::search_order_record(const OrderId& id) const {
    std::unordered_map<OrderId, VectorIndex>::const_iterator current_iterator = order_journal_search_book.find(id);
    if (current_iterator == order_journal_search_book.end()) {
        throw std::out_of_range("The order record is not found.");
    }
    return order_journal[current_iterator->second];
}

void OrderJournal::set_canceled_time_in_order_journal(OrderId id, Time canceled_t) {
    std::unordered_map<OrderId, VectorIndex>::const_iterator current_iterator = order_journal_search_book.find(id);
    if (current_iterator == order_journal_search_book.end()) {
        throw std::out_of_range("The order record is not found.");
    }
    order_journal[current_iterator->second].set_canceled_time(canceled_t);
}

void OrderJournal::set_completed_time_in_order_journal(OrderId id, Time completed_t) {
    std::unordered_map<OrderId, VectorIndex>::const_iterator current_iterator = order_journal_search_book.find(id);
    if (current_iterator == order_journal_search_book.end()) {
        throw std::out_of_range("The order record is not found.");
    }
    order_journal[current_iterator->second].set_completed_time(completed_t);
}

void OrderJournal::set_status_in_order_journal(OrderId id, Status s) {
    std::unordered_map<OrderId, VectorIndex>::const_iterator current_iterator = order_journal_search_book.find(id);
    if (current_iterator == order_journal_search_book.end()) {
        throw std::out_of_range("The order record is not found.");
    }
    order_journal[current_iterator->second].set_status(s);
}

void OrderJournal::subtract_remaining_quantity_in_order_journal(OrderId id, Quantity q) {
    std::unordered_map<OrderId, VectorIndex>::const_iterator current_iterator = order_journal_search_book.find(id);
    if (current_iterator == order_journal_search_book.end()) {
        throw std::out_of_range("The order record is not found.");
    }
    order_journal[current_iterator->second].subtract_remaining_quantity(q);
}
        