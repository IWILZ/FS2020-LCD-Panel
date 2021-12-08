/*
  LCD & CDI for FS2020
  --------------------
  This program uses two alphanumeric LCDs and 2 LED bars 
  to show and manage some in-flight parameters for FS2020.
  The 2 LCDs are used to: 
    1) Radio - display & edit stack radio NAV1, NAV2 e ADF
    2) Flight parameter - display some flight parameters (IAS, HDG, etc)

  The communications with the simulator is made 
  using "FS2020TA.exe" on the PC side.

  For the documentation see:
  https://github.com/IWILZ/FS2020-LCD-Panel/blob/main/README.md

  UPDATE LOG
  ----------
  V 1.0.0 -> First english version.
             IN_BUTTON and OUT_LED_3 still not used.
*/
#define PROGRAM_NAME1  "LCD & CDI FS2020"
#define PROGRAM_NAME2  "V 1.0.0"
#define PROGRAM_NAME3  "Initializing..."

#include <Arduino.h>
#include <BasicEncoder.h>       // Rotary encoder library
#include <LiquidCrystal_I2C.h>  // Manages serial communications with LCD 16x2 
#include <Grove_LED_Bar.h>      // Manages LED bars

/*************************************************************
             Parameter IDs received from FS2020
 *************************************************************/
#define ID_ADF_HDG      9       // ADF CARD (degrees HDG ADF)
#define ID_ADF_ACT_FREQ 7       // ADF ACTIVE FREQUENCY
#define ID_NAV_ACT_FREQ 502     // NAV 1 & 2 ACTIVE FREQ
#define ID_NAV_SBY_FREQ 526     // NAV 1 & 2 STANDBY FREQ
#define ID_NAV_OBS      519     // OBS 1 & 2 (degrees)
#define ID_HEADING      413     // HEADING INDICATOR
#define ID_AIRSPEED     37      // AIRSPEED INDICATED
#define ID_ALTITUDE     431     // INDICATED ALTITUDE
#define ID_QFE          557     // PLANE ALT ABOVE GROUND
#define ID_VARIOMETER   763     // VERTICAL SPEED
#define ID_NAV_CDI      505     // NAV CDI value of VORx/NAVx (range -127...+127) (index 1/2)
#define ID_NAV_HAS_NAV  516     // (Bool) NAVx active (index 1/2)
#define ID_NAV_CODES    506     // Binary mask for NAVx state (index 1/2)

#define NUM_FS_PARAM    19      // Number of parameters from FS2020

/*************************************************************
                    Commands to FS2020
 *************************************************************/
#define NAV1_INC_MHZ  "@568/$"  // NAV1_RADIO_WHOLE_INC
#define NAV2_INC_MHZ  "@577/$"  // NAV2_RADIO_WHOLE_INC
#define NAV1_DEC_MHZ  "@567/$"  // NAV1_RADIO_WHOLE_DEC
#define NAV2_DEC_MHZ  "@576/$"  // NAV2_RADIO_WHOLE_DEC
#define NAV1_INC_KHZ  "@564/$"  // NAV1_RADIO_FRACT_INC_CARRY
#define NAV2_INC_KHZ  "@573/$"  // NAV2_RADIO_FRACT_INC_CARRY
#define NAV1_DEC_KHZ  "@562/$"  // NAV1_RADIO_FRACT_DEC_CARRY
#define NAV2_DEC_KHZ  "@571/$"  // NAV2_RADIO_FRACT_DEC_CARRY
#define NAV1_SWAP     "@566/$"  // NAV1_RADIO_SWAP 
#define NAV2_SWAP     "@575/$"  // NAV2_RADIO_SWAP 
#define OBI1_INC      "@972/$"  // VOR1_OBI_INC
#define OBI1_DEC      "@971/$"  // VOR1_OBI_DEC
#define OBI2_INC      "@975/$"  // VOR2_OBI_INC
#define OBI2_DEC      "@974/$"  // VOR2_OBI_DEC
#define ADF_100_INC   "@8/$"    // ADF_100_INC
#define ADF_100_DEC   "@7/$"    // ADF_100_DEC
#define ADF_1_INC     "@19/$"   // ADF1_WHOLE_INC
#define ADF_1_DEC     "@18/$"   // ADF1_WHOLE_DEC
#define ADF_HDG_INC   "@10/$"   // ADF_CARD_INC  
#define ADF_HDG_DEC   "@9/$"    // ADF_CARD_DEC 
#define ADF_FRACT_INC_CARRY   "@14/$"   // Inc, ADF 1 freq. by 0.1 KHz, with carry  
#define ADF_FRACT_DEC_CARRY   "@13/$"   // Dec, ADF 1 freq. by 0.1 KHz, with carry  

// Editing buttons
#define BTN_ENC       4     // D4 - Encoder button x "Start editing" & "CONFIRM new value"
#define BTN_EXT       5     // D5 - Button x "Abort editing"

// Other I/O pins
#define IN_BUTTON     A0    // Button next to 3 LEDs over LED bars
#define OUT_LED_1     A1    // 1° LED over LED bars
#define OUT_LED_2     A2    // 2° LED over LED bars
#define OUT_LED_3     A3    // 3° LED over LED bars
#define OUT_LED_CDI   11    // LED between the 2 LED bars

#define N_STATUS_MAIN       5   // Number of states of the finite-state machine on the Radio LCD
#define N_VAL               10  // Number of values used by CalcMeanValue()
#define VAL_ALT             0   // CalcMeanValue() works on Altitude values
#define VAL_VARIO           1   // CalcMeanValue() works on Vertical Speed values
#define ASCII_ARROW         (char)0x7f  // Simbol code showing the actual parameter edited for frequency and HDG
#define MAX_COUNT_ALT_VAR   40  // Number of cycles to switch between QFE/vert.speed on the parameter LCD 

// Defines for LED bars
#define CDI_ZERO_RANGE  5       // Value range to swicth-on central LED
#define CDI_BAR_RANGE   3       // Minimum value to switch-on the first LED on the bars

/*************************************************************
                     GLOBAL VARIABLES
 *************************************************************/
// Pins D2 & D3 must be used on the Arduino Nano to manage hardware
// interrupts provided by rotation of the encoder. 
const int8_t EncoderPin1 = 3;
const int8_t EncoderPin2 = 2;

float     PrevActNav1, PrevSbyNav1; // Previous NAV1 frequencies
float     PrevActNav2, PrevSbyNav2; // Previous NAV2 frequencies
int       PrevNavObs1, PrevNavObs2; // Previous OBS 1 e 2 radials
float     PrevAdf;                  //
int       PrevHdg = 999;            //
int       PrevVario, PrevAltezza;   // Previous QFE & vert. speed 
int       PrevLedCdi;               // 
int       CountAltVar = 0, CountSlowDown = 0;
bool      ShowVario = false;        //
int       ValArray[2][N_VAL];       // Used by CalcMeanValue() 
long int  MeanVal;                  // Used by CalcMeanValue() 
int       Vor1Active=0, Vor2Active=0;
int       PrevVORActive=0;

