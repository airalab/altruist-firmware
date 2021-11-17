/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */
#include <FS.h> 

static void writeDataFile(String data) {
	File dataFile = SPIFFS.open(F("/data1.json"), "w");
	if (dataFile) {
		dataFile.println(data);
		dataFile.close();
    }
	// 	debug_outln_info(F("Data written successfully."));
	// } else {
	// 	debug_outln_error(F("failed to open data file for writing"));
	// }
}

void setup()
{
  // initialize LED digital pin as an output.
  SPIFFS.begin();
  Serial.begin(9600);
}

void loop()
{
    // turn the LED on (HIGH is the voltage level)
    // File f = SPIFFS.open(F("/data1.json"), "w");

    // wait for a second
    delay(1000);
    // String kk = "hello";
    // // f.println(kk);
    // // f.close();
    // writeDataFile(String("hello"));

    File f1 = SPIFFS.open(F("/data.json"), "r");
		Serial.println(F("Reading Data from File:"));
			//Data from file
		int i;
		char st;
    Serial.println(String(f1.size()));
		for(i=0;i<f1.size();i++) //Read upto complete file size
		{
			st = char(f1.read());
			Serial.print(st);
		}
		f1.close();  //Close file
    // Serial.println(String(s));
    Serial.println("File Closed");

     // wait for a second
    delay(1000);
}