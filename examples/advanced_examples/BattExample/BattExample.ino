/*
 *	Sample code using both Battery.h and VoltageReference.h libraries by rlogiacco
 *	Battery Sense - https://github.com/rlogiacco/BatterySense
 *	VoltageReference - https://github.com/rlogiacco/VoltageReference
 *	Based on sample code also by rlogiacco
 *	Brought to you by MicaelJarniac
 *	Contact me at micaeljarniac@gmail.com
 *	v0002
 *	May 06, 2018
 */

#include <VoltageReference.h> //Including VoltageReference library
#include <Battery.h> //Including Battery Sense library

#define SERIAL_BAUD 57600

#define BOARD_VOLTAGE 5000 //Target board nominal voltage, in mV - Usually 5000, for 5V boards, or 3300, for 3.3V boards

#define CELL_MIN 3200 //Target battery nominal cell minimal ("fully discharged") safe voltage, in mV - If not using LiPo batteries, set this to the minimal acceptable voltage and CELL_NUM to 1
#define CELL_MAX 4200 //Target battery nominal cell maximum ("fully charged") voltage, in mV - If not using LiPo batteries, set this to the maximum voltage and CELL_NUM to 1

#define CELL_NUM 3 //Target battery number of cells - If not using LiPo batteries, set this to 1 and CELL_MAX and CELL_MIN to the corresponding values according to your batteries

#define BATT_MIN ((CELL_NUM) * (CELL_MIN)) //Calculation of target battery nominal minimal ("fully discharged") safe voltage, in mV
#define BATT_MAX ((CELL_NUM) * (CELL_MAX)) //Calculation of target battery nominal maximum ("fully charged") voltage, in mV

//Measure both the resistors used for the voltage divider with, e.g., a multimeter, and enter the measured values below, for better accuracy during battery measurements
//Using nominal values is also possible, but may generate less accurate results
#define VOLDIV_R1 21600 //Measured or nominal resistence of voltage divider R1 resistor, in Ohm
#define VOLDIV_R2 10120 //Measured or nominal resistance of voltage divider R2 resistor, in Ohm

#define VOLDIV_RTOTAL ((VOLDIV_R1) + (VOLDIV_R2)) //Calculation of voltage divider total resistance, in Ohm

#define VOLDIV_RATIO (((float)(VOLDIV_RTOTAL)) / ((float)(VOLDIV_R2))) //Calculation of voltage divider ratio

#define VOLDIV_MINRATIO (((float)(BATT_MAX)) / ((float)(BOARD_VOLTAGE))) //Calculation of nominal minimal safe voltage divider ratio

#define VOLDIV_DRAWMAX (((float)(BATT_MAX)) / ((float)(VOLDIV_RTOTAL))) //Calculation of voltage divider maximum current draw, in mA

#define VOLDIV_DRAWNOW (((float)(batt.voltage())) / ((float)(VOLDIV_RTOTAL))) //Calculation of voltage divider current current draw, in mA, to only be used after measurement is running

//The below code is commented out because the C preprocessor can't compare floating point values, and I haven't figured how to make it work yet
/*
#if (((int)((VOLDIV_RATIO) * 1000)) < ((int)((VOLDIV_MINRATIO) * 1000))) //Warning if voltage divider ratio is below nominal minimal safe voltage divider ratio
	#error "!WARNING! CURRENT VOLTAGE DIVIDER RATIO IS BELOW MINIMAL VALUE !WARNING!"
#endif
*/

#define BATT_PIN A4 //Analog input pin wired to measure target battery

#define BATT_ONDEMAND 1 //Set it to 1 if using on-demand configuration, and 0 otherwise
#define BATT_TRIGGERPIN 3 //Digital output pin wired to trigger voltage divider on on-demand configurations, and not needed otherwise
#define BATT_TRIGGERSTATE HIGH //State of BATT_TRIGGERPIN during target battery measurement - Usually HIGH or LOW

#define BATT_MAP &sigmoidal //Battery level mapping function

