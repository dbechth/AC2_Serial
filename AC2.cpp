// 
// 
// 

#include "AC2.h"
#include <ArduinoOTA.h>
#include "index.h" 
#include "home.h" 
#include "terminal.h"

unsigned long taskRate;
unsigned long taskCount;


void AC2Index()
{

	AC2.webserver.send(200, "text/html", MAIN_page);
}

void AC2Home()
{
	AC2.webserver.send(200, "text/html", home_page);
}

void terminal()
{
	AC2.webserver.send(200, "text/html", terminal_page);
}

void handleTerminal()
{
	String tmpBuffer = "";
	if (AC2.bufferCount != 0)
	{
		for (int i = 0; i < (AC2.bufferCount); i++)
		{
			tmpBuffer += AC2.buffer[i];
		}
		AC2.bufferCount = 0;
	}
	AC2.webserver.send(200, "text/plain", tmpBuffer);
}

String BuildJSON(String paramName, String Value)
{
	return "\"" + paramName + "\":\"" + Value + "\"";
}

void getData()
{
	String IOIndex = AC2.webserver.arg("IOIndex");
	int idx = 0;
	if (IOIndex != "")
	{
		idx = IOIndex.toInt();
		if (idx > 15)
		{
			idx = 15;
		}
		if (idx < 0)
		{
			idx = 0;
		}
	}
	AC2Class::IO tmpIO = AC2.device.Data[idx];

	String tmpBuffer = "{ " +
		BuildJSON("Name", tmpIO.Name) + ", " +
		BuildJSON("Pin", String(tmpIO.Pin)) + ", " +
		BuildJSON("Timeout", String(tmpIO.Timeout)) + ", " +
		BuildJSON("DataTimer", String(tmpIO.DataTimer)) + ", " +
		BuildJSON("Type", String(tmpIO.Type)) + ", " +
		BuildJSON("DefaultValue", String(tmpIO.DefaultValue)) + ", " +
		BuildJSON("Value", String(tmpIO.Value)) + ", " +
		BuildJSON("Invert", String(tmpIO.Invert)) +
		"}";

	AC2.webserver.send(200, "text/plain", tmpBuffer);
}

void AC2Class::print(String message)
{
	if (bufferCount >= ACBufferSize)
	{
		buffer[0] = "Buffer Overflow! Messages were lost";
		bufferCount = 1;
	}
	buffer[bufferCount] = message;
	bufferCount++;
}

void AC2Class::println(String message)
{
	print(message + "<br>");
}
bool AC2Class::init(String name, IPAddress ip, IPAddress broadcastIP, int port, int TaskRateMS)
{
	device.Name = name;
	device.IP = ip;
	device.Port = port;
	device.BroadcastIP = broadcastIP;//IPADDR_BROADCAST;
	taskRate = TaskRateMS;
	SetupOTA();

	//webserver.on("/", AC2Home);      //Which routine to handle at root location. This is display page
	webserver.on("/AC2", AC2Index);      //Which routine to handle at root location. This is display page
	webserver.on("/getData", getData);
	webserver.on("/terminal", terminal);      //Which routine to handle at root location. This is display page
	webserver.on("/handleTerminal", handleTerminal);
	webserver.begin();

	bufferCount = 0;

	//get started off right
	timeNow = millis();
	lastTime = timeNow;
	return Udp.begin(device.Port);
}


void AC2Class::task()
{

	ArduinoOTA.handle();
	webserver.handleClient();

	timeNow = millis();
	unsigned long elapsedTime = timeNow - lastTime;
	if (elapsedTime >= taskRate) {
		lastTime = timeNow;
		ReadMessages();
		HandleIO();
		//WriteIO();
		//SendMessages();
		taskCount++;
	}

}

void AC2Class::SetupOTA()
{
	// Port defaults to 8266
 // ArduinoOTA.setPort(8266);

 // Hostname defaults to esp8266-[ChipID]
	ArduinoOTA.setHostname(device.Name.c_str());

	// No authentication by default
	// ArduinoOTA.setPassword("admin");

	// Password can be set with it's md5 value as well
	// MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
	// ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) {
			type = "sketch";
		}
		else { // U_FS
			type = "filesystem";
		}

		// NOTE: if updating FS this would be the place to unmount FS using FS.end()
		Serial.println("Start updating " + type);
		});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd");
		});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
		});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		}
		else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		}
		else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		}
		else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		}
		else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
		});
	ArduinoOTA.begin();
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

void AC2Class::HandleIO()
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

void AC2Class::ReadMessages()
{
	int packetSize = Udp.parsePacket();
	if (packetSize)
	{
		char incomingPacket[256];
		// receive incoming UDP packets
		Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
		int len = Udp.read(incomingPacket, 255);
		if (len > 0)
		{
			incomingPacket[len] = 0;

			print(String(millis()) + " " + "UDP: ");
			println(incomingPacket);
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
				//Serial.println(packet);

				if (packet.startsWith(packetRecipient + ".WhoIs"))
				{
					//Serial.println(packet);
					//HandleWhoIs(packet);
				}
				else if (packet.startsWith(packetRecipient + ".IAm"))
				{
					//Serial.println(packet);
					//HandleIAm(packet);
				}
				else if (packet.startsWith(packetRecipient + ".Write"))
				{
					//Serial.println(packet);
					HandleWriteCommand(packet);
				}
				else if (packet.startsWith(packetRecipient + ".Read"))
				{
					//Serial.println(packet);
					HandleReadCommand(packet);
				}
				else if (packet.startsWith(packetRecipient + ".Reply"))
				{
					//Serial.println(packet);
					HandleReplyCommand(packet);
				}
				else if (packet.startsWith(packetRecipient + ".Configure"))
				{
					//Serial.println(packet);
					HandleConfigureCommand(packet);
				}
			}

		}
	}
}

