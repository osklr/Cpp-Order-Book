#include "cli.hpp"

int main() {
    OrderBook book;
    Cli cli(book);
    cli.run_cli();
    return 0;
}