#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include <iostream>

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Producer", "1.0", "A comand-line version of a dummy Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "Test", "string",
      "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    auto app=eudaq::Factory<eudaq::Producer>::MakeShared<const std::string&,const std::string&>
      (eudaq::cstr2hash("TestProducer"), name.Value(), rctrl.Value());
    app->Exec();
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
