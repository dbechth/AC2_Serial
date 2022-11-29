// AC2_Serial.h

#ifndef _AC2_Serial_h
#define _AC2_Serial_h

#include <Arduino.h>
#define ACBufferSize   16

class AC2_SerialClass
{
protected:
    void ReadMessages();
    void SendMessages();
    void HandleWriteCommand(String command);
    void HandleReadCommand(String command);

public:
    enum IOType
    {
        DigitalInput,
        DigitalOutput,
        AnalogInput,
        AnalogOutput,
        Data
    };

    enum TimeScale
    {
        TimeScaleMilliseconds,
        TimeScaleSeconds,
        TimeScaleMinutes,
        TimeScaleHours
    };

    struct IO
    {
        String Name;
        int Pin;
        long Timeout;
        long DataTimer;
        IOType Type;
        int DefaultValue;
        int Value;
        boolean Invert;
    };
    struct Device
    {
        String Name;
        int Port;
        IO Data[16];
    };

    bool init(String name, int TaskRateMS);
    void task();
    void WriteIO(String DeviceName, IO io);
    void LocalWriteIO(IO io);
    int  LocalReadIO(String Name);
    void ReadIO(String DeviceName, IO io);
    void HandleIO();
    Device device;
    unsigned long lastTime;
    unsigned long timeNow;
    bool EnableOTA;
    String buffer[ACBufferSize];
    int bufferCount;
};

extern AC2_SerialClass AC2_Serial;

#endif