/*********** Few vars useful for some debugging ****************/
unsigned long time_prev=0L, time_now=0L;
String dummy, tmp_str;

// ************** Struct of data received from FS2020 ******************
struct t_FromFS {
  int id;
  int index;
  String value;
};
t_FromFS FromFS;  

// Array storing all the parameters received from FS2020
// Each "value" field is filled by GetParamFromFS2020()
t_FromFS FromFSArray[NUM_FS_PARAM] = {
  {ID_ADF_HDG,        -1, "0"},   // 0
  {ID_ADF_ACT_FREQ,    1, "0.0"}, // 1
  {ID_NAV_ACT_FREQ,    1, "0.0"}, // 2 
  {ID_NAV_ACT_FREQ,    2, "0.0"}, // 3 
  {ID_NAV_SBY_FREQ,    1, "0.0"}, // 4 
  {ID_NAV_SBY_FREQ,    2, "0.0"}, // 5 
  {ID_NAV_OBS,         1, "0"},   // 6
  {ID_NAV_OBS,         2, "0"},   // 7
  {ID_HEADING,        -1, "0"},   // 8
  {ID_AIRSPEED,       -1, "0"},   // 9
  {ID_ALTITUDE,       -1, "0"},   // 10
  {ID_QFE,            -1, "0"},   // 11
  {ID_VARIOMETER,     -1, "0"},   // 12
  {ID_NAV_CDI,         1, "0"},   // 13
  {ID_NAV_CDI,         2, "0"},   // 14
  {ID_NAV_HAS_NAV,     1, "0"},   // 15
  {ID_NAV_HAS_NAV,     2, "0"},   // 16
  {ID_NAV_CODES,       1, "0"},   // 17
  {ID_NAV_CODES,       2, "0"}    // 18
};

// Define of each position inside the previous array of struct
#define POS_ADF_HDG       0     // ADF CARD (degrees HDG ADF)
#define POS_ADF_ACT_FREQ  1     // ADF ACTIVE FREQUENCY
#define POS_NAV_ACT_FREQ1 2     // NAV 1 & 2 ACTIVE FREQ
#define POS_NAV_ACT_FREQ2 3     // NAV 1 & 2 ACTIVE FREQ
#define POS_NAV_SBY_FREQ1 4     // NAV 1 & 2 STANDBY FREQ
#define POS_NAV_SBY_FREQ2 5     // NAV 1 & 2 STANDBY FREQ
#define POS_NAV_OBS1      6     // OBS 1 & 2 (degrees)
#define POS_NAV_OBS2      7     // OBS 1 & 2 (degrees)
#define POS_HEADING       8     // HEADING INDICATOR
#define POS_AIRSPEED      9     // AIRSPEED INDICATED
#define POS_ALTITUDE     10     // INDICATED ALTITUDE
#define POS_QFE          11     // PLANE ALT ABOVE GROUND
#define POS_VARIOMETER   12     // VERTICAL SPEED
#define POS_NAV_CDI1     13     // NAV CDI value of VORx/NAVx (range -127...+127) (index 1/2)
#define POS_NAV_CDI2     14     // NAV CDI value of VORx/NAVx (range -127...+127) (index 1/2)
#define POS_NAV_HAS_NAV1 15     // (Bool) NAV1 active
#define POS_NAV_HAS_NAV2 16     // (Bool) NAV2 active
#define POS_NAV_CODES1   17     // (Mask) NAV1 detail
#define POS_NAV_CODES2   18     // (Mask) NAV2 detail

// *****************************************************
// Starting status of the main finite-state machine
int StatusMain = 1, PrevStatusMain = -1;
// There is also a secondary finite-state machine for the 
// editing functions of NAV (with OBS) and ADF (with HDG).
int StatusEdit;

// ********* Defines of LCDs, encoder and LED bars **************
LiquidCrystal_I2C lcdRadio = LiquidCrystal_I2C(0x26, 16, 2); // Radio Stack
LiquidCrystal_I2C lcdParam = LiquidCrystal_I2C(0x27, 16, 2); // IAS, ALTITUDE, ecc
// Encoder
BasicEncoder encoder(EncoderPin1, EncoderPin2);
// LED bars 
Grove_LED_Bar BarL(7, 8, 1);    // Clock pin=D7, Data pin=D8, Orientation
Grove_LED_Bar BarR(9, 10, 1);   // Clock pin=D9, Data pin=D10, Orientation

// ***********************************************************************
// Interrupt Service Routine --> detects each "click" of the encoder
// ***********************************************************************
void ISRoutine()
{
  encoder.service();
}

// ***********************************************************************
// setup()
// ***********************************************************************
void setup() {
  // USB initialization
  Serial.begin(115200);
  Serial.setTimeout(3);

  // before to start i write something on each LCD just for test
  ChkLCDs();     

  BarL.begin();
  BarR.begin();
  // Switches off each LED on bars
  BarL.setLevel(0);
  BarR.setLevel(0);
  CheckBars();

  pinMode(OUT_LED_1, OUTPUT);
  pinMode(OUT_LED_2, OUTPUT);
  pinMode(OUT_LED_3, OUTPUT);
  pinMode(OUT_LED_CDI, OUTPUT);
  pinMode(IN_BUTTON, INPUT_PULLUP);
  pinMode(BTN_ENC, INPUT_PULLUP);
  pinMode(BTN_EXT, INPUT_PULLUP);

  // Setup Encoder
  pinMode(EncoderPin1, INPUT_PULLUP);
  pinMode(EncoderPin2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(EncoderPin1), ISRoutine, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EncoderPin2), ISRoutine, CHANGE);

  CheckLEDs();    // Switches on each LED for a while for test
  delay(300);
  InitLCDs();
} // setup()