VoltageReference vRef; //Initializing VoltageReference library as vRef
Battery batt(BATT_MIN, BATT_MAX, BATT_PIN); //Initializing Battery library as batt, using target battery calculated values and selected analog input pin

int battRefresh = 2000; //Time between measurements of target battery, in ms

bool safety = true; //Enables battery safety feature

//Uncomment below line to enable debug features, or comment it to disable
#define ENABLE_DEBUG
bool debugBat = true; //Enables target battery debugging - Should ONLY be used for quick tests, and disabled afterwards, because it WILL DRAIN the target battery quicker for on-demand configurations and slow down the code, apart from other possible side-effects

void setup() {
	Serial.begin(SERIAL_BAUD);
	Serial.print("Initializing...\n");
	Serial.print("Initializing battery sensor...\n");
	vRef.begin(); //Starts vRef
	delay(500); //Delay to give vRef some time to properly initialize and measure board voltage - May be unneeded
	if(BATT_ONDEMAND) batt.onDemand(BATT_TRIGGERPIN, BATT_TRIGGERSTATE); //Configures on-demand measurement, using selected digital output pin and selected state
	batt.begin(vRef.readVcc(), VOLDIV_RATIO, BATT_MAP); //Starts batt, using measured board voltage, in mV, calculated voltage divider ratio and selected mapping function
	char battstring[100];
	sprintf(battstring, "Battery Info: BOARD_VOLTAGE=%dmV\tCELL_MIN=%dmV\tCELL_MAX=%dmV\tCELL_NUM=%d\tBATT_MIN=%dmV\tBATT_MAX=%dmV\tVOLDIV_R1=%dOhm\tVOLDIV_R2=%dOhm\tVOLDIV_RTOTAL=%d\tVcc=%dmV", BOARD_VOLTAGE, CELL_MIN, CELL_MAX, CELL_NUM, BATT_MIN, BATT_MAX, VOLDIV_R1, VOLDIV_R2, VOLDIV_RTOTAL, vRef.readVcc());
	Serial.print(battstring);
	Serial.print("\tVOLDIV_RATIO=");
	Serial.print(VOLDIV_RATIO, 6);
	Serial.print("\tVOLDIV_MINRATIO=");
	Serial.print(VOLDIV_MINRATIO, 2); //Prints information about target battery, voltage divider and measured board voltage
	Serial.print("\tVOLDIV_DRAWMAX=");
	Serial.print(VOLDIV_DRAWMAX, 2);
	Serial.println("mA");
	Serial.print("DONE!\n");
	delay(500); //Delay to give batt some time to initialize - May be unneeded

	//Enter rest of setup here

}

void loop() {
	static unsigned long timeBatt = 0;

	#ifdef ENABLE_DEBUG
		if(debugBat) {
			Serial.print("voltage=");
			Serial.print(batt.voltage());
			Serial.print("mV\tlevel=");
			Serial.print(batt.level());
			Serial.print("%\tvoltage divider current current draw=");
			Serial.print(VOLDIV_DRAWNOW);
			Serial.println("mA");
		} //Prints debug information about target battery
	#endif //ENABLE_DEBUG

	if(millis() - timeBatt >= battRefresh) {
		timeBatt = millis();
		if(safety && (batt.level() <= 1)) { //Battery safety feature, that's triggered if enabled and if target battery measured percentage is equal or below 1%
			Serial.print("\nLOW VOLTAGE! EMERGENCY LOCKDOWN LOOP! SHUTDOWN MANUALLY ASAP!\n");
			//Insert code to warn about low voltage and to turn everything off here
			while(1) {
				delay(100000); //Infinite loop that repeats every 100s and does nothing, sort of stopping the code in order to avoid target battery damage (perhaps there are better ways of doing this)
			}
		}

		//Insert measurement routine here, like code to print target battery voltage, percentage, etc, to be run on timed intervals according to battRefresh value

		/*
			batt.voltage() returns measured target battery voltage, in mV
			batt.level() returns calculated target battery percentage based on measured voltage and selected mapping function
			VOLDIV_DRAWNOW returns current current draw based on measured voltage and total voltage divider resistence, in mA
		*/

	}

	//Insert rest of code here

}
