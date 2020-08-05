#include <hot/guest/rawkit/serial.h>

#include <vector>
#include <algorithm>
#include <iterator>
#include <serial/serial.h>
#include <hot/jitjob.h>

using namespace std;

typedef serial::Serial * OptionalSerial;

vector<OptionalSerial> serial_ports;

SerialID Serial_Open(const char *portName) {
  auto it = find_if(
    serial_ports.begin(),
    serial_ports.end(),
    [portName](OptionalSerial port) {
      if (port == nullptr) {
        return false;
      }

      return port->getPort() == portName;
  });

  if (it != serial_ports.end()) {
    SerialID d = distance(serial_ports.begin(), it);
    return d;
  }

  try {
    OptionalSerial port = new serial::Serial(
      portName,
      115200,
      serial::Timeout::simpleTimeout(1)
    );

    SerialID index = serial_ports.size();
    serial_ports.push_back(port);
    return index;
  } catch (serial::IOException e) {
    return -1;
  }
}

inline OptionalSerial Serial_Valid(SerialID id) {
  
  if (serial_ports.size() <= id) {
    return nullptr;
  }
  
  OptionalSerial sp = serial_ports[id];
  
  if (sp == nullptr) {
    return nullptr;
  }

  if (!sp->isOpen()) {
    try {
      sp->open();
      if (!sp->isOpen()) {
        return nullptr; 
      }
    } catch (serial::IOException e) {
      return nullptr;
    }
  }

  return sp;
}

size_t Serial_Available(SerialID id){
  OptionalSerial sp = Serial_Valid(id);
  if (sp == nullptr) {
    return 0;
  }
  
  try {
    return sp->available();
  } catch (serial::IOException e) {
    sp->close();
    return 0;
  }
}

int16_t Serial_Read(SerialID id) {
  OptionalSerial sp = Serial_Valid(id);
  if (sp == nullptr) {
    return -1;
  }

  uint8_t out = 0;
  try  {
    sp->read(&out, 1);
    return static_cast<int16_t>(out);
  } catch (serial::IOException e) {
    sp->close();
    return -1;
  }
}

void host_rawkit_serial_init(JitJob *job) {
  job->addExport("Serial_Open", Serial_Open);
  job->addExport("Serial_Available", Serial_Available);
  job->addExport("Serial_Read", Serial_Read);
}