// ***********************************************************************
// loop()
// ***********************************************************************
void loop() {
  String  strTmp;
  int     int_tmp;

  // Reads a parameter from FS stores it into FromFSArray[]
  GetParamFromFS2020();

  // The VORs status (and CDI) is checked at each loop
  CheckVORs(); 

  // Updates the status of the finite-state machine checking the encoder "clicks"
  StatusMain += encoder.get_change();
  if (StatusMain < 1) StatusMain = N_STATUS_MAIN - StatusMain;
  if (StatusMain > N_STATUS_MAIN) StatusMain = StatusMain - N_STATUS_MAIN;

  // Shows flight parameters
  ShowFlightParam();

  // Reads again a parameter from FS stores it into FromFSArray[]
  // in this way i can update the parameter list 2 times/loop
  GetParamFromFS2020();

  // What is the current status?
  switch (StatusMain) {
    case 1: // Act Nav1 & Nav2 + OBS1 & OBS2
      // The Radio LCD layout is drawn every time the previous status was different 
      if (PrevStatusMain != 1) {
        ShowDisplayRadio(); 
      }
      // --------- Reads both active frequencies from the array ----------
      PrevActNav1 = FromFSArray[POS_NAV_ACT_FREQ1].value.toFloat();
      PrevActNav2 = FromFSArray[POS_NAV_ACT_FREQ2].value.toFloat();
      // --------------- Reads both OBS from the array -------------------
      PrevNavObs1 = FromFSArray[POS_NAV_OBS1].value.toInt();
      PrevNavObs2 = FromFSArray[POS_NAV_OBS2].value.toInt();

      // ------------- We can show NAV1 & NAV2 on two lines ----------------
      ShowNav(PrevActNav1, PrevNavObs1, 1);  // NAV1 on the first row
      ShowNav(PrevActNav2, PrevNavObs2, 2);  // NAV2 on the second row
      PrevStatusMain = 1;
      break;

    case 2: // ADF & HDG
      // The Radio LCD layout is drawn every time the previous status was different 
      if (PrevStatusMain != 2) {
        ShowDisplayRadio(); 
      }
      PrevAdf = FromFSArray[POS_ADF_ACT_FREQ].value.toFloat();
      PrevHdg = FromFSArray[POS_ADF_HDG].value.toInt();
      ShowAdf (PrevAdf, PrevHdg, 1); // show ADF freq and HDG on the first row
      PrevStatusMain = 2;
      break;

    case 3: // Edit Nav1 (stdby) & OBS1
      // The Radio LCD layout is drawn every time the previous status was different 
      if (PrevStatusMain != 3) {
        ShowDisplayRadio(); 
      }
      // Read stby freq of NAV1 from the array
      PrevSbyNav1 = FromFSArray[POS_NAV_SBY_FREQ1].value.toFloat();
      ShowNav(PrevSbyNav1, PrevNavObs1, 2); 

      // Is the encoder button pressed? (to start editing)
      if (ButtonActive(BTN_ENC)) {
        EditNav(PrevSbyNav1, PrevNavObs1, 1); // go to edit NAV1
      }
      PrevStatusMain = 3;
      break;

    case 4: // Edit Nav2 (stdby) & OBS2
      // The Radio LCD layout is drawn every time the previous status was different 
      if (PrevStatusMain != 4) {
        ShowDisplayRadio(); 
      }
      // Read stby freq of NAV2 from the array
      PrevSbyNav2 = FromFSArray[POS_NAV_SBY_FREQ2].value.toFloat();
      ShowNav(PrevSbyNav2, PrevNavObs2, 2); 

      // Is the encoder button pressed? (to start editing)
      if (ButtonActive(BTN_ENC)) {
        EditNav(PrevSbyNav2, PrevNavObs2, 2); // go to edit NAV2
      }
      PrevStatusMain = 4;
      break;

    case 5: // Edit ADF & HDG
      // The Radio LCD layout is drawn every time the previous status was different 
      if (PrevStatusMain != 5) {
        ShowDisplayRadio(); 
      }
      // Read ADF freq and HDG from the array
      PrevAdf = FromFSArray[POS_ADF_ACT_FREQ].value.toFloat();
      PrevHdg = FromFSArray[POS_ADF_HDG].value.toInt();
      ShowAdf (PrevAdf, PrevHdg, 2); 
      
      // Is the encoder button pressed? (to start editing)
      if (ButtonActive(BTN_ENC)) {
        EditAdf(PrevAdf, PrevHdg); // go to edit ADF freq & HDG
      }
      PrevStatusMain = 5;
      break;
  } // switch
} // loop()


/***********************************************************
   ShowNav - Shows NavX freq. & OBSX on the row number "Row"
 ***********************************************************/
void ShowNav(float Nav, int Obs, byte Row) {
  float ActNav;
  char str[5];

  // To achieve the format NNN.DD we need to divide 
  // the frequency by 1000
  ActNav = Nav / 1000.0;
  lcdRadio.setCursor(4, Row - 1);
  lcdRadio.print(ActNav);
  sprintf (str, "%03d", Obs);
  lcdRadio.setCursor(13, Row - 1);
  lcdRadio.print(str);
} // ShowNav()


/***********************************************************
   ShowAdf - Shows ADF active frequency and HDG on the row "Row"
 ***********************************************************/
void ShowAdf(float Adf, int Hdg, byte Row) {
  char str_val[8];

  // *********** WARNING *************
  // Floating to string conversion using sprintf() don't works
  // The statement sprintf(str_val,"%6.1f",Adf)
  // produces a "?" into str_val
  // To solve this problem i use dtostrf()
  lcdRadio.setCursor(4, Row - 1);
  dtostrf(Adf, 6, 1, str_val);        // Works great!
  lcdRadio.print(str_val);

  sprintf (str_val, "%03d", Hdg);
  lcdRadio.setCursor(13, Row - 1);
  lcdRadio.print(str_val);
} // ShowAdf()


/***********************************************************
   InitLCDs
 ***********************************************************/
void InitLCDs() {
  // Radio stack LCD initialization
  lcdRadio.init();
  lcdRadio.backlight();

  // Ready for the status number 1
  lcdRadio.setCursor(0, 0);
  lcdRadio.print("N1:        -");
  lcdRadio.setCursor(0, 1);
  lcdRadio.print("N2:        -");

  // Flight parameter LCD initialization
  lcdParam.init();
  lcdParam.backlight();
  lcdParam.setCursor(0, 0);
  lcdParam.print("IAS:     HDG:      ");
  lcdParam.setCursor(0, 1);
  lcdParam.print("A/V:     /     ");
} // InitLCDs()

/***********************************************************
   ShowDisplayRadio
 ***********************************************************/
void ShowDisplayRadio() {
  // Initializes the Radio LCD according to StatusMain value
  switch (StatusMain) {
    case 1:
      lcdRadio.clear();
      lcdRadio.setCursor(0, 0);
      lcdRadio.print("N1:        -");
      lcdRadio.setCursor(0, 1);
      lcdRadio.print("N2:        -");
      break;
    case 2:
      lcdRadio.clear();
      lcdRadio.setCursor(0, 0);
      lcdRadio.print("ADF:       -");
      break;
    case 3:
      lcdRadio.clear();
      lcdRadio.setCursor(0, 0);
      lcdRadio.print("-- Edit NAV 1 --");
      lcdRadio.setCursor(0, 1);
      lcdRadio.print("S1:        -");
      break;
    case 4:
      lcdRadio.clear();
      lcdRadio.setCursor(0, 0);
      lcdRadio.print("-- Edit NAV 2 --");
      lcdRadio.setCursor(0, 1);
      lcdRadio.print("S2:        -");
      break;
    case 5:
      lcdRadio.clear();
      lcdRadio.setCursor(0, 0);
      lcdRadio.print("--- Edit ADF ---");
      lcdRadio.setCursor(0, 1);
      lcdRadio.print("ADF:       -");
      break;
  } // switch
} // ShowDisplayRadio

