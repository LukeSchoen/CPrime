#include <chrono>
#include <ctime>
#include <thread>
int main() {
  using namespace std::chrono;
  if (duration_cast<microseconds>(milliseconds(123)).count()!=123000) return 1;
  if (duration_cast<seconds>(milliseconds(-1999)).count()!=-1) return 2;
  if (duration_cast<duration<double>>(milliseconds(1500)).count()!=1.5) return 3;
  auto before=steady_clock::now();
  std::this_thread::sleep_for(milliseconds(15));
  auto after=steady_clock::now();
  if (duration_cast<milliseconds>(after-before).count()<10) return 4;
  auto wall=system_clock::to_time_t(system_clock::now());
  auto crt=std::time(nullptr);
  if (wall<crt-2 || wall>crt+2) return 5;
  if (system_clock::to_time_t(system_clock::from_time_t(12345))!=12345) return 6;
  return 0;
}
