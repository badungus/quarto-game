#include "engine.hpp"
#include "uqi.hpp"

int main() {
  quarto::Engine engine;
  uqi_loop(engine);
  return 0;
}