/***********************************************************
   ButtonActive 
   Checks a button status implementing a simple anti-bounce
 ***********************************************************/
bool ButtonActive(byte Button) {
  bool active = false;

  if (digitalRead(Button) == LOW) {
    delay(30);
    if (digitalRead(Button) == LOW) {
      active = true;
    }
  }

  // If button active i will wait until it will be released
  if (active) {
    do {
      // just wait for the release
    } while (digitalRead(Button) == LOW);
    // delay anti-bounce
    delay(20);
    return (true);
  }
  else return (false);
} // ButtonActive()

/***********************************************************
   EditNav - edit NAVx frequency and OBS
 ***********************************************************/
void EditNav(float Nav, int Obs, byte NavNumber) {
  float ActNav, NavTmp, NavOrig;
  char str[5];
  bool continue_loop = true, abort_edit = false;
  int encoderClick = 0, ObsTmp, ObsOrig;
  bool switch_flag=true;

  // Write constant strings on the LCD
  lcdRadio.clear();
  lcdRadio.setCursor(0, 0);
  lcdRadio.print("NAV  Frq:");
  lcdRadio.setCursor(3, 0);
  lcdRadio.print(NavNumber);
  lcdRadio.setCursor(0, 1);
  lcdRadio.print("Sw:Y    OBS:");

  NavOrig = Nav;
  ObsOrig = Obs;
  NavTmp = Nav;
  ObsTmp = Obs;

  // To achieve the format NNN.DD we need to divide 
  // the frequency by 1000
  ActNav = NavTmp / 1000.0;
  lcdRadio.setCursor(9, 0);
  lcdRadio.print(ActNav);
  sprintf (str, "%03d", ObsTmp);
  lcdRadio.setCursor(12, 1);
  lcdRadio.print(str);
  // Blinka the cursor on the arrow
  lcdRadio.setCursor(12, 0);
  // this symbol shows which is the parameter we are editing
  lcdRadio.print(ASCII_ARROW); 
  lcdRadio.setCursor(12, 0);
  lcdRadio.blink();

  // Secondary finite-state machine for the editing
  StatusEdit = 1; // We start editing MHz
  do {
    // Which is the status?
    switch (StatusEdit) {
      case 1: // Edit MHz
        encoderClick = encoder.get_change();
        if (encoderClick > 0) {
          NavTmp = NavTmp + 1000.0;
          if (NavTmp > 117950.0) NavTmp = 108000.0;
        }
        if (encoderClick < 0) {
          NavTmp = NavTmp - 1000.0;
          if (NavTmp < 108000.0) NavTmp = 117950.0;
        }

        // We need to update the MHz?
        if (encoderClick != 0) {
          ActNav = NavTmp / 1000.0;
          lcdRadio.setCursor(9, 0);
          lcdRadio.print(ActNav);
          // Blink the cursor on the decimal point
          lcdRadio.setCursor(12, 0);
          lcdRadio.print(ASCII_ARROW); 
          lcdRadio.setCursor(12, 0);
        }

        // Is the encoder button pressed?
        if (ButtonActive(BTN_ENC)) {
          StatusEdit = 2;     // go to the next status
          lcdRadio.setCursor(12, 0);
          lcdRadio.print(".");
          lcdRadio.setCursor(15, 0);
          lcdRadio.print(ASCII_ARROW);
        }
        // is the BTN_EXT (abort) pressed?
        if (ButtonActive(BTN_EXT)) {
          abort_edit = true;  // Abort edit
          continue_loop = false;
        }
        break;

      case 2: // Edit KHz
        // Blink the cursor after the decimal digits
        lcdRadio.setCursor(15, 0);
        encoderClick = encoder.get_change();
        if (encoderClick > 0) {
          NavTmp = NavTmp + 50.0;
          if (NavTmp > 117950.0) NavTmp = 108000.0;
        }
        if (encoderClick < 0) {
          NavTmp = NavTmp - 50.0;
          if (NavTmp < 108000.0) NavTmp = 117950.0;
        }

        // We need to update the frequency?
        if (encoderClick != 0) {
          ActNav = NavTmp / 1000.0;
          lcdRadio.setCursor(9, 0);
          lcdRadio.print(ActNav);
        }
        // Is the encoder button pressed?
        if (ButtonActive(BTN_ENC)) {
          StatusEdit = 3;     // go to the next status
          lcdRadio.setCursor(15, 0);
          lcdRadio.print(" ");
          // move the arrow after "Sw:Y"
          lcdRadio.setCursor(4, 1);
          lcdRadio.print(ASCII_ARROW);
        }
        // is the BTN_EXT (abort) pressed?
        if (ButtonActive(BTN_EXT)) {
          abort_edit = true;  // Abort edit
          continue_loop = false;
        }
        break;

      case 3: // Edit switch flag
        // This flag indicates if the user want to switch 
        // stdby<->active frequency or not.
        
        // Blink the cursor after "Sw:Y"
        lcdRadio.setCursor(4, 1);
        encoderClick = encoder.get_change();
        if (encoderClick != 0) {
          switch_flag=!switch_flag;
          lcdRadio.setCursor(3, 1);
          if (switch_flag) lcdRadio.print('Y');
             else lcdRadio.print('N');
          lcdRadio.setCursor(4, 1);
        }
        // Is the encoder button pressed?
        if (ButtonActive(BTN_ENC)) {
          StatusEdit = 4;     // go to the next status
          lcdRadio.setCursor(4, 1);
          lcdRadio.print(" ");
          // move the arrow after "OBS:"
          lcdRadio.setCursor(15, 1);
          lcdRadio.print(ASCII_ARROW);
        }
        // is the BTN_EXT (abort) pressed?
        if (ButtonActive(BTN_EXT)) {
          abort_edit = true;  // Abort edit
          continue_loop = false;
        }
        break;

      case 4: // Edit OBS
        // Blink the cursor after OBS digits
        lcdRadio.setCursor(15, 1);
        encoderClick = encoder.get_change();
        if (encoderClick > 0) {
          ObsTmp++;
          if (ObsTmp > 359) ObsTmp = 0;
        }
        if (encoderClick < 0) {
          ObsTmp--;
          if (ObsTmp < 0) ObsTmp = 359;
        }
        // We need to update OBS value?
        if (encoderClick != 0) {
          sprintf (str, "%03d", ObsTmp);
          lcdRadio.setCursor(12, 1);
          lcdRadio.print(str);
          // Blink the cursor after the digits
          lcdRadio.setCursor(15, 1);
        }
        // Is the encoder button pressed?
        if (ButtonActive(BTN_ENC)) {
          continue_loop = false;
        }
        // is the BTN_EXT (abort) pressed?
        if (ButtonActive(BTN_EXT)) {
          abort_edit = true;  // Abort edit
          continue_loop = false;
        }
        break;
    } // switch
  } while (continue_loop);

  lcdRadio.clear();
  lcdRadio.noBlink();
  lcdRadio.setCursor(0, 0);

  // We need to store new data on FS?
  if (!abort_edit) {
    lcdRadio.print("- Saving  data -");
    // Saves new values for the NAV "NavNumber" and if needed switches stby-active
    SaveNewNavSetup(NavTmp, NavOrig, NavNumber, switch_flag);
    delay(50);
    SaveNewObsSetup(ObsTmp, ObsOrig, NavNumber);
  }
  else {
    lcdRadio.print("-- NO  CHANGE --");
  } // if abort_edit
  delay (2000); // just shows the message exiting the edit function
  StatusMain = 1;  // We restart from the main status 1
} // EditNav()


