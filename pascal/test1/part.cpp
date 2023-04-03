module;

#include <iostream>
#include <string>

module m:impl_part;
import :interface_part;

std::string w = "World.";
void world() {
  std::cout << w << std::endl;
}