void AC2Class::SendMessages()
{
	Udp.beginPacket(device.BroadcastIP, device.Port);
	Udp.print(device.Name + "," + device.Data[0].Value + "," + device.Data[1].Value);
	Udp.endPacket();
}

void AC2Class::WriteIO(String DeviceName, IO io)
{

	if (Udp.beginPacket(device.BroadcastIP, device.Port) == 1)
	{
		Udp.print(DeviceName + ".Write(" + io.Name + ")%" + io.Value + "%{" + device.Name + "}");
		if (Udp.endPacket() == 0)
		{
			Serial.println("reply send failed");
		}
	}
	else
	{
		Serial.println("beginPacket failed");
	}


}

void AC2Class::LocalWriteIO(IO io)
{
	HandleWriteCommand(device.Name + ".Write(" + io.Name + ")%" + io.Value + "%{" + device.Name + "}");
}

int  AC2Class::LocalReadIO(String ioName)
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

void AC2Class::HandleWriteCommand(String command)
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
			//Serial.println(value);
			device.Data[i].Value = value.toInt();
			device.Data[i].DataTimer = device.Data[i].Timeout;
			//Serial.print(device.Data[i].Name);
			//Serial.print(": ");
			//Serial.println(device.Data[i].DataTimer);
		}
	}
}

//void AC2Class::HandleWhoIs(String command)
//{
//    int ioNameStart = command.indexOf("(", 0) + 1;
//    int ioNameEnd = command.indexOf(")", ioNameStart + 1);
//    String ioName = command.substring(ioNameStart, ioNameEnd);
//    //Serial.println(ioName);
//    for (int i = 0; i < 16; i++)
//    {
//        if (ioName.equalsIgnoreCase(device.Data[i].Name))
//        {
//            Serial.print("Updating... Task Count: ");
//            Serial.print(taskCount);
//            Serial.print(", Name: ");
//            Serial.print(device.Data[i].Name);
//            Serial.print(", Timer: ");
//            Serial.println(device.Data[i].DataTimer);
//            int valueStart = command.indexOf("%", 0) + 1;
//            int valueEnd = command.indexOf("%", valueStart);
//            String value = command.substring(valueStart, valueEnd);
//            //Serial.println(value);
//            device.Data[i].Value = value.toInt();
//            device.Data[i].DataTimer = device.Data[i].Timeout;
//
//            Serial.print("Updating... Task Count: ");
//            Serial.print(taskCount);
//            Serial.print(", Name: ");
//            Serial.print(device.Data[i].Name);
//            Serial.print(", Timer: ");
//            Serial.println(device.Data[i].DataTimer);
//        }
//    }
//}

void AC2Class::HandleReplyCommand(String command)
{
	//int ioNameStart = command.indexOf("(", 0) + 1;
	//int ioNameEnd = command.indexOf(")", ioNameStart + 1);
	//String ioName = command.substring(ioNameStart, ioNameEnd);
	//Serial.println(ioName);
	//for (size_t i = 0; i < 16; i++)
	//{
	//   if (ioName.equalsIgnoreCase(device.Data[i].Name))
	//   {
	//      int valueStart = command.indexOf("%", 0) + 1;
	//      int valueEnd = command.indexOf("%", valueStart);
	//      String value = command.substring(valueStart, valueEnd);
	//      Serial.println(value);
	//      device.Data[i].Value = (uint8_t)value.toInt();
	//      device.Data[i].Timeout = 60000;
	//      pinMode(device.Data[i].Pin, OUTPUT);
	//      digitalWrite(device.Data[i].Pin, device.Data[i].Value);
	//   }
	//}
}

void AC2Class::HandleReadCommand(String command)
{
	//int ioNameStart = command.indexOf("(", 0) + 1;
	//int ioNameEnd = command.indexOf(")", ioNameStart + 1);
	//String ioName = command.substring(ioNameStart, ioNameEnd);
	//Serial.println(ioName);
	//for (size_t i = 0; i < 16; i++)
	//{
	//   if (ioName.equalsIgnoreCase(device.Data[i].Name))
	//   {
	//      int valueStart = command.indexOf("%", 0) + 1;
	//      int valueEnd = command.indexOf("%", valueStart);
	//      String value = command.substring(valueStart, valueEnd);
	//      Serial.println(value);
	//      device.Data[i].Value = (uint8_t)value.toInt();
	//      device.Data[i].Timeout = 60000;
	//      pinMode(device.Data[i].Pin, OUTPUT);
	//      digitalWrite(device.Data[i].Pin, device.Data[i].Value);
	//   }
	//}
}

void AC2Class::HandleConfigureCommand(String command)
{
	//int ioNameStart = command.indexOf("(", 0) + 1;
	//int ioNameEnd = command.indexOf(")", ioNameStart + 1);
	//String ioName = command.substring(ioNameStart, ioNameEnd);
	//Serial.println(ioName);
	//for (size_t i = 0; i < 16; i++)
	//{
	//   if (ioName.equalsIgnoreCase(device.Data[i].Name))
	//   {
	//      int valueStart = command.indexOf("%", 0) + 1;
	//      int valueEnd = command.indexOf("%", valueStart);
	//      String value = command.substring(valueStart, valueEnd);
	//      Serial.println(value);
	//      device.Data[i].Value = (uint8_t)value.toInt();
	//      device.Data[i].Timeout = 60000;
	//      pinMode(device.Data[i].Pin, OUTPUT);
	//      digitalWrite(device.Data[i].Pin, device.Data[i].Value);
	//   }
	//}
}

AC2Class AC2;