/***********************************************************
   EditAdf - edit ADF frequency and course  
 ***********************************************************/
void EditAdf(float Adf, int Hdg) {
  int Adf1Tmp, Adf100Tmp, AdfDecTmp, HdgOrig, HdgTmp;
  float AdfOrig;
  int i_tmp;
  float f_tmp;
  char str[5];
  bool continue_loop = true, abort_edit = false;
  int encoderClick = 0, ObsTmp, ObsOrig;

  // Draw fixed strings on the LCD
  lcdRadio.clear();
  lcdRadio.setCursor(0, 0);
  lcdRadio.print("ADF Frq:     .");
  lcdRadio.setCursor(10, 0);
  lcdRadio.print(ASCII_ARROW); 
  lcdRadio.setCursor(4, 1);
  lcdRadio.print("HDG: ");

  AdfOrig = Adf;
  HdgOrig = Hdg;
  Adf100Tmp = (int) (Adf / 100.0); // Hundreds of KHz (1..17)
  Adf1Tmp = (int) (Adf - Adf100Tmp*100); // Units of KHz (1..99)
  // Extracting the first decimal digit after "."
  i_tmp=((int)Adf)*10;
  f_tmp=(Adf*10.0-(float)i_tmp);
  AdfDecTmp=(int)f_tmp; // First decimal digit of the frequency

  HdgTmp = Hdg;

  sprintf (str, "%2d", Adf100Tmp);
  lcdRadio.setCursor(8, 0);
  lcdRadio.print(str);
  sprintf (str, "%02d", Adf1Tmp);
  lcdRadio.setCursor(11, 0);
  lcdRadio.print(str);
  sprintf (str, "%1d", AdfDecTmp);
  lcdRadio.setCursor(14, 0);
  lcdRadio.print(str);

  sprintf (str, "%03d", HdgTmp);
  lcdRadio.setCursor(8, 1);
  lcdRadio.print(str);
  lcdRadio.setCursor(10, 0);
  lcdRadio.blink();

  // Secondary finite-state machine for the editing
  StatusEdit = 1; // We start editing hundred of KHz
  do {
    // Which is the status?
    switch (StatusEdit) {
      case 1: // Edit hundred of Khz (1..17)
        encoderClick = encoder.get_change();
        if (encoderClick > 0) {
          Adf100Tmp++;
          if (Adf100Tmp > 17) Adf100Tmp = 1;
        }
        if (encoderClick < 0) {
          Adf100Tmp--;
          if (Adf100Tmp < 1) Adf100Tmp = 17;
        }

        // We need to update the hundreds?
        if (encoderClick != 0) {
          lcdRadio.setCursor(8, 0);
          sprintf (str, "%2d", Adf100Tmp);
          lcdRadio.print(str);
          // Blink the cursor after the hundreds
          lcdRadio.setCursor(10, 0);
        }

        // Is the encoder button pressed?
        if (ButtonActive(BTN_ENC)) {
          StatusEdit = 2;     // go to the next status
          lcdRadio.setCursor(10, 0);
          lcdRadio.print("-");
          lcdRadio.setCursor(13, 0);
          lcdRadio.print(ASCII_ARROW);
        }
        // is the BTN_EXT (abort) pressed?
        if (ButtonActive(BTN_EXT)) {
          abort_edit = true;  // Abort edit
          continue_loop = false;
        }
        break;

      case 2: // Edit KHz (1..99)
        // Blink the cursor after the digits
        lcdRadio.setCursor(13, 0);
        encoderClick = encoder.get_change();
        if (encoderClick > 0) {
          Adf1Tmp++;
          if (Adf1Tmp > 99) Adf1Tmp = 0;
        }
        if (encoderClick < 0) {
          Adf1Tmp--;
          if (Adf1Tmp < 0) Adf1Tmp = 99;
        }

        // We need to update the frequency?
        if (encoderClick != 0) {
          sprintf (str, "%02d", Adf1Tmp);
          lcdRadio.setCursor(11, 0);
          lcdRadio.print(str);
        }
        // Is the encoder button pressed?
        if (ButtonActive(BTN_ENC)) {
          StatusEdit = 3;     // go to the next status
          lcdRadio.setCursor(13, 0);
          lcdRadio.print(".");
          // predispone la freccia dopo l'HDG
          lcdRadio.setCursor(15, 0);
          lcdRadio.print(ASCII_ARROW);
        }
        // is the BTN_EXT (abort) pressed?
        if (ButtonActive(BTN_EXT)) {
          abort_edit = true;  // Abort edit
          continue_loop = false;
        }
        break;

      case 3: // edit decimal (0..9)
        lcdRadio.setCursor(15, 0);
        encoderClick = encoder.get_change();
        if (encoderClick > 0) {
          AdfDecTmp++;
          if (AdfDecTmp > 9) AdfDecTmp = 0;
        }
        if (encoderClick < 0) {
          AdfDecTmp--;
          if (AdfDecTmp < 0) AdfDecTmp = 9;
        }

        // We need to update the frequency?
        if (encoderClick != 0) {
          sprintf (str, "%d", AdfDecTmp);
          lcdRadio.setCursor(14, 0);
          lcdRadio.print(str);
        }
        // Is the encoder button pressed?
        if (ButtonActive(BTN_ENC)) {
          StatusEdit = 4;     // go to the next status
          lcdRadio.setCursor(15, 0);
          lcdRadio.print(" ");
          lcdRadio.setCursor(11, 1);
          lcdRadio.print(ASCII_ARROW);
        }
        // is the BTN_EXT (abort) pressed?
        if (ButtonActive(BTN_EXT)) {
          abort_edit = true;  // Abort edit
          continue_loop = false;
        }
        break;

      case 4: // Edit HDG
        // Blink the cursor after HDG digits
        lcdRadio.setCursor(11, 1);
        encoderClick = encoder.get_change();
        if (encoderClick > 0) {
          HdgTmp++;
          if (HdgTmp > 359) HdgTmp = 0;
        }
        if (encoderClick < 0) {
          HdgTmp--;
          if (HdgTmp < 0) HdgTmp = 359;
        }
        // We need to update HDG value?
        if (encoderClick != 0) {
          sprintf (str, "%03d", HdgTmp);
          lcdRadio.setCursor(8, 1);
          lcdRadio.print(str);
        }
        // Is the encoder button pressed?
        if (ButtonActive(BTN_ENC)) {
          continue_loop = false;
        }
        // is the BTN_EXT (abort) pressed?
        if (ButtonActive(BTN_EXT)) {
          abort_edit = true;  // Abort edit
          continue_loop = false;
        }
        break;
    } // switch
  } while (continue_loop);

  lcdRadio.clear();
  lcdRadio.noBlink();
  lcdRadio.setCursor(0, 0);

  // We need to store new data on FS?
  if (!abort_edit) {
    lcdRadio.print("- Saving  data -");
    // Send new values for ADF frequency and HDG
    SaveNewAdfSetup(Adf100Tmp, Adf1Tmp, AdfDecTmp, AdfOrig, HdgTmp, HdgOrig);
  }
  else {
    lcdRadio.print("-- NO  CHANGE --");
  } // if abort_edit
  delay (2000); // just shows the message exiting the edit function
  StatusMain = 2;  // // We restart from the main status 2 (display ADF & HDG)
} // EditAdf()


