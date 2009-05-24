#pragma once

#include <string>

/// Structure to hold information about a single stock.
struct stock
{
  std::string code;
  std::string name;
  double open_price;
  double high_price;
  double low_price;
  double last_price;
  double buy_price;
  int buy_quantity;
  double sell_price;
  int sell_quantity;

  template <typename Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    ar & code;
    ar & name;
    ar & open_price;
    ar & high_price;
    ar & low_price;
    ar & last_price;
    ar & buy_price;
    ar & buy_quantity;
    ar & sell_price;
    ar & sell_quantity;
  }
};

std::ostream& operator << (std::ostream& ostrm, const stock& obj)
{
	ostrm << "  code: " << obj.code << "\n";
	ostrm << "  name: " << obj.name << "\n";
	ostrm << "  open_price: " << obj.open_price << "\n";
	ostrm << "  high_price: " << obj.high_price << "\n";
	ostrm << "  low_price: " << obj.low_price << "\n";
	ostrm << "  last_price: " << obj.last_price << "\n";
	ostrm << "  buy_price: " << obj.buy_price << "\n";
	ostrm << "  buy_quantity: " << obj.buy_quantity << "\n";
	ostrm << "  sell_price: " << obj.sell_price << "\n";
	ostrm << "  sell_quantity: " << obj.sell_quantity << "\n";
	return ostrm;
}