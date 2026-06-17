#pragma once
#include <cstdint>

enum class OrderSide {
    Buy,
    Sell
};

enum class OrderType {
    Limit,
    Market,
    Stop,
    StopLimit
};

enum class TimeInForce {
    GFD, // Good For Day
    GTC, // Good Till Cancelled
    IOC, // Immediate Or Cancel
    FOK  // Fill Or Kill
};

enum class Status : uint8_t {
    New,
    PartiallyFilled,
    Filled,
    Canceled,
    Rejected
};

class Order {
    public:
        using Price = int64_t;
        using OrderId = uint64_t;
        using Quantity = uint32_t;
        using Time = uint64_t;
        
    private:
        OrderId order_id;
        OrderSide order_side;
        OrderType order_type;
        TimeInForce time_in_force;
        Price price;
        Quantity quantity;
        Quantity filled_quantity;
        Status status;
        Time created_time;
        Time completed_time;
        Time canceled_time;

    public:
        // Constructor
        Order(
            OrderId id,
            OrderSide side, 
            OrderType type, 
            TimeInForce t_in_force, 
            Price p, 
            Quantity q,
            Time created_t
        ) 
            : order_id(id)
            , order_side(side)
            , order_type(type)
            , time_in_force(t_in_force)
            , price(p)
            , quantity(q)
            , filled_quantity(0)
            , status(Status::New)
            , created_time(created_t)
            , completed_time(0)
            , canceled_time(0) 
        {}

        // Get functions
        OrderId get_order_id() const { return order_id; }
        OrderSide get_order_side() const { return order_side; }
        OrderType get_order_type() const { return order_type; }
        TimeInForce get_time_in_force() const { return time_in_force; }
        Price get_price() const { return price; }
        Quantity get_quantity() const { return quantity; }
        Quantity get_filled_quantity() const { return filled_quantity; }
        Status get_status() const { return status; }
        Time get_created_time() const { return created_time; }
        Time get_completed_time() const { return completed_time; }
        Time get_canceled_time() const { return canceled_time; }

        // Set functions
        void set_status(Status s) { status = s; }
        void set_completed_time(Time t) { completed_time = t; }
        void set_canceled_time(Time t) { canceled_time = t; }
        void add_filled_quantity(Quantity q) { filled_quantity += q; }
};