/***********************************************************
   SaveNewAdfSetup - Send new values for ADF frequency & HDG
 ***********************************************************/
void SaveNewAdfSetup(int Adf100New, int Adf1New, int AdfDecNew, float AdfOrig, int HdgNew, int HdgOrig) {
  int freq_tmp, i, i_tmp;

  // Sending first 2 ADF frequency digits (hundreds)
  freq_tmp = AdfOrig / 100;
  if (Adf100New > freq_tmp)
    for (i = freq_tmp; i < Adf100New; i++)
      Serial.print(ADF_100_INC);  // ADF_100_INC
  if (Adf100New < freq_tmp)
    for (i = Adf100New; i < freq_tmp; i++)
      Serial.print(ADF_100_DEC);  // ADF_100_DEC

  // Sending last 2 ADF frequency digits (units)
  freq_tmp = (int) (AdfOrig - freq_tmp*100); // KHz units (1..99)
  if (Adf1New > freq_tmp)
    for (i = freq_tmp; i < Adf1New; i++)
      Serial.print(ADF_1_INC);  // Increments ADF by 1 KHz
  if (Adf1New < freq_tmp)
    for (i = Adf1New; i < freq_tmp; i++)
      Serial.print(ADF_1_DEC);  // Decrements ADF by 1 KHz

  // Extracting the first decimal digit after "."
  i_tmp=((int)AdfOrig)*10;
  freq_tmp=(int)(AdfOrig*10.0-(float)i_tmp); // First decimal digit of the frequency
  if (AdfDecNew > freq_tmp)
    for (i = freq_tmp; i < AdfDecNew; i++)
      Serial.print(ADF_FRACT_INC_CARRY);  // 
  if (AdfDecNew < freq_tmp)
    for (i = AdfDecNew; i < freq_tmp; i++)
      Serial.print(ADF_FRACT_DEC_CARRY);  // 

  // Sending new HDG
  if (HdgNew > HdgOrig)
    for (i = HdgOrig; i < HdgNew; i++)
      Serial.print(ADF_HDG_INC);  // ADF_CARD_INC
  if (HdgNew < HdgOrig)
    for (i = HdgNew; i < HdgOrig; i++)
      Serial.print(ADF_HDG_DEC);  // ADF_CARD_DEC

} // SaveNewAdfSetup()


/***********************************************************
   SaveNewNavSetup 
   Send new freq values for NavNumber stdby NAV and OBS
 ***********************************************************/
void SaveNewNavSetup(float FreqNew, float FreqOrig, byte NavNumber, bool switch_flag) {
  int MHzFreqNew, MHzFreqOrig;
  int KHzFreqNew, KHzFreqOrig;
  int i, delta;

  MHzFreqNew = GetMHz(FreqNew);
  KHzFreqNew = GetKHz(FreqNew);
  MHzFreqOrig = GetMHz(FreqOrig);
  KHzFreqOrig = GetKHz(FreqOrig);

  // Are the MHz changed?
  if (MHzFreqNew != MHzFreqOrig) {
    delta = MHzFreqOrig - MHzFreqNew;
    delta = abs(delta);

    for (i = 0; i < delta; i++) {
      if (MHzFreqNew > MHzFreqOrig) {
        switch (NavNumber) {
          case 1:
            Serial.print(NAV1_INC_MHZ);
            break;
          case 2:
            Serial.print(NAV2_INC_MHZ);
            break;
        } // switch
      } // if

      if (MHzFreqNew < MHzFreqOrig) {
        switch (NavNumber) {
          case 1:
            Serial.print(NAV1_DEC_MHZ);
            break;
          case 2:
            Serial.print(NAV2_DEC_MHZ);
            break;
        } // switch
      } // if
    } // for
  } // if (MHzFreqNew!=MHzFreqOrig)

  // Are KHz changed?
  if (KHzFreqNew != KHzFreqOrig) {
    delta = KHzFreqOrig - KHzFreqNew;
    delta = abs(delta);
    // each "inc" or "dec" acts on 50KHz
    delta = delta / 5;

    for (i = 0; i < delta; i++) {
      if (KHzFreqNew > KHzFreqOrig) {
        switch (NavNumber) {
          case 1:
            Serial.print(NAV1_INC_KHZ);
            break;
          case 2:
            Serial.print(NAV2_INC_KHZ);
            break;
        } // switch
      } // if

      if (KHzFreqNew < KHzFreqOrig) {
        switch (NavNumber) {
          case 1:
            Serial.print(NAV1_DEC_KHZ);
            break;
          case 2:
            Serial.print(NAV2_DEC_KHZ);
            break;
        } // switch
      } // if
    } // for
  } // if (KHzFreqNew!=KHzFreqOrig)

  // If switch_flag=true i need also to switch the frequencies 
  if (switch_flag) {
    switch (NavNumber) {
      case 1:
        Serial.print(NAV1_SWAP);
        break;
      case 2:
        Serial.print(NAV2_SWAP);
        break;
    } // switch
  } // if
} // SaveNewNavSetup()


