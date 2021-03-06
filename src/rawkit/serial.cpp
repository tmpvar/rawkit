#include <hot/guest/rawkit/serial.h>

#include <vector>
#include <algorithm>
#include <iterator>
#include <serial/serial.h>
#include <rawkit/jit.h>

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
      // TODO: was seeing a error:121 on windows randomly, so this value might need to be tuned.
      // node-serialport had a similar problem: https://github.com/serialport/node-serialport/issues/781
      serial::Timeout::simpleTimeout(0)
    );

    SerialID index = serial_ports.size();
    serial_ports.push_back(port);
    return index;
  } catch (serial::IOException &e) {
    return -1;
  }
}

inline OptionalSerial GetSerialPortById(SerialID id) {

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
    } catch (serial::IOException &e) {
      return nullptr;
    }
  }

  return sp;
}

size_t Serial_Available(SerialID id){
  OptionalSerial sp = GetSerialPortById(id);
  if (sp == nullptr) {
    return 0;
  }

  try {
    return sp->available();
  } catch (serial::IOException &e) {
    sp->close();
    return 0;
  }
}

bool Serial_Valid(SerialID id){
  OptionalSerial sp = GetSerialPortById(id);
  if (sp == nullptr) {
    return false;
  }
  return true;
}

int16_t Serial_Read(SerialID id) {
  OptionalSerial sp = GetSerialPortById(id);
  if (sp == nullptr) {
    return -1;
  }

  uint8_t out = 0;
  try  {
    sp->read(&out, 1);
    return static_cast<int16_t>(out);
  } catch (serial::IOException &e) {
    printf("closing serialport because read failed\n");
    sp->close();
    return -1;
  }
}

void Serial_Write(SerialID id, const uint8_t *buf, size_t len) {
  OptionalSerial sp = GetSerialPortById(id);
  if (sp == nullptr) {
    return;
  }

  try  {
    sp->write(buf, len);
  } catch (serial::IOException &e) {
    printf("closing serialport because write failed (len=%zu) (%s)\n", len, (char *)buf);
    printf("error (%i): %s\n\n", e.getErrorNumber(), e.what());
    //sp->close();
  }
}


void host_rawkit_serial_init(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "Serial_Open", (void *)&Serial_Open);
  rawkit_jit_add_export(jit, "Serial_Valid", (void *)&Serial_Valid);
  rawkit_jit_add_export(jit, "Serial_Available", (void *)&Serial_Available);
  rawkit_jit_add_export(jit, "Serial_Read", (void *)&Serial_Read);
  rawkit_jit_add_export(jit, "Serial_Write", (void *)&Serial_Write);
}

