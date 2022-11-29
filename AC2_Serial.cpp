#include "AC2_Serial.h"

unsigned long taskRate;
unsigned long taskCount;

bool AC2_SerialClass::init(String name, int TaskRateMS)
{
	device.Name = name;
	taskRate = TaskRateMS;

	bufferCount = 0;
	//get started off right
	timeNow = millis();
	lastTime = timeNow;
	return true;
}


void AC2_SerialClass::task()
{
	timeNow = millis();
	unsigned long elapsedTime = timeNow - lastTime;
	if (elapsedTime >= taskRate) {
		lastTime = timeNow;
		ReadMessages();
		HandleIO();
		taskCount++;
	}
}

void AC2_SerialClass::HandleIO()
{
	for (int i = 0; i < 16; i++)
	{

		if (device.Data[i].Type == DigitalOutput)
		{
			if ((device.Data[i].DataTimer >= 0) && (device.Data[i].DataTimer <= (int)taskRate)) {
				device.Data[i].Value = device.Data[i].DefaultValue;
			}
			else if (device.Data[i].DataTimer >= 0) {
				device.Data[i].DataTimer -= taskRate;
			}

			pinMode(device.Data[i].Pin, OUTPUT);
			if (device.Data[i].Invert)
			{
				digitalWrite(device.Data[i].Pin, !device.Data[i].Value);
			}
			else
			{
				digitalWrite(device.Data[i].Pin, device.Data[i].Value);
			}
		}
		else if (device.Data[i].Type == DigitalInput)
		{
			pinMode(device.Data[i].Pin, INPUT);
			if (device.Data[i].Invert)
			{
				device.Data[i].Value = digitalRead(device.Data[i].Pin) == 0;
			}
			else
			{
				device.Data[i].Value = digitalRead(device.Data[i].Pin) == 1;
			}
		}
	}
}

void AC2_SerialClass::ReadMessages()
{
	int packetSize = 10;
	if (packetSize)
	{
		char incomingPacket[256];
		// receive incoming packets
		int len = 10;
		if (len > 0)
		{
			incomingPacket[len] = 0;

			String packet = incomingPacket;
			String packetRecipient = "";
			if (packet.startsWith(device.Name))// ignore messages not sent to you and not broadcast
			{
				packetRecipient = device.Name;
			}
			else if (packet.startsWith("Broadcast"))
			{
				packetRecipient = "Broadcast";
			}

			if (packetRecipient.length() != 0)
			{
				if (packet.startsWith(packetRecipient + ".Write"))
				{
					HandleWriteCommand(packet);
				}
			}
		}
	}
}

void AC2_SerialClass::SendMessages()
{
	Serial.println(device.Name + "," + device.Data[0].Value + "," + device.Data[1].Value);
}

void AC2_SerialClass::WriteIO(String DeviceName, IO io)
{
	Serial.println(DeviceName + ".Write(" + io.Name + ")%" + io.Value + "%{" + device.Name + "}");
}

void AC2_SerialClass::LocalWriteIO(IO io)
{
	HandleWriteCommand(device.Name + ".Write(" + io.Name + ")%" + io.Value + "%{" + device.Name + "}");
}

int  AC2_SerialClass::LocalReadIO(String ioName)
{
	for (int i = 0; i < 16; i++)
	{
		if (ioName.equalsIgnoreCase(device.Data[i].Name))
		{
			return	device.Data[i].Value;
		}
	}

	return 0;
}

void AC2_SerialClass::HandleWriteCommand(String command)
{
	int ioNameStart = command.indexOf("(", 0) + 1;
	int ioNameEnd = command.indexOf(")", ioNameStart + 1);
	String ioName = command.substring(ioNameStart, ioNameEnd);
	//Serial.println(ioName);
	for (int i = 0; i < 16; i++)
	{
		if (ioName.equalsIgnoreCase(device.Data[i].Name))
		{
			int valueStart = command.indexOf("%", 0) + 1;
			int valueEnd = command.indexOf("%", valueStart);
			String value = command.substring(valueStart, valueEnd);

			device.Data[i].Value = value.toInt();
			device.Data[i].DataTimer = device.Data[i].Timeout;
		}
	}
}

AC2_SerialClass AC2_Serial;