/***********************************************************
   SaveNewObsSetup - Sends the new OBS value for the NAV
   "NavNumber"
 ***********************************************************/
void SaveNewObsSetup(int ObsNew, int ObsOrig, byte NavNumber) {
  int i, delta;
  // Are OBS degrees changed?
  if (ObsNew != ObsOrig) {
    delta = ObsOrig - ObsNew;
    delta = abs(delta);

    for (i = 0; i < delta; i++) {
      if (ObsNew > ObsOrig) {
        switch (NavNumber) {
          case 1:
            Serial.print(OBI1_INC);
            break;
          case 2:
            Serial.print(OBI2_INC);
            break;
        } // switch
      } // if

      if (ObsNew < ObsOrig) {
        switch (NavNumber) {
          case 1:
            Serial.print(OBI1_DEC);
            break;
          case 2:
            Serial.print(OBI2_DEC);
            break;
        } // switch
      } // if
    } // for
  } // if (ObsNew!=ObsOrig)
} // SaveNewObsSetup()

/***********************************************************
   GetMHz - return the MHz part of a NAV fequency
 ***********************************************************/
int GetMHz(float Freq) {
  int retval = 0;
  retval = Freq / 1000.0;
  return (retval);
} // GetMHz()

/***********************************************************
   GetKHz - return the KHz part of a NAV fequency
 ***********************************************************/
int GetKHz(float Freq) {
  int retval = 0;
  retval = Freq / 1000.0;
  retval = (Freq - ((float) retval * 1000.0)) / 10;
  return (retval);
} // GetKHz()


/***********************************************************
  GetParamFromFS2020()
  Simply get the first valid parameter received from 
  FS2020TA.exe and calls StoreData()
 ***********************************************************/
void GetParamFromFS2020() {
int at, slash, dollar, equal;
  
  if (Serial.available() > 0) {
    // Read from the USB a string until the first '\n'
    dummy = Serial.readStringUntil('\n');

    at = dummy.indexOf('@');
    slash = dummy.indexOf('/');
    equal = dummy.indexOf('=');
    dollar = dummy.indexOf('$');

    // The first character of the string MUST be a '@'
    if (at==0 && dollar>=5) {
      tmp_str = dummy.substring(at + 1, slash);           // Extract "id" from the string
      FromFS.id = tmp_str.toInt();                        // 
      tmp_str = dummy.substring(slash + 1, equal);        // Extract "index" from the string
      FromFS.index = tmp_str.toInt();                     // 
      FromFS.value = dummy.substring(equal + 1, dollar);  // Extract "value" from the string
      StoreData();    
    }
  }
} // GetParamFromFS2020()


/***********************************************************
 * Dummy function useful for some DEBUGGING
 ***********************************************************/
void PrintFromFSArray(){
  int i;
  for (i=0; i<NUM_FS_PARAM; i++){
    Serial.print(i);
    Serial.print("->");
    Serial.print(FromFSArray[i].id);
    Serial.print("/");
    Serial.print(FromFSArray[i].index);
    Serial.print("=");
    Serial.println(FromFSArray[i].value);
  }  
} // PrintFromFSArray()

/***********************************************************
  StoreData()
  Just stores the "FromFS" parameter into the right place of
  the FromFSArray[] array
 ***********************************************************/
void StoreData() {
  int i;
  bool found=false;
  
  for (i=0; i<NUM_FS_PARAM && !found; i++){
    if (FromFSArray[i].id==FromFS.id && FromFSArray[i].index==FromFS.index){
      FromFSArray[i].value=FromFS.value;
      found=true;
    }
  }
} // StoreData


/***********************************************************
  ShowFlightParam()
  Shows flight parameters on the lcdParam LCD
 ***********************************************************/
void ShowFlightParam() {
char row_tmp[32];

  // Update the IAS
    sprintf (row_tmp, "%-3d ", FromFSArray[POS_AIRSPEED].value.toInt());
    lcdParam.setCursor(5, 0);
    lcdParam.print(row_tmp);

  // Update the aircraft HDG
    sprintf (row_tmp, "%03d ", FromFSArray[POS_HEADING].value.toInt());
    lcdParam.setCursor(13, 0);
    lcdParam.print(row_tmp);

  // Update the ALTITUDE
    CalcMeanValue(VAL_ALT, FromFSArray[POS_ALTITUDE].value.toInt());
    sprintf (row_tmp, "%5d", MeanVal); 
//    sprintf (row_tmp, "%5d", FromFSArray[POS_ALTITUDE].value.toInt());
    lcdParam.setCursor(4, 1);
    lcdParam.print(row_tmp);

  // Read and save vert. speed and QFE to use them later
    CalcMeanValue(VAL_VARIO, FromFSArray[POS_VARIOMETER].value.toInt());
    PrevVario = MeanVal;
//    PrevVario=FromFSArray[POS_VARIOMETER].value.toInt();
    PrevAltezza = FromFSArray[POS_QFE].value.toInt();

  // At the end of the second line QFE (format NNNNN) and vert. speed 
  // (format sNNNN) are displayed alternately each MAX_COUNT_ALT_VAR cycles 
  CountAltVar++;
  if (CountAltVar >= MAX_COUNT_ALT_VAR) {
    CountAltVar = 0;
    CountSlowDown = 0;
    ShowVario = !ShowVario; // switches the 2 values
  } // if (CountAltVar)

  CountSlowDown++;
  if (ShowVario) {
    if (CountSlowDown >= MAX_COUNT_ALT_VAR/3) {
      CountSlowDown = 0;
      sprintf (row_tmp, "%+04d ", PrevVario);
      lcdParam.setCursor(10, 1);
      lcdParam.print(row_tmp);
    }
  }
  else {
    if (CountSlowDown >= MAX_COUNT_ALT_VAR/3) {
      CountSlowDown = 0;
      sprintf (row_tmp, "%d        ", PrevAltezza);
      lcdParam.setCursor(10, 1);
      lcdParam.print(row_tmp);
    }
  } // if (ShowVario)

} // ShowFlightParam(){


/***********************************************************
  CalcMeanValue()
  Calculates the value as an integer average of the previous 
  N_VAL integers already "sampled".
  It is used to "soften" excessively variable values such as 
  altitude and vertical speed. 
  Anyway this function is nice but don't seems very useful.
 ***********************************************************/
void CalcMeanValue(int type, int val) {
  int i;

  MeanVal = 0L; // it's a long int
  // Move forward N_VAL-1 values
  for (i = 0; i < N_VAL - 1; i++) {
    ValArray[type][i + 1] = ValArray[type][i];
    MeanVal = MeanVal + ValArray[type][i]; // Updates the sum
  } // for
  ValArray[type][0] = val;
  MeanVal = MeanVal + ValArray[type][0];
  MeanVal = MeanVal / N_VAL;
  return;
} // CalcMeanValue()


/***********************************************************
  CheckVORs()
  Check if there is an active VOR (valid signal on the active 
  frequency) and consequently manages the 2 LED bars as CDI 
 ***********************************************************/
void CheckVORs() {
  bool NAV1act = false, NAV2act = false;

  // Reads the 2 NAV status (that is the 2 CDIs)
  Vor1Active = FromFSArray[POS_NAV_HAS_NAV1].value.toInt();  // VOR1 status  
  Vor2Active = FromFSArray[POS_NAV_HAS_NAV2].value.toInt();  // VOR2 status  

  // I decide which of the 2 CDIs is to be displayed giving priority to VOR1
  if (Vor1Active == 1){
    PrevVORActive = 1;
    ShowCdi(FromFSArray[POS_NAV_CDI1].value.toInt(), 1);
  } else {
    if (Vor2Active == 1){ 
      PrevVORActive = 2;
      ShowCdi(FromFSArray[POS_NAV_CDI2].value.toInt(), 2);
    }
  }
 
  // If neither of the 2 radios are receiving a valid signal 
  // turn off the CDI LEDs 
  if (Vor1Active == 0 && Vor2Active == 0) OffCdi();
} // CheckVORs

/***********************************************************
  OffCdi()
  Turns off all LEDs that have to do with the CDI / ILS
***********************************************************/
void OffCdi() {
  PrevVORActive = 0;
  digitalWrite(OUT_LED_1, HIGH);        // Spegne il LED VOR1
  digitalWrite(OUT_LED_2, HIGH);        // Spegne il LED VOR2
  digitalWrite(OUT_LED_3, HIGH);        // Spegne il LED "IS LOCALIZER"
  digitalWrite(OUT_LED_CDI, HIGH);      // Spegne il LED centrale
  BarL.setBits(0);
  BarR.setBits(0);
} // OffCdi

/***********************************************************
  ShowCdi()
  The function first checks that one of the 2 VORs is a LOCALIZER (ILS)
  Then it displays the Course Deviation Indicator via 2 bars of 10 LEDs 
  + a central LED for the active VOR.
  It should be noted that "cdi" is in the range -127 ... + 127 and 
  it represents the entire excursion of the CDI on the VOR (from left 
  to right).
  The central LED lights up around zero, that is: 
  -CDI_ZERO_RANGE <= cdi <= CDI_ZERO_RANGE. 
 ***********************************************************/
void ShowCdi(int cdi, int n_nav) {
  int led, led_abs;

  // If none of the 2 is an ILS, turn off the "IS LOCALIZER" LED
  if ((FromFSArray[POS_NAV_CODES1].value.toInt() & 0x0080)==0 && 
      (FromFSArray[POS_NAV_CODES2].value.toInt() & 0x0080)==0) 
    digitalWrite(OUT_LED_3, HIGH); 
    
  if (n_nav == 1){
    if (FromFSArray[POS_NAV_CODES1].value.toInt() & 0x0080) // If bit 7=1, it is an ILS
      digitalWrite(OUT_LED_3, LOW); // Switch on the "IS LOCALIZER" LED 
    digitalWrite(OUT_LED_1, LOW);   // Switch on the LED VOR1
    digitalWrite(OUT_LED_2, HIGH);  // Switch off the LED VOR2
  }
  if (n_nav == 2){
    if (FromFSArray[POS_NAV_CODES2].value.toInt() & 0x0080) // If bit 7=1, it is an ILS
      digitalWrite(OUT_LED_3, LOW); // Switch on the "IS LOCALIZER" LED 
    digitalWrite(OUT_LED_2, LOW);   // Switch on the LED VOR2
    digitalWrite(OUT_LED_1, HIGH);  // Switch off the LED VOR1
  }

  // The central LED lights up around the "0"
  if (abs(cdi) <= CDI_ZERO_RANGE) 
    digitalWrite(OUT_LED_CDI, LOW); // Switch on the LED 
  else
    digitalWrite(OUT_LED_CDI, HIGH); // Switch off the LED
  led = (int)(cdi * 10 / 127);
  // The bars are turned off only if the led to be lit is not 
  // the one that is already lit or if we are around 0
  if (led != PrevLedCdi || led==0){
    BarL.setBits(0);
    BarR.setBits(0);
    PrevLedCdi = led;
  }
  
  if (abs(cdi) > CDI_BAR_RANGE) {
    led_abs = abs(led) + 1;
    // At the extremes of the range there we can have "led_abs = 11"
    if (led_abs > 10)
      led_abs = 10; 
    if (cdi > 0) {
      // Barra di dx
      BarR.setLed(led_abs, 1);
    } else {
      if (cdi < 0) {
        // Barra di sx
        BarL.setLed(led_abs, 1);
      }
    }
  }
} // ShowCdi()


// ***********************************************************************
// CheckLEDs()
// Used at stat up just to check that every LED is working
// ***********************************************************************
void CheckLEDs() {
  int i;
  for (i = 0; i < 2; i++) {
    // Switch on the LEDs
    digitalWrite(OUT_LED_1, LOW); 
    digitalWrite(OUT_LED_2, LOW); 
    digitalWrite(OUT_LED_3, LOW); 
    digitalWrite(OUT_LED_CDI, LOW); 
    delay (200);
    // Switch off the LEDs
    digitalWrite(OUT_LED_1, HIGH); 
    digitalWrite(OUT_LED_2, HIGH); 
    digitalWrite(OUT_LED_3, HIGH); 
    digitalWrite(OUT_LED_CDI, HIGH); 
    delay (200);
  }
} // CheckLEDs

// ***********************************************************************
// CheckBars()
// Used at stat up just to check that the bars are working
// ***********************************************************************
void CheckBars() {
  int i;
  for (i = 1; i <= 10; i++) {
    BarR.setLed(i, 1);
    BarL.setLed(i, 1);
    delay (100);
    BarL.setBits(0);
    BarR.setBits(0);
  }
} // CheckBars

// ***********************************************************************
// ChkLCDs()
// Used at stat up just to check that the LCDs are working
// ***********************************************************************
void ChkLCDs() {
  lcdRadio.init();
  lcdRadio.backlight();
  lcdParam.init();
  lcdParam.backlight();
  lcdRadio.setCursor(0, 0);
  lcdRadio.print(PROGRAM_NAME1);
  lcdRadio.setCursor(0, 1);
  lcdRadio.print(PROGRAM_NAME2);
  lcdParam.setCursor(0, 0);
  lcdParam.print(PROGRAM_NAME3);
  delay (300);
} // ChkLCDs
